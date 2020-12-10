/* ***********************************
 * dfs.c - A Distributed File System Server
 * usage: ./dfs <port>
 * Program by Josh Miltier
 *********************************** */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h> // for directory
#include <fcntl.h> 			/* for open */
#include <sys/types.h>  /* for open (UNIX)*/
#include <sys/stat.h>		/* for open (UNIX)*/
#include <signal.h> 		/* to gracefully stop */
#include <limits.h>
#include <time.h> 			/* for time keeping */

#define BUFSIZE  1024
#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

volatile sig_atomic_t done = 0;

//! error - wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; // socket
  int portno; // port to listen on
  int clientlen; // byte size of client's address
  struct sockaddr_in serveraddr; // server's addr
  struct sockaddr_in clientaddr; // client addr
  struct hostent *hostp; // client host info
  char buf[BUFSIZE]; // message buf
  char *hostaddrp; // dotted decimal host addr string
  int optval; // flag value for setsockopt
  int n, snd; // message byte size
  char cmd_in[10], filename_in[30]; // incoming command and filename
  int login = 0; // login tracker

  // check command line arguments
  if (argc != 3) {
    fprintf(stderr, "usage: %s <server> <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  // socket: create the parent socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  // build the server's Internet address
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  // bind: associate the parent socket with a port
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  // main loop: wait for a datagram, then echo it
  clientlen = sizeof(clientaddr);

  // just to help clarify
  system("clear"); // clean console when initiated :)
  printf("Server listening on port %i.\n", portno);

  while (1) {
    // make sure input strings are empty each time
    buf[0] = '\0';
    cmd_in[0] = '\0';
    filename_in[0] = '\0';

    // recvfrom: receive a UDP datagram from a client
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    // format for server
    sscanf(buf, "%s %s", cmd_in, filename_in);

    // gethostbyaddr: determine who sent the datagram
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);

    // see who is logged in
    if (!login) {
      printf("Client @ %s joined.\n", hostaddrp);
      login = 1;
    }

    /************************* get command handling *************************/
    if (!strcmp(cmd_in, "get")) {
      char data[BUFSIZE];

      if (access(filename_in, F_OK) != -1) {
        // open file to send
        FILE *file = fopen(filename_in, "rb");
        fseek(file, 0L, SEEK_END);
        long int file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        fread(data, strlen(file)+1, file_size, file);
        fclose(file);
        sendto(sockfd, &(data), file_size, 0, (struct sockaddr *) &clientaddr, clientlen);
      } else {
        data[0] = '\0';
        sendto(sockfd, &(data), 0, 0, (struct sockaddr *) &clientaddr, clientlen);
      }

    /************************* put command handling *************************/
    } else if (!strcmp(cmd_in, "put")) {
      FILE *file = fopen(filename_in, "wb");
      fwrite(&file, 1, strlen(file), file);
      fclose(file);

    /************************* ls command handling *************************/
    } else if (!strcmp(cmd_in, "ls")) {
      struct dirent **namelist;
      n = scandir(".", &namelist, NULL, alphasort);
      char file_list[BUFSIZE];
      char files[BUFSIZE] = "";
      int i = 2;
      while (i < n) {
        sprintf(file_list, "%s\n", namelist[i]->d_name);
        free(namelist[i]);
        strcat(files, file_list);
        i++;
      }
      free(namelist);

      sendto(sockfd, files, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);

    /************************* exit command handling *************************/
    } else if (!strcmp(cmd_in,"exit")) {
      printf("Client @ %s exited.\n", hostaddrp);
      login = 0;
      // exit(0); // probably shouldn't allow from client side

    /************************* trash command handling *************************/
    } else {
      // what to do?
      continue;
    }
  }
}






int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv) {
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp) {
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}


#define MAXLINE 8192 /* max text line length */
#define MAXBUF 8192	 /* max I/O buffer size */
#define LISTENQ 1024 /* second argument to listen() */
#define TYPELENGTH 7
#define WWW_SERVER_PATH "/www"


int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
void error500(char buf[MAXLINE], int connfd);
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
 * requests sent to server & server's response
 */
void server_res(int connfd) {
	size_t n;
	char buf[MAXLINE], httpmsg[MAXLINE], *http_request[3], *filetype;
	int filedesc, socket_msg, filesize, filetype_index;
	FILE file;

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

			// for specific files, get file type (extension)
			char *ext = strrchr(http_request[1], '.');
			if (!ext) filetype = "";
			else filetype = ext + 1;

			// make sure incoming file types are supported
			for (int i = 0; i < TYPELENGTH; i++)
				if (strcmp(file_types[i], filetype) == 0) {
					filetype_index = i;
					break;
				}

			// get directory of file, with request URI
			strcpy(buf, cwd);
			strcpy(&buf[strlen(cwd)], http_request[1]);

			// system call, open file and read in descriptor
			if ((filedesc = open(buf, 0)) != -1) {
				// get content type for the header
				char *content_type = malloc(100);
				strcpy(content_type, "Content-Type:");
				/* using filetype index for supported content type */
				strcat(content_type, content_types[filetype_index]);
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

/* Error 500 Internal Server Error handling */
void error500(char buf[MAXLINE], int connfd) {
	strcpy(buf, "HTTP/1.1 500 Internal Server Error");
	strcat(buf, "\n");
	write(connfd, buf, strlen(buf));
}

/* Gracefully exit program (while loop) */
void term(int signum){
	done = 1;
}

