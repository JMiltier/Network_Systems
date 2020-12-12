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
#include <dirent.h>      /* for directory */
#include <fcntl.h>       /* for open */
#include <sys/types.h>   /* for open (UNIX)*/
#include <sys/stat.h>    /* for open (UNIX)*/
#include <signal.h>      /* to gracefully stop */
#include <limits.h>
#include <time.h>        /* for time keeping */

#define BUFSIZE     4096
#define MAXLINE     8192           /* max text line length */
#define MAXBUF      8192           /* max I/O buffer size */
#define LISTENQ     1024           /* second argument to listen() */
#define TYPELENGTH  7              /* number of file types */
#define CONF_FILE   "dfs.conf"     /* server config file */
#define USERS       2              /* static number of users */

int login_auth = 0; // login tracker
char server_name[10]; // file directory
char username[USERS][30], password[USERS][30];
char *file_types[TYPELENGTH] = {"html", "txt", "png", "gif", "jpg", "css", "js"};
char *content_types[TYPELENGTH] = {"text/html", "text/plain", "image/png", "image/gif",
                                   "image/jpg", "text/css", "application/javascript"};

volatile sig_atomic_t done = 0;

/* function references to top */
int open_listenfd(int port);
void *thread(void *vargp);
void parse_config_file();
void error500(char buf[MAXLINE], int connfd);
void term(int signum);
void server_res(int n);
char *getcwd(char *buf, size_t size);
void error(char *msg);

int main(int argc, char **argv) {
  socklen_t clientlen; // byte size of client's address
  struct sockaddr_in clientaddr; // client addr
  char buf[BUFSIZE]; // message buf
  int optval; // flag value for setsockopt
  int n, snd; // message byte size
  struct stat st = {0}; // for creating user file structures
  char cmd_in[10], filename_in[30]; // incoming command and filename
  int listenfd, *connfdp, port;
  pthread_t tid;

  // check command line arguments
  if (argc != 3) {
    fprintf(stderr, "usage: %s <server> <port>\n", argv[0]);
    exit(1);
  }
  strcpy(server_name, argv[1]);
  port = atoi(argv[2]);

  // create the server folder if it doesn't exist
  char cwd[BUFSIZE];
  getcwd(cwd, sizeof(cwd));
	strcat(cwd, "/");
	strcat(cwd, server_name);
  if (stat(cwd, &st) == -1)
      mkdir(cwd, 0700);

  // gracefully exit
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = term;
  sigaction(SIGINT, &action, NULL);

  // read configuration file (usernames, passwords)
  parse_config_file();

  // just to help clarify
  // system("clear"); // clean console when initiated :)
  printf("Server listening on port %i.\n", port);
  printf("Graceful exit: escape character is 'Ctrl+C'.\n");

  // continuous listening of server
  if ((listenfd = open_listenfd(port)) < 0)
    printf("Failure to open listening port.\n");

  // main loop: wait for a datagram, then echo it
  clientlen = sizeof(clientaddr);

  while (!done) {
    connfdp = malloc(sizeof(int));
    if((*connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0)
      printf("accept failure\n");
    // printf("Connected to http://localhost:%i on socket %i\n", port, *connfdp);
    pthread_create(&tid, NULL, thread, connfdp);
  }
  // once while loop gracefully exits, close connections
  shutdown(*connfdp, 0);
  close(*connfdp);
  close(listenfd);
}


/*
 * requests sent to server & server's response
 */
void server_res(int connfd) {
  size_t n;
  char buf[BUFSIZE], auth[MAXLINE], *filetype;
  char cmd_in[10], filename_in[30]; // incoming command and filename
  char user_in[30], pass_in[30];
  int filedesc, socket_msg, filesize, filetype_index;
  struct sockaddr_in clientaddr; // client addr
  socklen_t clientlen; // byte size of client's address
  struct stat st = {0}; // for creating user file structures
  FILE file;

  // set working directory
	char cwd[MAXLINE];
	getcwd(cwd, sizeof(cwd));
  if (strncmp(server_name, "/", 1) != 0)
    strcat(cwd, "/");
	strcat(cwd, server_name);

  // receive message from socket
  socket_msg = recv(connfd, auth, MAXLINE, 0);

  // receive and validate user; add user folder if validated
  sscanf(auth, "%s %s[^\n]", user_in, pass_in);
  if (strcmp(username[0], user_in) == 0 && strcmp(password[0], pass_in) == 0) {
    printf("** %s authenticated. **\n", user_in);
    login_auth = 1;
    strcat(cwd, "/");
    strcat(cwd, user_in);
    if (stat(cwd, &st) == -1)
      mkdir(cwd, 0700);
    write(connfd, "authed", 6);
  } else printf("Invalid user.\n");

  // idle response
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  if (socket_msg == 0)
    printf("Connection has idled at %s. Refresh page to continue.\n", asctime (timeinfo));
  else if (socket_msg < 0)
    printf("Socket error.\n");

  // connected
  else if (socket_msg > 0) {
    printf("** Client connected. **\n");
    buf[0] = '\0';
    cmd_in[0] = '\0';
    filename_in[0] = '\0';

    // recvfrom: receive a UDP datagram from a client
    bzero(buf, BUFSIZE);
    n = recvfrom(connfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
    write(connfd, "ready", 5);
    sscanf(buf, "%s %s", cmd_in, filename_in);

    // format the filename_in to point to right spot
    if (strncmp(filename_in, "/", 1) != 0) strcat(cwd, "/");
    strcat(cwd, filename_in);

    // user has authenticated
    if (login_auth == 1) {
      // format for server
      /* ******* list command handling ******* */
      if (!strcmp(cmd_in, "list")) {
        struct dirent **namelist;
        n = scandir(cwd, &namelist, NULL, alphasort);
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
        // return file list to client
        // sendto(connfd, files, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
        write(connfd, files, BUFSIZE);

      /* ******* get command handling ******* */
      } else if (!strcmp(cmd_in, "get")) {
        char data[BUFSIZE];
        if (access(cwd, F_OK) != -1) {
          // open file to send
          FILE *file = fopen(cwd, "rb");
          fseek(file, 0L, SEEK_END);
          long int file_size = ftell(file);
          fseek(file, 0, SEEK_SET);
          fread(data, BUFSIZE, file_size, file);
          fclose(file);
          write(connfd, &(data), file_size);
        } else {
          data[0] = '\0';
          write(connfd, &(data), 0);
        }

      // /* ******* put command handling ******* */
      } else if (!strcmp(cmd_in, "put")) {
        char data[BUFSIZE];
        read(connfd, data, BUFSIZE);
        FILE *file = fopen(cwd, "wb");
        fwrite(data, sizeof(char), strlen(data), file);
        fclose(file);

      /* ******* exit command handling ******* */
      } else if (!strcmp(cmd_in,"exit")) {
        printf("Client @ %d exited.\n", connfd);
        shutdown(connfd, 0);
        close(connfd);
        // exit(0); // probably shouldn't allow from client side

      /* ******* trash command handling ******* */
      } else printf("Invalid command from client.\n");
    } else write(connfd, "Not authorized.", 0);
  }
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
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    error("listenfd, error opening socket\n");
    return -1;
  }

  /* Eliminates "Address already in use" error from bind. */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
    error("setsockopt fn\n");
    return -1;
  }

  /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)(port));
  if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    error("unable to bind socket\n");
    return -1;
  }

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) {
    error("listenfd, listen fn\n");
    return -1;
  }

  return listenfd;
} /* end open_listenfd */

/* parse username and password from config file */
void parse_config_file() {
  FILE *file = fopen(CONF_FILE, "r");
  char *line = NULL;
  size_t len = 0, cnt = 0;
  ssize_t read;
  if (file == NULL) { printf("Configuration file not found.\n"); EXIT_FAILURE; }
  while ((read = getline(&line, &len, file)) != -1) {
    if(sscanf(line, "%s %s[^\n]", username[cnt], password[cnt]))
      cnt++;
    else { printf("Bad configuration file.\n"); EXIT_FAILURE; }
  }
  // printf("Configuration file '%s' successfully parsed.\n", CONF_FILE);
  if (line) free(line);
  if (fclose(file) != 0) printf("Not able to close file '%s'.\n", CONF_FILE);
  return;
}

/* Error 500 Internal Server Error handling */
void error500(char buf[MAXLINE], int connfd) {
  strcpy(buf, "HTTP/1.1 500 Internal Server Error");
  strcat(buf, "\n");
  write(connfd, buf, strlen(buf));
}

//! error - wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(1);
}

/* Gracefully exit program (while loop) */
void term(int signum){
  done = 1;
}