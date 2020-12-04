/*
 * webproxy.c - A web proxy server using pthreads
 * starting with C code from HTTP Web Server project
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>			/* for fgets */
#include <strings.h>		/* for bzero, bcopy */
#include <unistd.h>			/* for read, write */
#include <sys/socket.h> /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> 			/* for open */
#include <sys/types.h>  /* for open (UNIX)*/
#include <sys/stat.h>		/* for open (UNIX)*/
#include <pthread.h>
#include <signal.h> 		/* to gracefully stop */
#include <limits.h>
#include <time.h> 			/* for time keeping, and cache timeout */
#include <dirent.h> 		/* for reading directory */

#define MAXLINE 8192		/* max text line length */
#define MAXBUF 8192			/* max I/O buffer size */
#define LISTENQ 1024		/* second argument to listen() */
#define EXPIRATION 10 * (24 * 60 * 60)	/* days (first number) before a cache is deleted */

volatile sig_atomic_t done = 0;

int open_listenfd(int port);
void *thread(void *vargp);
void proxy_res(int n);
int blacklisted(char host[100]);
int is_page_cached(char host[100]);
void write_cache_page_to_file(char host[100], char url[100], int size);
void read_cache_page_from_file(char host[100], int connfd);
void check_file_and_dirs_exist();
void check_page_caches();
void check_hostname_caches();
void term(int signum);
void httpError(char buf[MAXLINE], int connfd, int error, char httpVersion[10]);

/*
 * main driver
 */
int main(int argc, char **argv) {
	int listenfd, *connfdp, port, timeout = 0, exp_check = 0;
	struct sockaddr_in clientaddr;
	socklen_t clientlen;
	pthread_t tid;

	// check arguments, then assign proxy's port
	if (2 != argc && argc != 3) {
		fprintf(stderr, "usage: %s <port> <timeout>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);
	if (argc == 3) timeout = atoi(argv[2]);

	// gracefully exit
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	// for timeout handling
	time_t start_t, end_t;
	double diff_t;
	time(&start_t);

	printf("Graceful exit: escape character is 'Ctrl+C'.\n");

	// continuous listening of proxy server
	listenfd = open_listenfd(port);
	while (!done) {
		// check for expiration times (caches older than a certain time)
		if (!done && !exp_check) {
			check_file_and_dirs_exist();
			check_page_caches();
			check_hostname_caches();
			exp_check = 1;
		}

		// do not check/purge blacklist, as these should be managed manually
		connfdp = malloc(sizeof(int));
		*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		// printf("Connected to http://localhost:%i on socket %i\n", port, *connfdp);
		pthread_create(&tid, NULL, thread, connfdp);

		// timeout setting
		if (timeout) {
			time(&end_t);
			diff_t = difftime(end_t, start_t);
			if (diff_t > timeout){
				printf("Exiting due to proxy server timeout.\n");
				done = 1;
			}
		}
	}
	// once while loop gracefully exits, close connections
	shutdown(*connfdp, 0);
	close(*connfdp);
	close(listenfd);
}

/*
 * requests sent to web proxy & web proxy's response
 */
void proxy_res(int connfd) {
	char buf[MAXBUF], httpmsg[MAXLINE], *http_request[3];
	char filestruct[MAXLINE];
	char host[100], page[100], ip_addr[20];
	int socket_msg, port = 80, server_socket;
	struct hostent *resolve_hostname;
	struct sockaddr_in serveraddr;
	socklen_t serverlen;
	size_t n;

	// receive message from socket
	socket_msg = recv(connfd, httpmsg, MAXLINE, 0);

	// connection closed, possibly from idle response
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
  timeinfo = localtime ( &rawtime );
	if (socket_msg == 0) {
		printf("Socket %i: connection was closed at %s\n", socket_msg, asctime (timeinfo));
	}
	// not so good, client socket error
	else if (socket_msg < 0) {
		printf("Fatal socket connection error"); exit(EXIT_FAILURE);
	}

	// connected
	else if (socket_msg > 0) {
		// parse request
		http_request[0] = strtok(httpmsg, " \r\t"); // method
		http_request[1] = strtok(NULL, " \r\t"); // url
		http_request[2] = strtok(NULL, " \r\t\n"); // http version

		// printf("what's method? %s\n", http_request[0]);
		// printf("what's url? %s\n", http_request[1]);
		// printf("what's version? %s\n", http_request[2]);

		// make sure formatting properly
		if (strncmp(http_request[1], "http", 4) != 0 || strncmp(http_request[2], "HTTP", 4) != 0)
			httpError(buf, connfd, 0, NULL);

		// check if http version is supported (not checking for HTTP/0.9)
		if (strcmp(http_request[2], "HTTP/1.0\0") != 0 && strcmp(http_request[2], "HTTP/1.1\0") != 0
				&& strcmp(http_request[2], "HTTP/2.0\0") != 0 && strcmp(http_request[2], "HTTP/2\0") != 0) {
			httpError(buf, connfd, 505, http_request[2]);
		}

		// GET request from client
		if (strcmp(http_request[0], "GET\0") == 0) {
			/* URL PARSING */
			// for HTTPS
			if (strncmp(http_request[1], "https", 5) == 0) {
				// parse url with port
				if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7)
					sscanf(http_request[1], "https://%99[^:]:%6i/%99[^\n]", host, &port, page);
				// parse url with no port specified, but has content request
				else if ((int)(strrchr(http_request[1], '/') - http_request[1]) > 7)
					sscanf(http_request[1], "https://%99[^/]/%99[^\n]", host, page);
				// parse url with no content request, but has port
				else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7)
					sscanf(http_request[1], "https://%99[^:]:%6i[^\n]", host, &port);
				// just has regular host request (no port or content request)
				else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 1)
					sscanf(http_request[1], "https://%99[^\n]", host);
				// else an error, since unable to parse URL
				else httpError(buf, connfd, 404, http_request[2]);
			}
			// FOR HTTP
			else if (strncmp(http_request[1], "http", 4) == 0) {
				// parse url with port
				if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7)
					sscanf(http_request[1], "http://%99[^:]:%6i/%99[^\n]", host, &port, page);
				// parse url with no port specified, but has content request
				else if ((int)(strrchr(http_request[1], '/') - http_request[1]) > 7)
					sscanf(http_request[1], "http://%99[^/]/%99[^\n]", host, page);
				// parse url with no content request, but has port
				else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7)
					sscanf(http_request[1], "http://%99[^:]:%6i[^\n]", host, &port);
				// just has regular host request (no port or content request)
				else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 1)
					sscanf(http_request[1], "http://%99[^\n]", host);
				// else an error, since unable to parse URL
				else httpError(buf, connfd, 404, http_request[2]);
			} else httpError(buf, connfd, 404, http_request[2]);

			// to use for page caching
			strcpy(filestruct, host);
			if(strcmp(page, "") != 0) {
				strcat(filestruct, "_");
				strcat(filestruct, page);
				strcat(filestruct, ".txt");
			}

			memset(&serveraddr, 0, serverlen);
			serveraddr.sin_family = AF_INET;
			//? serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
			serveraddr.sin_port = htons((int)port);

			// check if hostname has already been resolved
			FILE *host_cache_file;
			char hostname[100];
			char cache_ip[20];
			host_cache_file = fopen("hostname_cache.txt", "r");
			while (!feof(host_cache_file)) {
				fscanf(host_cache_file, "%s%s%*s%*[^\n]", hostname, cache_ip);
				if (strcmp(host, hostname) == 0) {
					strcpy(ip_addr, cache_ip);
					break;
				}
			}
			fclose(host_cache_file);

			// host's IP not cached, resolve
			if (strcmp(ip_addr, "") == 0) {
				// not resolved, so get the hostname and store it
				resolve_hostname = gethostbyname(host);
				if (resolve_hostname == NULL) {
					printf("**Unable to resolve hostname: %s.**\n", host);
					httpError(buf, connfd, 404, http_request[2]);
				} else {
					memcpy(&serveraddr.sin_addr, resolve_hostname->h_addr, resolve_hostname->h_length);
					// write to hostname cache file
					host_cache_file = fopen("hostname_cache.txt", "a");
					fprintf(host_cache_file, "%s ", host);
					fprintf(host_cache_file, "%s ", inet_ntoa(serveraddr.sin_addr));
					fprintf(host_cache_file, "%li\n", time(&rawtime));
					fclose(host_cache_file);
				}
			} else { // server IP in hostname cache
				serveraddr.sin_addr.s_addr = inet_addr(ip_addr);
			}
			//? printf("Hostname address resolved to: %s\n", inet_ntoa(serveraddr.sin_addr));

			// check for blacklisted hosts or IPs
			if (blacklisted(host) || blacklisted(ip_addr)) {
				httpError(buf, connfd, 403, http_request[2]);
			}

			// check if server's page is cached
			if (is_page_cached(filestruct)) {
				read_cache_page_from_file(filestruct, connfd);
			} else {
				// create HTTP server socket
				if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
					printf("**Error creating HTTP server socket.**\n");
				};

				// connect to HTTP server
				if (connect(server_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
					printf("**Error connecting to HTTP server @ %s.**\n", host);
				} else { // connected to HTTP server
					memset(&buf, 0, MAXBUF);

					// create message to send to HTTP server
					strcpy(buf, http_request[0]);
					strcat(buf, " ");
					strcat(buf, http_request[1]);
					strcat(buf, " ");
					strcat(buf, http_request[2]);
					strcat(buf, "\r\n\r\n");

					// printf("request %s", buf);
					// send the response from the HTTP client to the HTTP server
					if (send(server_socket, buf, strlen(buf), 0) < 0) {
						httpError(buf, connfd, 404, http_request[2]); // error on send
					}

					/* Send message back to client from server */
					memset(&buf, 0, MAXBUF); // result buffer to use it
					char new_url[100];
					while ((n = recv(server_socket, buf, sizeof(buf), 0)) > 0) {
						// printf("Data from server %s retrieved, sending back to client at socket %i\n", host, connfd);
						// write to file (cache), while also sending to client
						write_cache_page_to_file(filestruct, buf, n);
						send(connfd, buf, n, 0);
					}
					// close connection
					shutdown(connfd, 0);
					close(connfd);
				}
			}
		} else httpError(buf, connfd, 400, http_request[2]); // request method
	} else httpError(buf, connfd, 500, http_request[2]); // socket issues

	shutdown(connfd, 0);
	close(connfd);
}

/* thread routine */
void *thread(void *vargp) {
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	proxy_res(connfd);
	free(vargp);
	close(connfd);
	return NULL;
}

/*
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure
 */
int open_listenfd(int port) {
	int listenfd, optval = 1;
	struct sockaddr_in serveraddr;

	/* Create a socket descriptor */
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	/* Eliminates "Address already in use" error from bind. */
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0)
		return -1;

	/* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	/* binds the server socket */
	if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
		return -1;

	/* Make it a listening socket ready to accept connection requests */
	if (listen(listenfd, LISTENQ) < 0) {
		perror("listenfd, listen fn\n");
		return -1;
	}

	return listenfd;
} /* end open_listenfd */

/* Checks if server hostname or IP is blacklisted */
int blacklisted(char host[100]) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	fp = fopen("blacklist.txt", "r");
	while (getline(&line, &len, fp) != -1) {
		if (strcmp(host, line) == 0) {
			fclose(fp);
			return 1; // server is blacklisted
		}
	}
	fclose(fp);
	return 0; // server is not blacklisted
}

/* Checks if pages are cached, so as to not fetch them */
int is_page_cached(char host[100]) {
	char filename[100] = "./page_caches/";
	strcat(filename, host);
	struct stat buffer;
	if (stat(filename, &buffer) == 0) return 1; // exists
	return 0; // file doesn't exist
}

/* if page doesn't already exist, write to cache as returned to customer */
void write_cache_page_to_file(char host[100], char buf[MAXBUF], int size) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	char filename[100] = "./page_caches/";
	strcat(filename, host);
	fp = fopen(filename, "a");
	strcat(buf, "\n");
	fwrite(buf, 1, size, fp);
	fclose(fp);
}

/* if cache file exists, return to client */
void read_cache_page_from_file(char host[100], int connfd) {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	char buf[MAXBUF];
	char filename[100] = "./page_caches/";
	strcat(filename, host);
	fp = fopen(filename, "r");
	// read in file contents and send to client
	while((len = fread(buf, 1, sizeof(buf), fp)) > 0) {
		send(connfd, buf, len, 0);
	}
	fclose(fp);
}

/* make sure the necessary files and directories are present */
void check_file_and_dirs_exist(){
	// before anything, make sure the directory exists
	struct stat st = {0};
	FILE *fp;
	if (stat("./page_caches", &st) == -1)
		mkdir("./page_caches", 0700);

	if (access("blacklist.txt", F_OK) == -1) {
		fp = fopen("blacklist.txt", "w");
		fclose(fp);
	}

	if (access("hostname_cache.txt", F_OK) == -1) {
		fp = fopen("hostname_cache.txt", "w");
		fclose(fp);
	}
}

/* look to see if any page caches need to be removed (beyond expiration) */
void check_page_caches() {
	/* look through each file in the directory (except '.' and '..')
	 * and delete if past EXPIRATION timeframe */
	DIR *FD = opendir("./page_caches/");
	struct dirent *de;
	struct stat filestat;
	char filename[50];
	while((de = readdir(FD)) != NULL) {
		time_t rawtime;
		time (&rawtime);
		strcpy(filename, "./page_caches/");
		strcat(filename, de->d_name);
		stat(filename, &filestat);
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
			time_t file_time = filestat.st_birthtime;
			if((time(&rawtime) - file_time) > EXPIRATION) {
				remove(filename);
			}
		}
	}
	closedir(FD);
}

/* check if any hostnames need to be removed from list (past expiration date) */
void check_hostname_caches() {
	FILE *fp;
	FILE *tmp;
	int file_time;
	time_t rawtime;
	char *line = NULL;
	size_t len;
	fp = fopen("hostname_cache.txt", "r");
	tmp = fopen("hostname_cache_tmp.txt", "a");
	while ((getline(&line, &len, fp)) != -1) {
		sscanf(line, "%*s %*s %i%*[^\n]", &file_time);
		// file has not yet expired
		if((time(&rawtime) - file_time) < EXPIRATION) {
			fprintf(tmp, "%s", line);
		}
	}
	fclose(tmp);
	fclose(fp);
	remove("hostname_cache.txt");
	rename("hostname_cache_tmp.txt", "hostname_cache.txt");
}

/* Gracefully exit program (while loop) with Ctrl+C press */
void term(int signum) {
	done = 1;
}

/* Error handling for HTTP client requests */
void httpError(char buf[MAXLINE], int connfd, int error, char httpVersion[10]) {
	switch (error) {
		case 400:
			strcat(buf, httpVersion);
			strcat(buf, " 400 Bad Request");
			break;
		case 403:
			strcat(buf, httpVersion);
			strcat(buf, " 403 Forbidden");
			break;
		case 404:
			strcat(buf, httpVersion);
			strcat(buf, " 404 Not Found");
			break;
		case 500:
			strcat(buf, httpVersion);
			strcat(buf, " 500 Internal Server Error");
			break;
		case 505:
			strcat(buf, httpVersion);
			strcat(buf, " 505 HTTP Version not supported");
			break;
		default:
			strcat(buf, "Invalid request format. Use: <METHOD> <PATH> <VERSION>. Refer to README for more info.");
			break;
	}
	strcat(buf, "\n");
	send(connfd, buf, strlen(buf), 0);
	shutdown(connfd, 0);
	close(connfd);
}