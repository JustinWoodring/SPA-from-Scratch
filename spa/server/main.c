#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static char path[100];
static int counter = 0;

int main(char * argv[], int argc){

    strncpy(path, "../client", sizeof(path));
    
    //Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(1));

    //Create bind address
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr))<0){
        puts("Cannot bind.");
        exit(-1);
    }

    if (listen(sockfd, 5)<0){
        puts("Cannot listen.");
        exit(-1);
    }

    while(1){
        struct sockaddr_in peer_address;
        int peer_addrlen = sizeof(peer_address);

        int new_socket = accept(sockfd, (struct sockaddr *) &peer_address, (socklen_t*) &peer_addrlen);

        handle(new_socket);
    }
}

void handle(int socket){
    char response_buffer[2048];
    memset(response_buffer, '\0', 2048);

    char buffer[1024];
    int numbytes;

    numbytes = read(socket, buffer, sizeof(buffer));

    char method[50], file[50], proto_buf[50], proto[50], proto_version[50];

    strncpy(method, strtok(buffer," "), 50);
    strncpy(file, strtok(NULL, " "), 50);
    strncpy(proto_buf, strtok(NULL, " "), 50);
    strncpy(proto,strtok(proto_buf, "/"), 50);
    strncpy(proto_version,strtok(NULL, "\r\n"), 50);

    if(strncmp("GET", method, sizeof(method)) == 0){
        if(strncmp("/api/", file, sizeof(5)) == 0){
            printf("Received a GET request to the API for %s using %s version %s.\n", file, proto, proto_version);
            if(strncmp("/api/counter", file, sizeof("/api/counter")) == 0){
                char json_response[50];
                memset(json_response, '\0', 50);

                snprintf(json_response, sizeof(json_response), "{\"count\": %d}", counter++);

                snprintf(response_buffer, sizeof(response_buffer), "%s\r\n%s\r\n%s%d\r\n%s%s\r\n%s\r\n\r\n%s",
                    "HTTP/1.1 200 OK",
                    "Server: Dinkomatic",
                    "Content-Length: ", strlen(json_response),
                    "Content-Type: ", "application/json",
                    "Connection: Closed",
                    json_response
                );

                printf("Sending %s...\n", response_buffer);

                int response_bytes_sent = send(socket, response_buffer, strlen(response_buffer), 0);

                printf("Sent %d response bytes.\n", response_bytes_sent);
            }
        } else {
            printf("Received a GET request for %s using %s version %s.\n", file, proto, proto_version);
        
            char file_loc[150];
            memset(file_loc, '\0', 150);
            strncat(file_loc, path, sizeof(path));
            strncat(file_loc, file, sizeof(file));

            printf("Attempting to read %s\n", file_loc);

            char file_buffer[1024*100];
            memset(file_buffer, '\0', 1024*100);
            FILE * myfile = fopen(file_loc, "rb+");

            if(myfile == NULL){
                snprintf(file_loc, sizeof(file_loc), "%s%s", path, "/index.html");
                printf("File not found! Attempting to read %s\n", file_loc);
                myfile=fopen(file_loc,"rb+");
            }

            if(myfile != NULL){
                fseek(myfile, 0, SEEK_SET);
                int bytes_read = fread(file_buffer, 1, 1024*100, myfile);
                if(bytes_read < 1024*100){
                    char file_name[50];
                    char file_type[50];

                    strncpy(file_name, strrchr(file_loc, '/')+1, sizeof(file_name));
                    
                    char * extension = strrchr(file_loc, '.')+1;

                    if(strncmp(extension, "html", 5)==0){
                        strncpy(file_type, "text/html", sizeof(file_type));
                    }
                    else if(strncmp(extension, "png", 5)==0){
                        strncpy(file_type, "image/png", sizeof(file_type));
                    } 
                    else if(strncmp(extension, "css", 5)==0){
                        strncpy(file_type, "text/css", sizeof(file_type));
                    }

                    snprintf(response_buffer, sizeof(response_buffer), "%s\r\n%s\r\n%s%d\r\n%s%s\r\n%s%s\r\n%s\r\n\r\n",
                    "HTTP/1.1 200 OK",
                    "Server: Dinkomatic",
                    "Content-Length: ", bytes_read,
                    "Content-Type: ", file_type,
                    "Content-Name:", file_name,
                    "Connection: Closed");

                    printf("Sending %s...\n", response_buffer);
                    int response_bytes_sent = send(socket, response_buffer, strlen(response_buffer), 0);
                    int file_bytes_sent = send(socket, file_buffer, bytes_read, 0);
                    printf("Sent %d response bytes and file %d file bytes.\n", response_bytes_sent, file_bytes_sent);

                }
            }
        }
    }

    close(socket);
}