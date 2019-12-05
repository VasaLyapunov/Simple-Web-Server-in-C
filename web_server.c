/* 
 * web_server.c - I gonna reply with file
 * usage: web_server <port>
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> // Use this for directory mapping
#include <sys/stat.h>
#include <math.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#define BUFFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char const *msg) {
    perror(msg);
    exit(1);
}

void debug(char const *msg) {
    printf("%s\n", msg);
}

/*
// Sends message currently in buffer
void sendBuffer(char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen) {
    int n;
    n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
        error("ERROR in sendto");
}
void receiveBuffer(char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen) {
    int n;
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
        error("ERROR in recvfrom");
}
*/

int main(int argc, char **argv) {
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char buffer[BUFFSIZE]; /* message buffer */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */
    int new_socket;
    int addrlen = sizeof(serveraddr);
    int valread;
    //char response[BUFFSIZE] = "";

    /*
     * check command line arguments
     */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);
    while (1) {

        debug("Listening...");
        if (listen(sockfd, 3) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }


        debug("Accepting..."); // Fails if < 0
        while((new_socket = accept(sockfd, (struct sockaddr *)&serveraddr, (socklen_t*)&addrlen)) > 0) {

            // Child will handle the new connection
            if(!fork()) {
                // Close listening socket
                close(sockfd);
                // Handle the request:

                // First receive browser's message:
                debug("Request received:");
                valread = read( new_socket, buffer, 1024);
                //debug(buffer); // TODO rm
                strtok(buffer, " "); // First word is the request type (eg. GET) - We dont use this
                char* filename = strtok(NULL, " ");
                char* filename_tmp[strlen(filename)]; // Copy filename so we can parse the filetype out of it
                strcpy(filename_tmp, filename);
                strtok(filename_tmp, ".");
                char* filetype = strtok(NULL, "");
                filename++; // Gets rid of first character, getting rid of initial slash
                if(!strcmp(filename, "")) {
                    filename = "index.html";
                    filetype = "html";
                }
                debug(filename);
                //debug(filetype);

                // Begin formulating response:
                debug("Sending response...");
                // Gotta compile all of the header parts and get their sizes
                // Base message size contains all these characters: "HTTP/1.1 200 Document Follows\r\n Content-Type: \r\n Content-Length: \r\n\r\n"
                char* base_message_1 = "HTTP/1.1 200 Document Follows\r\n Content-Type: ";
                char* base_message_2 = "\r\n Content-Length: ";
                char* base_message_3 = "\r\n\r\n";
                int base_message_size = strlen(base_message_1) + strlen(base_message_2) + strlen(base_message_3);


                char* content_type;
                if(!strcmp(filetype, "html"))
                    content_type = "text/html";
                else if(!strcmp(filetype, "txt"))
                    content_type = "text/plain";
                else if(!strcmp(filetype, "png"))
                    content_type = "image/png";
                else if(!strcmp(filetype, "gif"))
                    content_type = "image/gif";
                else if(!strcmp(filetype, "jpg"))
                    content_type = "image/jpg";
                else if(!strcmp(filetype, "css"))
                    content_type = "text/css";
                else if(!strcmp(filetype, "js"))
                    content_type = "application/javascript";
                else
                    content_type = "unknown";
                int content_type_size = strlen(content_type);

                // Now file & file length
                // Thanks 'Nils Pipenbrinck' - https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
                char* file_content = 0;
                long file_content_size;
                FILE * f = fopen (filename, "rb");
                if (f) {
                    fseek (f, 0, SEEK_END);
                    file_content_size = ftell (f);
                    fseek (f, 0, SEEK_SET);
                    file_content = malloc(file_content_size);
                    if (file_content)
                    {
                        fread(file_content, 1, file_content_size, f);
                    }
                    fclose (f);
                }
                else {
                    debug("Could not open file!\n");
                    char* error_message = "HTTP/1.1 500 Internal Server Error";
                    send(new_socket, error_message, strlen(error_message), 0 );
                    return 500;
                }

                // We have to make content length and get its size as a string too
                int content_length_size = (int)((ceil(log10(file_content_size))+1)*sizeof(char));   // Find buffer size needed to store the file contents length as string
                char content_length[content_length_size];                                           // Create the buffer
                sprintf(content_length, "%d", file_content_size);                           // Fill buffer with file contents length converted to a string

                // Combine all strings into one mega response
                int response_size = base_message_size + content_type_size + content_length_size;
                char response[response_size];
                bzero(response, response_size);
                strcat(response, base_message_1);
                strcat(response, content_type);
                strcat(response, base_message_2);
                strcat(response, content_length);
                strcat(response, base_message_3);
                //strcat(response, file_content);

                debug("Response:"); //TODO rm
                debug("====================");
                debug(response);
                debug("====================");

                send(new_socket, response, strlen(response), 0 );
                send(new_socket, file_content, file_content_size, 0 );
                debug("Response sent!\n");

                return 0;
            }
            else {
                // Parent has nothing to do with request. Will keep listening for more
                close(new_socket);
            }
        }
        // While loop fails
        perror("accept");
        exit(EXIT_FAILURE);
    }
}

#pragma clang diagnostic pop