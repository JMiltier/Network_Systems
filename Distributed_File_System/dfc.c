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

#define BUFSIZE 1024
#define EXIT_FAILURE exit(-1)

void error(char *msg);

int main(int argc, char **argv) {
  int sockfd, portno, n, rcv;
  int serverlen;
  struct sockaddr_in serveraddr[4]; // server sockets (now have 4)
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];
  char cmd[10], *config_file, ch, filename[30];
  char DFS[4][1], S_IP[4][16], S_PORT[4][6], user[30], u[30], pass[30], p[30];

  // check command line arguments
  if (argc != 2) { fprintf(stderr,"usage: %s <config_file>\n", argv[0]); EXIT_FAILURE; }
  config_file = argv[1];

  // Read in config file, and parse
  FILE *file = fopen(config_file, "r");
  char *line = NULL;
  size_t len = 0, cnt = 0;
  ssize_t read;
  if (file == NULL) { printf("Configuration file not found.\n"); EXIT_FAILURE; }
  while ((read = getline(&line, &len, file)) != -1) {
    if (strncmp(line, "Server", 6) == 0) {
      sscanf(line, "Server DFS%s %9s:%s[^\n]", DFS[cnt], S_IP[cnt], S_PORT[cnt]);
      // printf("d:%s i:%s p:%s\n", DFS[cnt], S_IP[cnt], S_PORT[cnt]);
      cnt++;
    }
    else if (strncmp(line, "Username", 8) == 0) sscanf(line, "Username: %s[^\n]", user);
    else if (strncmp(line, "Password", 8) == 0) sscanf(line, "Password: %s[^\n]", pass);
    else { printf("Bad configuration file.\n"); EXIT_FAILURE; }
  }
  printf("Configuration file '%s' successfully parsed.\n", config_file);
  if (line) free(line);
  if (fclose(file) != 0) printf("Not able to close file '%s'.\n", config_file);

  // socket: create the socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
      error("ERROR opening socket");

  // gethostbyname: get the server's DNS entry
  server = gethostbyname(S_IP[0]);
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host as %s\n", S_IP[0]);
      exit(0);
  }

  // build the server's internet addresses
  for (int i=0; i < 4; i++) {
    bzero((char *)&serveraddr[i], sizeof(&serveraddr[i]));
    serveraddr[i].sin_family = AF_INET;
    server = gethostbyname(S_IP[i]);
    portno = atoi(S_PORT[i]);
    bcopy((char *)server->h_addr,
    (char *)&serveraddr[i].sin_addr.s_addr, server->h_length);
    serveraddr[i].sin_port = htons(portno);

    // send the message to the server once connected
    serverlen = sizeof(serveraddr[i]);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr[i], serverlen);
    if (n < 0)
      error("ERROR in sendto");
  }

  // system("clear"); // clean console when initiated :)

  while (1) {
    // make sure input strings are empty each time
    buf[0] = '\0';
    cmd[0] = '\0';
    filename[0] = '\0';

    // printf("Connected to server @ %s:%i\n\n", hostname, portno);

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

    // print command options
    printf("Please enter a command:"
           "\n  list"
           "\n  get <filename>"
           "\n  put <filename>"
           "\n  exit"
           "\n\n> ");

    scanf(" %[^\n]%*c", buf); // get line, ignoring the newline <enter> and empty <enter>
    sscanf(buf, "%s %s", cmd, filename);
    system("clear"); // clean console :)

    // send the message to the server
    serverlen = sizeof(serveraddr);
    // n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    // if (n < 0)
    //   error("ERROR in sendto");

    /******************************** ls functionality ********************************/
    if (!strcmp(cmd, "list")) {
      char file_list[BUFSIZE];
      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      recvfrom(sockfd, file_list, sizeof(file_list), 0, &serveraddr, &serverlen);
      printf("%s\n", file_list);

    /******************************** get functionality ********************************/
    } else if (!strcmp(cmd, "get")) {
      char fn[BUFSIZE];

      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      recvfrom(sockfd, &(fn), sizeof(fn), 0, &serveraddr, &serverlen);

      // file is on server
      if (fn[0] != '\0'){
      FILE *file = fopen(filename, "wb");
      fwrite(fn, 1, strlen(fn), file);
      printf("File '%s' copied in.\n", filename);
      fclose(file);

      // file does not exist on server
      } else printf("Unable to get file '%s'. Try again.\n", filename);

    /******************************** put functionality ********************************/
    } else if (!strcmp(cmd, "put")) {
      char data[BUFSIZE];
      // first, check if filename exists
      if (access(filename, F_OK) != -1) {
        // open file to send
        FILE *file = fopen(filename, "rb");
        // fseek(file, 0L, SEEK_END);
        // determine if file size is bigger than buffer size
        // long int file_size = ftell(file);
        // printf("filesize %i\n", file_size);
        // int packets = ceil(file_size / BUFSIZE);
        // printf("packets %i\n", packets);
        fread(data, strlen(file)+1, BUFSIZE, file);

        // printf("data %s\n", data);
        sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        fclose(file);
        printf("File '%s' sent.\n", filename);
      } else {
        printf("Unable to send file. File '%s' does not exist. Try again.\n", filename);
      }

    /******************************** ls functionality ********************************/
    } else if (!strcmp(cmd, "ls")) {
      char file_list[BUFSIZE];
      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      recvfrom(sockfd, file_list, sizeof(file_list), 0, &serveraddr, &serverlen);
      printf("%s\n", file_list);

    /******************************** exit functionality ********************************/
    } else if (!strcmp(cmd, "exit")) {
      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      printf("Goodbye!\n\n");
      exit(0);

    /************************* when user input doesn't match given commands *************************/
    } else {
      // concat, to also have a space (since concat() won't include the space)
      buf[BUFSIZE] = cmd + filename[30];
      printf("ERROR: '%s' is not a valid command. Please try again.\n\n", buf);
    }
  }
}

//! error - wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(0);
}


/* good
blue
green
*/