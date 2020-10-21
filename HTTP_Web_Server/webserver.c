/*
 * webserver.c - A concurrent TCP echo server using threads
 * using C code template from httpechosrv.c (same dir)
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
#include <fcntl.h> // for open
#include <pthread.h>
#include <signal.h> // to gracefully stop
#include <limits.h>

#define MAXLINE 8192 /* max text line length */
#define MAXBUF 8192	 /* max I/O buffer size */
#define LISTENQ 1024 /* second argument to listen() */
#define TYPELENGTH 8
#define WWW_SERVER_PATH "/www"

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
char *fileType(char *filename);
char *contentType(char *ext);
void inthand(int signum);
void server_res(int n);
char *getcwd(char *buf, size_t size);

char *file_types[TYPELENGTH] = {"html", "txt", "png", "gif", "jpg", "css", "js", "ico"};
char *content_types[TYPELENGTH] = {"text/html", "text/plain", "image/png", "image/gif",
																	 "image/jpg", "text/css", "application/javascript", "image/x-icon"};

int main(int argc, char **argv) {
	int listenfd, *connfdp, port;
	struct sockaddr_in clientaddr;
	socklen_t clientlen;
	pthread_t tid;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	port = atoi(argv[1]);

	// continuous listening of server
	listenfd = open_listenfd(port);
	while (1) {
		connfdp = malloc(sizeof(int));
		*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		pthread_create(&tid, NULL, thread, connfdp);
	}
}

void server_res(int connfd) {
	size_t n;
	char buf[MAXLINE], httpmsg[MAXLINE], *http_request[3];

	// set working directory
	char cwd[MAXLINE];
	getcwd(cwd, sizeof(cwd));
	strcat(cwd, WWW_SERVER_PATH);

	int request, fd, bytes_read;

	memset((void *)httpmsg, (int)'\0', MAXLINE);

	if (recv(connfd, httpmsg, MAXLINE, 0) > 0) {
		http_request[0] = strtok(httpmsg, " \t\n");
		http_request[1] = strtok(NULL, " \t");
		http_request[2] = strtok(NULL, " \t\n");

		// root directory if nothing else specified for localhost:<port>
		if (strncmp(http_request[1], "/\0", 2) == 0)
			http_request[1] = "/index.html";

				int type_found = 0;
				for (int i = 0; i < TYPELENGTH; i++)
				{
					if (strcmp(file_types[i], fileType(http_request[1])) == 0)
					{
						type_found = 1;
						break;
					}
				}

				strcpy(buf, cwd);
				strcpy(&buf[strlen(cwd)], http_request[1]); //creating an absolute filepath

				/* 400 Error */
				if (strlen(buf) > 255) {
					printf("MADE IT HERE! BAD\n");
					char *invalid_uri_str;
					invalid_uri_str = malloc(80);
					strcpy(invalid_uri_str, "HTTP/1.1 400 Bad Request: Invalid URI: ");
					strcat(invalid_uri_str, buf);
					strcat(invalid_uri_str, "\n");

					int invalid_uri_strlen = strlen(invalid_uri_str);
					write(connfd, invalid_uri_str, invalid_uri_strlen);
					//free(invalid_uri_str);
				} else {
					if ((fd = open(buf, O_RDONLY)) != -1) {
						char *content_type_header;
						content_type_header = malloc(50);
						strcpy(content_type_header, "Content-Type: ");
						strcat(content_type_header, contentType(fileType(http_request[1])));
						strcat(content_type_header, "\n");
						int content_type_header_len = strlen(content_type_header);

						int fsize = lseek(fd, 0, SEEK_END);
						lseek(fd, 0, SEEK_SET);
						char fsize_str[20];
						sprintf(fsize_str, "%d", fsize);
						char *content_length_header;
						content_length_header = malloc(50);
						strcpy(content_length_header, "Content-Length: ");
						strcat(content_length_header, fsize_str);
						strcat(content_length_header, "\n");
						int content_length_header_len = strlen(content_length_header);

						send(connfd, "HTTP/1.1 200 OK\n", 16, 0);
						send(connfd, content_type_header, content_type_header_len, 0);
						send(connfd, content_length_header, content_length_header_len, 0);
						send(connfd, "Connection: keep-alive\n\n", 24, 0);
						while ((bytes_read = read(fd, buf, MAXBUF)) > 0)
							write(connfd, buf, bytes_read);
					}

					/* 404 Error */
					else {
						printf("MADE IT HERE! BAD\n");
						char *not_found_str;
						not_found_str = malloc(80);
						strcpy(not_found_str, "HTTP/1.1 404 Not Found: ");
						strcat(not_found_str, buf);
						strcat(not_found_str, "\n");

						int not_found_strlen = strlen(not_found_str);
						write(connfd, not_found_str, not_found_strlen);
					}

		}
	}

	shutdown(connfd, SHUT_RDWR);
	close(connfd);
	connfd = -1;
}

/* thread routine */
void *thread(void *vargp) {
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	free(vargp);
	server_res(connfd);
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
	if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
		return -1;

	/* Make it a listening socket ready to accept connection requests */
	if (listen(listenfd, LISTENQ) < 0) {
		perror("listenfd, listen fn\n");
		return -1;
	}

	return listenfd;
} /* end open_listenfd */

/* get file type (extension) of file */
char *fileType(char *file) {
	char *ext = strrchr(file, '.');
	if (!ext) return "";
	else return ext + 1;
}

/* looking at the extension, and return supported content type*/
char *contentType(char *ext)
{
	for (int i = 0; i < TYPELENGTH; i++)
	{
		if (strcmp(ext, file_types[i]) == 0)
		{
			return content_types[i];
		}
	}
	return "";
}