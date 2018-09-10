#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include<arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#define IPV4_PORT_NUM 33445
#define IPV6_PORT_NUM 33446
#define IPV4_BUFFER_SIZE 1440
#define IPV6_BUFFER_SIZE 1280
#define LOCAL_HOST_ADDR "127.0.0.1"
#define LOCAL_HOST_ADDR6 "::1"


// ipv4 port #, ipv6 port #, buffer size 
int main(int argc, char *argv[]){
    int ipv4_port;
    int ipv6_port;
    int buff_size;
    int buff_set = 0;

    if (argc == 4) {
        if (*argv[1] == '.'){
            ipv4_port = IPV4_PORT_NUM;
            printf("Default ipv4 port: 33445\n");
        } else {
            ipv4_port = atoi(argv[1]);
            printf("Set ipv4 port: %s\n", argv[1]);
        }
        if (*argv[2] == '.'){
            ipv6_port = IPV6_PORT_NUM;
            printf("Default ipv6 port: 33446\n");
        } else {
            ipv6_port = atoi(argv[2]);
            printf("Set ipv6 port: %s\n", argv[2]);
        }
        if (*argv[3] == '.'){
            buff_size = IPV4_BUFFER_SIZE;
            printf("Default buffer sizes\n");
        } else {
            buff_size = atoi(argv[3]);
            printf("Set buffer size: %s\n", argv[3]);
            buff_set = 1;
        }
    } else {
        printf("server ipv4-port, ipv6-port, buffer size\n");
        exit(1);
    }

    printf("\nstarting server\n");
    int server_desc;
    int server_desc6;
    int client_desc;
    int queue_length = 5;
    int pid;
    struct sockaddr_in client_addr;
    struct sockaddr_in6 client_addr6;
    struct sockaddr_in server_addr;
    struct sockaddr_in6 server_addr6;

    printf("creating ipv4 socket\n");
    server_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (server_desc == -1) {
        printf("could not create socket\n");
        exit(1);
    }

    printf("socket created\n");


    printf("creating ipv6 socket\n");
    server_desc6 = socket(AF_INET6, SOCK_STREAM, 0);

    if (server_desc6 == -1) {
        printf("could not create socket\n");
        exit(1);
    }

    printf("socket created\n");

    // bind 4
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ipv4_port);
    server_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST_ADDR);

    if (bind(server_desc, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind error\n");
        exit(1);
    }

    //bind 6
    server_addr6.sin6_family = AF_INET6;
    server_addr6.sin6_port = htons(ipv6_port);
    inet_pton(AF_INET6, LOCAL_HOST_ADDR6, (void *) &server_addr6.sin6_addr.s6_addr);

    if (bind(server_desc6, (struct sockaddr *) &server_addr6, sizeof(server_addr6)) < 0) {
        perror("bind error\n");
        exit(1);
    }    

    listen(server_desc6, queue_length);
    listen(server_desc, queue_length);

    socklen_t len = sizeof(client_addr);
    socklen_t len6 = sizeof(client_addr6);

    struct pollfd fds[2];
    fds[0].fd = server_desc;
    fds[1].fd = server_desc6;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    while (1) {
        int active_port;
        printf("\nserver waiting\n");
        printf("starting poll\n\n");

        sleep(1);
        int ret = poll(fds, 2, -1);
        if (ret > 0) {
            //echo server to process request
            if ((pid = fork()) == 0) {
                printf("server thread created\n");
                if (fds[0].revents && POLLIN) {
                    if ((client_desc = accept(server_desc, (struct sockaddr *) &client_addr, &len)) < 0) {
                        perror("Accept error\n");
                        exit(1);
                    } else {
                        printf("connection accepted, ipv4\n");
                        active_port = 4;
                    }
                } else if (fds[1].revents && POLLIN) {
                    if ((client_desc = accept(server_desc6, (struct sockaddr *) &client_addr6, &len6)) < 0) {
                        perror("Accept error\n");
                        exit(1);
                    } else {
                        printf("connection accepted, ipv6\n");
                        if (!buff_set) {
                            buff_size = IPV6_BUFFER_SIZE;
                        }
                        active_port = 6;
                    }
                }

                //close port from listening
                close(server_desc);
                close(server_desc6);

                //set buffer size based on ipv
                char message[buff_size];
                char received_message[buff_size];
                char sendBuffer[buff_size];

                //process incoming connection
                printf("reading message\n");
                int i = read(client_desc, received_message, sizeof(received_message));

                if (i < 0) {
                    perror("receive error\n");
                    exit(1);
                } else if (i == 0) {
                    printf("socket closed remotely\n\n");
                } else {
                    printf("data: %s\n", received_message);
                    char path[strlen(received_message) + 1]; //+1 to account for null terminator
                    snprintf(path, sizeof(path), "%s", received_message);
                    strtok(path, "\n"); //removing extra newline characters
                    FILE *fileName = fopen(path, "r"); //Opens the file specified by received_message

                    if(fileName == NULL) {
                        printf("COULD NOT OPEN REQUESTED FILE\n");
                        sprintf(message, "COULD NOT OPEN REQUESTED FILE\n");
                        send(client_desc, message, strlen(message), 0);
                        printf("ending thread\n\n");
                        exit(0);
                    }

                    fseek(fileName, 0, SEEK_END); //Moves the file pointer to the end of the file to get file size
                    int size = ftell(fileName);

                    fseek(fileName, 0, SEEK_SET);
                    printf("FILE SIZE IS %d bytes\n\n", size); //Should be sent to client
                    sprintf(message, "FILE SIZE IS %d bytes\n", size);

                    send(client_desc, message, strlen(message), 0);
                    //Following along to this https://stackoverflow.com/questions/5594042/c-send-file-to-socket
                    size_t bytesRead = 0;
                    int sent = 0;
                    while((bytesRead = fread(sendBuffer, sizeof(char), buff_size, fileName)) > 0) {
                        printf("sending data\n");
                        int offset = 0;
                        while((sent = send(client_desc, sendBuffer+offset, bytesRead, 0)) > 0 || (sent < 0)) {
                            offset += sent;
                            bytesRead -= sent;
                            sleep(1);
                        }
                    /*
                        while(1) {
                        read(client_desc, received_message, sizeof(received_message));
                        if(strcmp(received_message, "ACK") == 0) {
                        break;
                        }
                    }
                    */
                    }

                    printf("\nsend complete\n");
                    fclose(fileName);
                }

                //close connection and end thread
                printf("closing connection and ending thread\n\n");
                close(client_desc);
                exit(0);
            }
        }
        close(client_desc);
    }
    return 0;
}
