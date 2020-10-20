/*
 * webserver.c - A concurrent TCP echo server using threads
 * using C code template from httpechosrv.c (same dir)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <pthread.h>
#include <signal.h> // to gracefully stop

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define TYPELENGTH 8


int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
char *fileType(char *filename);
char *contentType(char *ext);
void inthand (int signum);

char *file_types [TYPELENGTH] = {".html", ".txt", ".png", ".gif", ".jpg", ".css", ".js", ".ico"};
char *content_types [TYPELENGTH] = {"text/html", "text/plain", "image/png", "image/gif",
                                    "image/jpg", "text/css", "application/javascript", "image/x-icon"};

int main(int argc, char **argv) {
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
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
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

void respond(int n) {
    char *root;
	char *extension;
	root = getenv("PWD");
	char *document_root;
	document_root = malloc(100);
	strcpy(document_root, root);
	strcat(document_root, "/www");

    char mesg[99999], *request_str[3], data_to_send[MAXBUF], path[99999];
    int request, fd, bytes_read;

    memset((void*)mesg, (int)'\0', 99999);

    request=recv(100, mesg, 99999, 0);

    if (request<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (request==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        request_str[0] = strtok(mesg, " \t\n");
        if (strncmp(request_str[0], "GET\0", 4)==0)
        {
            request_str[1] = strtok (NULL, " \t");
            request_str[2] = strtok (NULL, " \t\n");
            /* 400 Error */
            if ( strncmp( request_str[2], "HTTP/1.0", 8)!=0 && strncmp( request_str[2], "HTTP/1.1", 8)!=0 )
            {
            	char *bad_request_str;
            	bad_request_str = malloc(80);
            	strcpy(bad_request_str, "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: ");
            	strcat(bad_request_str, request_str[2]);
            	strcat(bad_request_str, "\n");

            	//char *bad_request_str = "HTTP/1.1 400 Bad Request: Invalid HTTP-Version: %s\n", request_str[2];
            	int bad_request_strlen = strlen(bad_request_str);
                write(100, bad_request_str, bad_request_strlen);
                //free(bad_request_str);
            }
            else
            {
                if ( strncmp(request_str[1], "/\0", 2)==0 )
                    request_str[1] = "/index.html"; //Client is requesting the root directory, send it index.html

                extension = fileType(request_str[1]);
                int type_found = 0;
                int i = 0;
                for(i; strcmp(file_types[i],"") != 0; i++){
                	if(strcmp(file_types[i], extension) == 0){
                		type_found = 1;
                		break;
                	}
                }
                /* 501 Error */
                if(type_found != 1){
                	char *not_imp_str;
                	not_imp_str = malloc(80);
                	strcpy(not_imp_str, "HTTP/1.1 501 Not Implemented: ");
                	strcat(not_imp_str, path);
                	strcat(not_imp_str, "\n");

					int not_imp_strlen = strlen(not_imp_str);
					write(100, not_imp_str, not_imp_strlen);
					//free(not_imp_str);
                }

                strcpy(path, document_root);
                strcpy(&path[strlen(document_root)], request_str[1]); //creating an absolute filepath

                /* 400 Error */
                if (strlen(path) > 100){

                	char *invalid_uri_str;
                	invalid_uri_str = malloc(80);
                	strcpy(invalid_uri_str, "HTTP/1.1 400 Bad Request: Invalid URI: ");
                	strcat(invalid_uri_str, path);
                	strcat(invalid_uri_str, "\n");

                	int invalid_uri_strlen = strlen(invalid_uri_str);
                	write(100, invalid_uri_str, invalid_uri_strlen);
                	//free(invalid_uri_str);
                }

                else{
	                if ( (fd=open(path, 01))!=-1 )
	                {

	                	char *content_type_header;
	                	content_type_header = malloc(50);
	                	strcpy(content_type_header, "Content-Type: ");
	                	strcat(content_type_header, contentType(extension));
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

	                    send(100, "HTTP/1.1 200 OK\n", 16, 0);
	                    send(100, content_type_header, content_type_header_len, 0);
	                    send(100, content_length_header, content_length_header_len, 0);
	                    send(100, "Connection: keep-alive\n\n", 24, 0);
	                    while ( (bytes_read=read(fd, data_to_send, MAXBUF))>0 )
	                        write (100, data_to_send, bytes_read);

	                    //free(content_type_header);
	                    //free(content_length_header);
	                }
	                /* 404 Error */
	                else{
	                	char *not_found_str;
	                	not_found_str = malloc(80);
	                	strcpy(not_found_str, "HTTP/1.1 404 Not Found: ");
	                	strcat(not_found_str, path);
	                	strcat(not_found_str, "\n");

	                	int not_found_strlen = strlen(not_found_str);
	                	write(100, not_found_str, not_found_strlen); //file not found, send 404 error
	                	//free(not_found_str);
	                }
            }
            }
        }
    }

    shutdown (100, SHUT_RDWR);
    close(100);
}

/* thread routine */
void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello World!</h1>";

    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);
    strcpy(buf, httpmsg);
    printf("server returning a http message with the following content.\n%s\n",buf);
    write(connfd, buf, strlen(httpmsg));

}

/*
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure
 */
int open_listenfd(int port){
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

/* get file type (extension) of file */
char *fileType(char *filename) {
    char *ext = strrchr(filename, '.');
    if (!ext) {
       // no extension
    } else {
        return ext + 1;
    }
}

/* looking at the extension, and return supported content type*/
char *contentType(char *ext) {
    for (int i = 0; i < TYPELENGTH; i++) {
        if (strcmp(ext, file_types[i]) == 0) {
            return ext[i];
        }
    }
}