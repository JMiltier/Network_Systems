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
void error400(char buf[MAXLINE], int connfd);
void error500(char buf[MAXLINE], int connfd);
void error505(char buf[MAXLINE], int connfd);
void term(int signum);
void server_res(int n);
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

	// continuous listening of server
	listenfd = open_listenfd(port);
	while (!done) {
		connfdp = malloc(sizeof(int));
		*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
		// printf("Connected to http://localhost:%i on socket %i\n", port, *connfdp);
		pthread_create(&tid, NULL, thread, connfdp);
	}

	// once while loop exits, close sockets
	shutdown(*connfdp, 0);
	close(*connfdp);
}

/*
 * requests sent to web proxy & web proxy's response
 */
void proxy_res(int connfd) {
	size_t n;
	char buf[MAXLINE], httpmsg[MAXLINE], *http_request[3], address[100], page[100];
	int filedesc, socket_msg, filesize, port = 80;
	FILE file;

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
		printf("Connection has closed at time %s.\n", asctime (timeinfo));
	}
	// not so good
	else if (socket_msg < 0) {
		printf("Fatal socket error"); exit(EXIT_FAILURE);
	}

	// connected
	else if (socket_msg > 0) {
		// parse request
		http_request[0] = strtok(httpmsg, " \t\n");
		// GET request from client
		if (strncmp(http_request[0], "GET\0", 4) == 0) {
			http_request[1] = strtok(NULL, " \t"); // url
			http_request[2] = strtok(NULL, " \t\n"); // http method

			// parse url
			sscanf(http_request[1], "http://%99[^:]:%99d/%99[^\n]", address, &port, page);
			printf("address: %s\n", address);
			printf("port: %d\n", port);
			printf("page: %s\n", page);

			// get directory of file, with request URI
			strcpy(buf, cwd);
			strcpy(&buf[strlen(cwd)], http_request[1]);

			// system call, open file and read in descriptor
			if ((filedesc = open(buf, 0)) != -1) {
				// get content type for the header
				char *content_type = malloc(100);
				strcpy(content_type, "Content-Type:");
				/* using filetype index for supported content type */
				strcat(content_type, "\r\n");

				// set content length for the header
				filesize = lseek(filedesc, 0L, SEEK_END);
				lseek(filedesc, 0L, SEEK_SET);
				char filesize_string[20];
				sprintf(filesize_string, "%d", filesize);
				char *content_length = malloc(100);
				strcpy(content_length, "Content-Length:");
				strcat(content_length, filesize_string);
				strcat(content_length, "\r\n");

				// send response
				send(connfd, "HTTP/1.1 200 Document Follows\r\n", 31, 0);
				send(connfd, content_type, strlen(content_type), 0);
				send(connfd, content_length, strlen(content_length), 0);
				send(connfd, "Connection: Keep-alive\r\n\r\n", 26, 0);
				// write based on buffer size limit
				while ((n = read(filedesc, buf, MAXBUF)) > 0)
					write(connfd, buf, n);

/** If any cases fail above, respond with Error 500 **/
			} else error400(buf, connfd);
		} else error505(buf, connfd);
	} else error500(buf, connfd);
}

/* thread routine */
void *thread(void *vargp) {
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	proxy_res(connfd);
	free(vargp);
	// close(connfd);
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
// Error 400 errors
void error400(char buf[MAXLINE], int connfd) {
	strcpy(buf, "HTTP/1.1 400 Bad Request");
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
}
// Error 500 Internal Server Error handling
void error500(char buf[MAXLINE], int connfd) {
	strcpy(buf, "HTTP/1.1 500 Internal Server Error");
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
}
// Error 505 HTTP version error handling
void error505(char buf[MAXLINE], int connfd) {
	strcpy(buf, "HTTP/1.1 505 HTTP Version not supported");
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
}

/* Gracefully exit program (while loop) with Ctrl+C press */
void term(int signum){
	done = 1;
}