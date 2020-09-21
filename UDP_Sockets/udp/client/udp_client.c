/* ***********************************
 * udp_client.c - A simple UDP client
 * usage: ./udp_client <host> <port>
 * Program by Josh Miltier
 *********************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // access
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
  int sockfd, portno, n, rcv;
  int serverlen;
  struct sockaddr_in serveraddr;
  struct hostent *server;
  char *hostname;
  char buf[BUFSIZE];
  char cmd[10], filename[30], ch;

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

  system("clear"); // clean console when initiated :)

   // send the message to the server once connected
  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
  if (n < 0)
    error("ERROR in sendto");

  while (1) {
    // make sure input strings are empty each time
    buf[0] = '\0';
    cmd[0] = '\0';
    filename[0] = '\0';

    // printf("Connected to server @ %s:%i\n\n", hostname, portno);

    // print command options
    printf("Please enter a command:"
           "\n  get <file_name>"
           "\n  put <filename>"
           "\n  delete <filename>"
           "\n  ls"
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

    /******************************** get functionality ********************************/
    if (!strcmp(cmd, "get")) {
      char fn[BUFSIZE];

      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      recvfrom(sockfd, &(fn), sizeof(fn), 0, &serveraddr, &serverlen);

      // file is on server
      if (!strcmp(fn, filename)){
        FILE *file = fopen(fn, "wb");
        fwrite(&file, 1, sizeof(file), file);
        printf("File '%s' copied in.\n", filename);

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
        // fread(data, 1, BUFSIZE, file);

        // printf("data %s\n", data);
        sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);

        printf("File '%s' sent.\n", filename);
      } else {
        printf("Unable to send file. File '%s' does not exist. Try again.\n", filename);
      }

    /******************************** delete functionality ********************************/
    } else if (!strcmp(cmd, "delete")) {
      rcv = 0;
      sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
      recvfrom(sockfd, &(rcv), sizeof(rcv), 0, &serveraddr, &serverlen);

      if (rcv) {
        printf("File '%s' deleted\n", filename);
      } else {
        printf("Unable to delete '%s' from server.\n", filename);
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

  // // get a message from the user
  // bzero(buf, BUFSIZE);
  // printf("Please enter msg: ");
  // fgets(buf, BUFSIZE, stdin);

  // // send the message to the server
  // serverlen = sizeof(serveraddr);
  // n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
  // if (n < 0)
  //   error("ERROR in sendto");

  // // print the server's reply
  // n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
  // if (n < 0)
  //   error("ERROR in recvfrom");
  // printf("Echo from server: %s", buf);
  // return 0;
}
