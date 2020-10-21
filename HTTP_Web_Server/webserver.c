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
#define TYPELENGTH 7
#define WWW_SERVER_PATH "/www"

volatile sig_atomic_t done = 0;

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
char *fileType(char *filename);
void error500(char buf[MAXLINE], int connfd);
char *contentType(char *ext);
void term(int signum);
void server_res(int n);
char *getcwd(char *buf, size_t size);

char *file_types[TYPELENGTH] = {"html", "txt", "png", "gif", "jpg", "css", "js"};
char *content_types[TYPELENGTH] = {"text/html", "text/plain", "image/png", "image/gif",
																	 "image/jpg", "text/css", "application/javascript"};

/*
 * main driver
 */
int main(int argc, char **argv) {
	int listenfd, *connfdp, port;
	struct sockaddr_in clientaddr;
	socklen_t clientlen;
	pthread_t tid;

	// check arguments
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

	// continuous listening of server
	listenfd = open_listenfd(port);
	while (!done) {
		connfdp = malloc(sizeof(int));
		*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		// printf("Connected to http://localhost:%i on socket %i\n", port, *connfdp);
		pthread_create(&tid, NULL, thread, connfdp);
	}
}

/*
 * requests send to server & server's response
 */
void server_res(int connfd) {
	size_t n;
	char buf[MAXLINE], httpmsg[MAXLINE], *http_request[3];
	int filedesc, socket_msg;

	// set working directory
	char cwd[MAXLINE];
	getcwd(cwd, sizeof(cwd));
	strcat(cwd, WWW_SERVER_PATH);

	// receive message from socket
	socket_msg = recv(connfd, httpmsg, MAXLINE, 0);

	// idle response
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
  timeinfo = localtime ( &rawtime );
	if (socket_msg == 0) {
		printf("Connection has idled at %s. Refresh page to continue.\n", asctime (timeinfo));
	}

	// connected
	else if (socket_msg > 0) {
		http_request[0] = strtok(httpmsg, " \t\n");
		if (strncmp(http_request[0], "GET\0", 4) == 0) {
			http_request[1] = strtok(NULL, " \t");
			http_request[2] = strtok(NULL, " \t\n");

			// default landing page/route, no file requested
			if (strncmp(http_request[1], "/\0", 2) == 0)
				http_request[1] = "/index.html";

			// make sure incoming file types are supported
			int valid = 1;
			for (int i = 0; i < TYPELENGTH; i++)
				if (strcmp(file_types[i], fileType(http_request[1])) == 0) {
					valid = 0;
					break;
				}
			if (valid) error500(buf, connfd);

			// get directory of file, with request URI
			strcpy(buf, cwd);
			strcpy(&buf[strlen(cwd)], http_request[1]);

			// system call, open file and read in descriptor
			if ((filedesc = open(buf, 0)) != -1) {
				// get content type for the header
				char *content_type = malloc(100);
				strcpy(content_type, "Content-Type:");
				strcat(content_type, contentType(fileType(http_request[1])));
				strcat(content_type, "\r\n");

				// set content length for the header
				int fsize = lseek(filedesc, 0, SEEK_END);
				lseek(filedesc, 0, SEEK_SET);
				char fsize_str[20];
				sprintf(fsize_str, "%d", fsize);
				char *content_length = malloc(100);
				strcpy(content_length, "Content-Length:");
				strcat(content_length, fsize_str);
				strcat(content_length, "\r\n");

				// send response
				send(connfd, "HTTP/1.1 200 Document Follows\r\n", 31, 0);
				send(connfd, content_type, strlen(content_type), 0);
				send(connfd, content_length, strlen(content_length), 0);
				send(connfd, "Connection: Keep-alive\r\n\r\n", 26, 0);
				while ((n = read(filedesc, buf, MAXBUF)) > 0)
					write(connfd, buf, n);

/** If any cases fail above, respond with Error 500 **/
			} else error500(buf, connfd);
		} else error500(buf, connfd);
	} else error500(buf, connfd);

	shutdown(connfd, 0);
	close(connfd);
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

/* Error 500 Internal Server Error handling */
void error500(char buf[MAXLINE], int connfd) {
	strcpy(buf, "HTTP/1.1 500 Internal Server Error");
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
}

void term(int signum){
	done = 1;
}

/* looking at the extension, and return supported content type*/
char *contentType(char *ext) {
	for (int i = 0; i < TYPELENGTH; i++)
		if (strcmp(ext, file_types[i]) == 0)
			return content_types[i];
	return "";
}