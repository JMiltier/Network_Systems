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
#include <time.h> 			/* for time keeping */

#define MAXLINE 8192 /* max text line length */
#define MAXBUF 8192	 /* max I/O buffer size */
#define LISTENQ 1024 /* second argument to listen() */

volatile sig_atomic_t done = 0;

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
void httpError(char buf[MAXLINE], int connfd, int error, char httpVersion[10]);
void term(int signum);
void proxy_res(int n);
char *getcwd(char *buf, size_t size);

/*
 * main driver
 */
int main(int argc, char **argv) {
	int listenfd, *connfdp, port;
	struct sockaddr_in clientaddr;
	socklen_t clientlen;
	pthread_t tid;

	// check arguments, then assign proxy's port
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	// gracefully exit
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

	printf("Graceful exit: escape character is 'Ctrl+C'.\n");

	// continuous listening of proxy server
	listenfd = open_listenfd(port);
	while (!done) {
		connfdp = malloc(sizeof(int));
		*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		// printf("Connected to http://localhost:%i on socket %i\n", port, *connfdp);
		pthread_create(&tid, NULL, thread, connfdp);
	}

	// once while loop gracefully exits, close connections
	// shutdown(*connfdp, 0);
	// close(*connfdp);
}

/*
 * requests sent to web proxy & web proxy's response
 */
void proxy_res(int connfd) {
	char buf[MAXLINE], httpmsg[MAXLINE], *http_request[3], host[100], page[100];
	int socket_msg, port = 80, httpsocket;
	struct hostent *phe;
	struct sockaddr_in proxyaddr;
	socklen_t proxylen;
	size_t n;

	// set working directory
	char cwd[MAXLINE];
	getcwd(cwd, sizeof(cwd));

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
	// not so good
	else if (socket_msg < 0) {
		printf("Fatal socket connection error"); exit(EXIT_FAILURE);
	}

	// connected
	else if (socket_msg > 0) {
		// parse request
		http_request[0] = strtok(httpmsg, " \r\t\n"); // method
		http_request[1] = strtok(NULL, " \r\t\n"); // url
		http_request[2] = strtok(NULL, " \r\t\n"); // http version

		// check if http version is supported (not checking for HTTP/0.9)
		if (strcmp(http_request[2], "HTTP/1.0\0") != 0 && strcmp(http_request[2], "HTTP/1.1\0") != 0
				&& strcmp(http_request[2], "HTTP/2.0\0") != 0 && strcmp(http_request[2], "HTTP/2\0") != 0) {
			httpError(buf, connfd, 505, http_request[2]);
			exit(EXIT_FAILURE);
		}

		// GET request from client
		if (strcmp(http_request[0], "GET\0") == 0) {
			/* URL PARSING */
			// parse url with port
			if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7) {
				sscanf(http_request[1], "http://%99[^:]:%6i/%99[^\n]", host, &port, page);
			}
			// parse url with no port specified, but has content request
			else if ((int)(strrchr(http_request[1], '/') - http_request[1]) > 7) {
				sscanf(http_request[1], "http://%99[^/]/%99[^\n]", host, page);
			}
			// parse url with no content request, but has port
			else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 7) {
				sscanf(http_request[1], "http://%99[^:]:%6i[^\n]", host, &port);
			}
			// just has regular host request (no port or content request)
			else if ((int)(strrchr(http_request[1], ':') - http_request[1]) > 1) {
				sscanf(http_request[1], "http://%99[^\n]", host);
			}
			// else an error, since unable to parse URL
			else {
				httpError(buf, connfd, 404, http_request[2]);
			}

			memset(&proxyaddr, 0, sizeof(proxyaddr));
			proxyaddr.sin_family = AF_INET;
			proxyaddr.sin_addr.s_addr = inet_addr(http_request[1]);
			proxyaddr.sin_port = htons((unsigned short)port);

			phe = gethostbyname(host);
			memcpy(&proxyaddr.sin_addr, phe->h_addr, phe->h_length);

			httpsocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			connect(httpsocket, (struct sockaddr *)&proxyaddr, proxylen);

			strcat(buf, http_request[0]);
			strcat(buf, page);
			strcat(buf, " ");
			strcat(buf, http_request[2]);
			strcat(buf, "\r\nHost: ");
			strcat(buf, host);
			strcat(buf, "\r\nProxy-Connection: keep-alive\r\n\r\n");
			if (send(httpsocket, &buf, strlen(buf), 0) > 0) {
				printf("Send sucessful on socket %i\n", connfd);
			} else httpError(buf, connfd, 404, http_request[2]); // sending data

			while ((n = recv(httpsocket, &buf, BUFSIZ, 0)) > 0) {
				buf[n] = '\0';
				send(connfd, &buf, n, 0);
			}
		} else httpError(buf, connfd, 400, http_request[2]); // request method
	} else httpError(buf, connfd, 500, http_request[2]); // socket issues

	memset(&buf[0], 0, sizeof(buf));
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
			strcat(buf, httpVersion);
			strcat(buf, " 400 Bad Request");
			break;
	}
	strcat(buf, "\n");
	send(connfd, buf, strlen(buf), 0);
}

/* Gracefully exit program (while loop) with Ctrl+C press */
void term(int signum){
	done = 1;
}