/* ***********************************
 * udp_server.c - A simple UDP echo server
 * usage: ./udp_server <port>
 *********************************** */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> // for directory

#define BUFSIZE 1024

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
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
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
      printf("get command\n");

    /************************* put command handling *************************/
    } else if (!strcmp(cmd_in, "put")) {
      FILE *file = fopen(filename_in, "wb");
      fwrite(&file, 1, sizeof(file), file);

    /************************* delete command handling *************************/
    } else if (!strcmp(cmd_in, "delete")) {
      // first check if filename exists
      snd = 0;

      if (access(filename_in, F_OK) != -1) {
        snd = 1;
        remove(filename_in);
        sendto(sockfd, &(snd), sizeof(snd), 0, (struct sockaddr *) &clientaddr, clientlen);
      } else {
        sendto(sockfd, &(snd), sizeof(snd), 0, (struct sockaddr *) &clientaddr, clientlen);
      }

    /************************* ls command handling *************************/
    } else if (!strcmp(cmd_in, "ls")) {
      struct dirent **namelist;
      n = scandir(".", &namelist, NULL, alphasort);
      char file_list[BUFSIZE];
      char files[BUFSIZE] = "";
      while (n--) {
        sprintf(file_list, "%s\n", namelist[n]->d_name);
        free(namelist[n]);
        strcat(files,file_list);
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

    // // gethostbyaddr: determine who sent the datagram
    // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
		// 	  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    // if (hostp == NULL)
    //   error("ERROR on gethostbyaddr");
    // hostaddrp = inet_ntoa(clientaddr.sin_addr);
    // if (hostaddrp == NULL)
    //   error("ERROR on inet_ntoa\n");
    // printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    // printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

    // // sendto: echo the input back to the client
    // n = sendto(sockfd, buf, strlen(buf), 0,
	  //      (struct sockaddr *) &clientaddr, clientlen);
    // if (n < 0)
    //   error("ERROR in sendto");
  }
}
