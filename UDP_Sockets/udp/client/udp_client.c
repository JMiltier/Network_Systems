/* ***********************************
 * udp_client.c - A simple UDP client
 * usage: ./udp_client <host> <port>
 *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024

//! error - wrapper for perror
void error(char *msg) {
  perror(msg);
  exit(0);
}

int main(int argc, char **argv) {
  int sockfd, portno, n;
  int serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];
  char line[40], cmd[10], filename[30];

  // check command line arguments
  if (argc != 3) {
      fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
      exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  // socket: create the socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
      error("ERROR opening socket");

  // gethostbyname: get the server's DNS entry
  server = gethostbyname(hostname);
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host as %s\n", hostname);
      exit(0);
  }

  // build the server's Internet address
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  while (1) {
    // make sure input strings are empty each time
    line[0] = '\0';
    cmd[0] = '\0';
    filename[0] = '\0';

    // print command options
    printf("\nPlease enter a command: \n  get <filename>\n"
          "  put <filename>\n  delete <filename>\n  ls\n  exit\n\n> ");
    scanf("%[^\n]%*c", line); // get line, ignoring the newline <enter>
    sscanf(line, "%s %s", cmd, filename);

    printf("--> %s %s", cmd, filename);

    if (!strcmp(cmd, "get")) {

    } else if (!strcmp(cmd, "put")) {

    } else if (!strcmp(cmd, "delete")) {

    } else if (!strcmp(cmd, "ls")) {

    } else if (!strcmp(cmd, "exit")) {
      printf("Goodbye!\n\n");
      exit(EXIT_SUCCESS);
    } else {
      line[40] = cmd + filename[30]; // concat, to also have a space (since concat() won't include the space)
      printf("\n'%s' is not a valid command. Please try again.\n", line);
    }
  }
  // get a message from the user
  bzero(buf, BUFSIZE);
  printf("Please enter msg: ");
  fgets(buf, BUFSIZE, stdin);

  // send the message to the server
  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
  if (n < 0)
    error("ERROR in sendto");

  // print the server's reply
  n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
  if (n < 0)
    error("ERROR in recvfrom");
  printf("Echo from server: %s", buf);
  return 0;
}



/* CODE THAT DIDN'T WORK

switch (cmd[0]) {
      case 'g':
        printf("\n%*c %*c is not a valid command. 1 Please try again.\n", cmd, filename);
        break;
      case 'p':
        printf("\n%s %s is not a valid command. 2 Please try again.\n", cmd, filename);
        break;
      case 'd':
        printf("\n%s %s is not a valid command. 3 Please try again.\n", cmd, filename);
        break;
      case 'l':
        printf("\n%s %s is not a valid command. 4 Please try again.\n", cmd, filename);
        break;
      case 'e':
        printf("Goodbye!");
        exit(EXIT_SUCCESS);
        break;
      default:
        printf("\n'%s %s' is not a valid command. Please try again.\n", cmd, filename);
    }

*/