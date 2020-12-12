/* ***************************************
 * dfc.c - distributed file system client
 * usage: ./dfc dfc.conf  *!loads config
 * Program by Josh Miltier
 ***************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // strncmp
#include <string.h>
#include <unistd.h> // access
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <sys/types.h>   /* for open (UNIX)*/
#include <sys/stat.h>    /* for open (UNIX)*/
#include <signal.h> 		/* to gracefully stop */
// #include <openssl/md5.h> /* MD5 for Linux */
#include <CommonCrypto/CommonDigest.h> /* MD5 for macOS */

#define BUFSIZE 4096
#define SVRS 4

volatile sig_atomic_t done = 0;

void term(int signum);
void error(char *msg);

int main(int argc, char **argv) {
  int sockfd[SVRS], MD5HASH = 0, user_auth = 0;
  long int file_size;
  struct sockaddr_in serveraddr[SVRS]; // server sockets (now have 4)
  struct stat st = {0}; // for creating user file structures
  // struct hostent *server[SVRS];
  char buf[BUFSIZE], cmd[10], filename[50], *config_file, spin_wait[10];
  char DFS[SVRS][1], S_IP[SVRS][16], S_PORT[SVRS][6], user[30], pass[30];
  unsigned char digest[CC_MD5_DIGEST_LENGTH];
  CC_MD5_CTX context;
  CC_MD5_Init(&context);
  int CC_MD5_Table[SVRS][SVRS*2] =
    {{1,2,2,3,3,4,4,1},
     {4,1,1,2,2,3,3,4},
     {3,4,4,1,1,2,2,3},
     {2,3,3,4,4,1,1,2}};

  // check command line arguments
  if (argc != 2) { fprintf(stderr,"usage: %s <config_file>\n", argv[0]); EXIT_FAILURE; }
  config_file = argv[1];

  // gracefully exit
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGINT, &action, NULL);

  // Read in config file, and parse
  FILE *file = fopen(config_file, "r");
  char *line = NULL;
  size_t len = 0, cnt = 0;
  ssize_t read_in;
  if (file == NULL) { printf("Configuration file not found.\n"); EXIT_FAILURE; }
  while ((read_in = getline(&line, &len, file)) != -1) {
    if (strncmp(line, "Server", 6) == 0) {
      sscanf(line, "Server DFS%s %9s:%s[^\n]", DFS[cnt], S_IP[cnt], S_PORT[cnt]);
      // printf("d:%s i:%s p:%s\n", DFS[cnt], S_IP[cnt], S_PORT[cnt]);
      cnt++;
    }
    else if (strncmp(line, "Username", 8) == 0) sscanf(line, "Username: %s[^\n]", user);
    else if (strncmp(line, "Password", 8) == 0) sscanf(line, "Password: %s[^\n]", pass);
    else { printf("Bad configuration file.\n"); EXIT_FAILURE; }
  }
  // printf("Configuration file '%s' successfully parsed.\n", config_file);
  if (line) free(line);
  if (fclose(file) != 0) printf("Not able to close file '%s'.\n", config_file);

   // set working directory, and create a folder to store files
  char cwd[BUFSIZE];
  getcwd(cwd, sizeof(cwd));
	strcat(cwd, "/");
	strcat(cwd, user);
  if (stat(cwd, &st) == -1)
      mkdir(cwd, 0700);

  for (int i=0; i < SVRS; i++) {
    // build the server's Internet addresses
    bzero(&serveraddr[i], sizeof(serveraddr[i]));
    serveraddr[i].sin_family = AF_INET;
    serveraddr[i].sin_port = htons(atoi(S_PORT[i]));
    serveraddr[i].sin_addr.s_addr = inet_addr(S_IP[i]);

    // socket: create the socket {SOCK_DGRAM}
    sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd[i] < 0)
        error("erro opening socket");

    // attempt to connect to server, timeout after 1 second
    struct timeval timeout;
    timeout.tv_sec = 1;
    if (setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
      error("setsockopt failed");
    if (connect(sockfd[i], (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i])) < 0)
      error("server connection error");

    // validate user on servers
    buf[0] = '\0';
    strcpy(buf, user);
    strcat(buf, " ");
    strcat(buf, pass);
    if ((sendto(sockfd[i], buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i]))) < 0)
      error("Server authentication");
    else {
      buf[0] = '\0';
      read(sockfd[i], buf, BUFSIZE);
      if (strcmp(buf, "auth") > 0)
        user_auth = 1;
    }
  }

  if (!user_auth)
    error("Failed to authenicate with server");

  while (!done && user_auth) {
    // make sure input strings are empty each time
    buf[0] = '\0';
    cmd[0] = '\0';
    filename[0] = '\0';

    // print command options
    printf("Please enter a command:"
           "\n  list"
           "\n  get <filename>"
           "\n  put <filename>"
           "\n  exit"
           "\n\n> ");

    scanf(" %[^\n]%*c", buf); // get line, ignoring the newline <enter> and empty <enter>
    sscanf(buf, "%s %s", cmd, filename); // assign command and filename

    // add filename to cwd
    if (strncmp(filename, "/", 1) != 0) strcat(cwd, "/");
    strcat(cwd, filename);

    /* ************** list command handling ************** */
    if (!strcmp(cmd, "list")) {
      for (int i=0; i < SVRS; i++) {
        char file_list[BUFSIZE];
        // send cmd/filename to each server, and spin wait for response before continuing
        sendto(sockfd[i], buf, strlen(buf), 0, (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i]));
        spin_wait[0] = '\0'; read(sockfd[i], spin_wait, 10);
        while(strcmp(spin_wait, "ready") < 0) {};
        // recvfrom(sockfd[0], file_list, sizeof(file_list), 0, (struct sockaddr *)&serveraddr[0], (socklen_t *)sizeof(&serveraddr[0]));
        read(sockfd[i], file_list, sizeof(file_list));
        printf("%s", file_list);
      }
      done = 1;
    /* ************** get command handling ************** */
    } else if (!strcmp(cmd, "get")) {
      char fn[BUFSIZE];
      for (int i=0; i < SVRS; i++) {
        // send cmd/filename to each server, and spin wait for response before continuing
        sendto(sockfd[i], buf, strlen(buf), 0, (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i]));
        spin_wait[0] = '\0'; read(sockfd[i], spin_wait, 10);
        while(strcmp(spin_wait, "ready") < 0) {};
        read(sockfd[i], &(fn), sizeof(fn));
        // file is on server
        if (fn[0] != '\0'){
          FILE *file = fopen(cwd, "wb");
          fwrite(fn, 1, strlen(fn), file);
          printf("File '%s' copied in from %s:%s.\n", filename, S_IP[i], S_PORT[i]);
          fclose(file);
        // file does not exist on server
        } else printf("Unable to get file '%s' from %s:%s. Try again.\n", filename, S_IP[i], S_PORT[i]);
        // reset fn for each read
        fn[0] = '\0';
      }
      done = 1;

    /* ************** put command handling ************** */
    } else if (!strcmp(cmd, "put")) {
      char data[BUFSIZE];
      // first, check if file exists
      if (access(cwd, F_OK) != -1) {
        // open file to send
        FILE *file = fopen(cwd, "rb");
        fseek(file, 0L, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0L, SEEK_SET);
        printf("filesize %li\n", file_size);
        int packets = ceil(file_size / BUFSIZE) + 1;
        printf("packets %i\n", packets);
        while ((read_in = getline(&line, &len, file)) != -1) {
          CC_MD5_Update(&context, line, (CC_LONG)len);
        }
        CC_MD5_Final(digest, &context);
        char str[10];
        for (size_t i=0; i<CC_MD5_DIGEST_LENGTH; ++i) {
          // printf("%i\n", digest[i]);
          snprintf(str, 10, "%d", digest[i]);
          MD5HASH += atoi(str);
        }
        // printf("hash mod: %i\n", MD5HASH%4);
        // fread(data, file_size, BUFSIZE, file);
        for(int i=0; i < SVRS; i++) {
          // send cmd/filename to each server, and spin wait for response before continuing
          sendto(sockfd[i], buf, strlen(buf), 0, (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i]));
          spin_wait[0] = '\0'; read(sockfd[i], spin_wait, 10);
          while(strcmp(spin_wait, "ready") < 0) {};
          // read in file contents and send to client
          fseek(file, 0L, SEEK_SET);
          data[0] = '\0'; len = 0;
          // write the data of the file to DFS
          while((len = fread(data, 1, sizeof(data), file)) > 0) {
            printf("test stuff %s\n", data);
            write(sockfd[i], data, len);
          }
          printf("File '%s' sent to %s:%s.\n", filename, S_IP[i], S_PORT[i]);
        }
        fclose(file);
      } else {
        printf("Unable to send file. File '%s' does not exist. Try again.\n", filename);
      } done = 1;

    /* ************** exit command handling ************** */
    } else if (!strcmp(cmd, "exit")) {
      for (int i=0; i < SVRS; i++) {
        // send cmd/filename to each server, and spin wait for response before continuing
        sendto(sockfd[i], buf, strlen(buf), 0, (struct sockaddr *)&serveraddr[i], sizeof(serveraddr[i]));
        spin_wait[0] = '\0'; read(sockfd[i], spin_wait, 10);
        while(strcmp(spin_wait, "ready") < 0) {};
      }
      printf("Goodbye!\n\n");
      done = 1;

    /* ************** garbage command handling ************** */
    } else {
      // concat, to also have a space (since concat() won't include the space)
      *buf = *cmd + *filename;
      printf("ERROR: '%s' is not a valid command. Please try again.\n\n", buf);
    }
  }
}

//? Gracefully exit program (while loop) with Ctrl+C press
void term(int signum) {
	done = 1;
}

//! error - wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(0);
}


// some manual validation
// have user validate
// printf("Please login to server:"
//        "\n Username: ");
// scanf(" %[^\n]%*c", buf); // get line, ignoring the newline <enter> and empty <enter>
// sscanf(buf,"%s", u);
// if (strcmp(u, user) != 0) { printf("✗  Username not found.\n"); EXIT_FAILURE; }
// printf(" Password: ");
// scanf(" %[^\n]%*c", buf); // get line, ignoring the newline <enter> and empty <enter>
// sscanf(buf,"%s", p);
// if (strcmp(p, pass) != 0) { printf("✗  Password incorrect.\n"); EXIT_FAILURE; }