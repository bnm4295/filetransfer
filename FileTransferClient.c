#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/stat.h>
#include <unistd.h>

#define IPV4_PORT_NUM 33445
#define IPV6_PORT_NUM 33446
#define IPV4_BUFFER_SIZE 1440
#define IPV6_BUFFER_SIZE 1280
#define LOCALHOST "127.0.0.1"
#define LOCALHOST6 "::1"
#define DEFAULTFILE "fileToTransfer"


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	/* Variable Definition */
	int sockfd;
	int sockfd6;
	char *message = (char*)malloc(12 * sizeof(char));
	struct sockaddr_in server;
	struct sockaddr_in6 server6;

	char *server_ipaddr;
	char *client_ipaddr;
	char *filename;
	int ipv4_port;
	int ipv6_port;
	int buff_size;
	char buf[16];
	int usingIPv4 = 1; //1 if using ipv4, 0 if using ipv6

	//Server IP Address (default: localhost)
	//Port Number (default required)
	//Client IP Address (default: localhost)
	//Filename (default: filename)
	//Size of send/receive buffer (1440 for Ipv4 and 1280 for Ipv6 if no size is given)
	if (argc != 6) {
		fprintf(stderr, "Command line arguments are required\n");
		fprintf(stderr, "In order the required arguments are:\n");
		fprintf(stderr, "IP address of remote communication endpoint:\n");
		fprintf(stderr, "	Default value localhost\n");
		fprintf(stderr, "port of remote communication endpoint\n");
		fprintf(stderr, "	  Default value IPv4: 33445\nDefault value IPv6: 33446\n");
		fprintf(stderr, "IP address of local communication endpoint\n");
		fprintf(stderr, "	  Default value localhost\n");
		fprintf(stderr, "input filename (contains data to be echoed)\n");
		fprintf(stderr, "	  Default value fileToTransfer\n");
		fprintf(stderr, "buffer size equals MSS for each packet\n");
		fprintf(stderr, "          Default value 1448\n");
		fprintf(stderr, "To accept any particular default replace\n");
		fprintf(stderr, "the variable with a . in the argument list\n");
		exit(0);
	}

	if (argc == 6) {
		if (*argv[1] == '.') {
			server_ipaddr = LOCALHOST;
			printf("Default server Address: %s\n", server_ipaddr);
			usingIPv4 = 1;
		}
		else {
			server_ipaddr = argv[1];
			printf("IP Server Address: %s\n", server_ipaddr);
			if(inet_pton(AF_INET, argv[1], buf) == 1) {
				usingIPv4 = 1;
			}
			else {
				usingIPv4 = 0;
			}
		}
		if (*argv[2] == '.') {
			if(usingIPv4 == 1) {
				ipv4_port = IPV4_PORT_NUM;
				printf("Default Port value: %d\n", ipv4_port);
			}
			else {
				ipv6_port = IPV6_PORT_NUM;
				printf("Default Port value: %d\n", ipv6_port);
			}
		}
		else {
			ipv4_port = atoi(argv[2]);
			ipv6_port = atoi(argv[2]);
		}
		if (*argv[3] == '.') {
			if(usingIPv4 == 1) {
				client_ipaddr = LOCALHOST;
				printf("Default client address: %s\n", client_ipaddr);
			}
			else {
				client_ipaddr = LOCALHOST6;
				printf("Default client address: %s\n", client_ipaddr);
			}
		}
		else {
			client_ipaddr = argv[3];
			printf("Client address: %s\n", client_ipaddr);
		}
		if (*argv[4] == '.') {
			filename = DEFAULTFILE;
			printf("Default file name: %s\n", DEFAULTFILE);
		}
		else {
			filename = argv[4];
			printf("Filename is: %s\n", filename);

		}
		if (*argv[5] == '.') {
			if(usingIPv4 == 1) {
				buff_size = IPV4_BUFFER_SIZE;
				printf("Default buffer size: %d\n", buff_size);
			}
			else {
				buff_size = IPV6_BUFFER_SIZE;
				printf("Default buffer size: %d\n", buff_size);
			}
		}
		else {
			buff_size = atoi(argv[5]);
			printf("Buffer size: %d\n", buff_size);
		}
	}
	else {
		printf("server ip address, port number, client address, file name, buffer size");
		exit(1);
	}


	/* Get the Socket file descriptor */

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		error("ERROR: Could not create socket \n");
	}
	if ((sockfd6 = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
	{
		error("ERROR: Could not create socket \n");
	}

  if(usingIPv4 == 1) {

		// bind 4
		server.sin_family = AF_INET;
		server.sin_port = htons(ipv4_port);
		server.sin_addr.s_addr = inet_addr(server_ipaddr);
		//bzero(&(server.sin_zero), 8);

		if (connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
			printf("failed to bind ipv4\n");
			perror("bind error\n");
			exit(1);
		}
		printf("Connected to server at port %d\n", ipv4_port);
	}
	else{
		//bind 6
		server6.sin6_family = AF_INET6;
		server6.sin6_port = htons(ipv6_port);
		inet_pton(AF_INET6, server_ipaddr, (void *) &server6.sin6_addr.s6_addr);
		//bzero(&(server.sin_zero), 16);

		if (connect(sockfd6, (struct sockaddr *) &server6, sizeof(server6)) < 0) {
			printf("failed to bind ipv6\n");
		  	perror("bind error\n");
		  	exit(1);
		}
		printf("Connected to server at port %d\n", ipv6_port);
	}


	//Send some data
	message = filename;
	//message needs to be filename

	if(usingIPv4 == 1){
		if (send(sockfd, message, strlen(message), 0) < 0){
			puts("Send failed\n");
			return 1;
		}
		// send(sockfd, message, strlen(message), 0);
		//printf("%s", message);
		puts("Request Sent\n");
	}
	else{
		if (send(sockfd6, message, strlen(message), 0) < 0){
			puts("Send failed\n");
			return 1;
		}
		// send(sockfd6, message, strlen(message), 0);
		//printf("%s", message);
		puts("Request Sent\n");
	}

	// receive
	int maxlen = buff_size;
	char buff[maxlen];
	char* pbuffer = buff;
	char errorMessage[buff_size];
	int file_size;

	//check if file was sent
	if(usingIPv4 == 1) {
		int i = recv(sockfd, errorMessage, sizeof(errorMessage), 0);
		if(strcmp(errorMessage, "COULD NOT OPEN REQUESTED FILE\n") == 0) {
			printf("COULD NOT OPEN REQUESTED FILE\n");
			close(sockfd);
			close(sockfd6);
			return 0;
		// extract file size
		} else if (strcmp(errorMessage, "FILE SIZE IS 0") > 0){
			char *word = strtok(errorMessage, " ");;
			for (int j = 0; word != NULL; j++){
				if (j == 3) {
					file_size = atoi(word);
					printf("File size received: %d\n\n", file_size);
					break;
				}
				word = strtok(NULL, " ");
			}
		}
	} else {
		int i = recv(sockfd6, errorMessage, sizeof(errorMessage), 0);
		if(strcmp(errorMessage, "COULD NOT OPEN REQUESTED FILE\n") == 0) {
			printf("COULD NOT OPEN REQUESTED FILE\n");
			close(sockfd);
			close(sockfd6);
			return 0;
		// extract file size
		} else if (strcmp(errorMessage, "FILE SIZE IS 0") > 0){
			char *word = strtok(errorMessage, " ");;
			for (int j = 0; word != NULL; j++){
				if (j == 3) {
					file_size = atoi(word);
					printf("File size received: %d\n\n", file_size);
					break;
				}
				word = strtok(NULL, " ");
			}
		}
	}
	//open until connection terminates

	FILE *fr = fopen(filename, "w");
	if(fr == NULL) {
		printf("COULD NOT OPEN OUTPUT FILE\n");
		fclose(fr);
		close(sockfd);
		close(sockfd6);
		return 0;
	} 
	else {
		//rewind(fr);
		bzero(pbuffer, maxlen);
		int n = 0;
		if( usingIPv4 == 1){
			while((n = recv(sockfd, pbuffer, maxlen, 0)) > 0) {
				printf("data received\n");
				int write_sz = fwrite(pbuffer, sizeof(char), n, fr);
				if(write_sz < n) {
			  		error("File write failed.\n");
		      	
				}
				bzero(pbuffer, maxlen);
				if (n == 0 || n > buff_size) {
					break;
				}
			}
			if(n < 0){
				if (errno == EAGAIN){
					printf("recv() timed out.\n");
				}
				else{
					fprintf(stderr, "recv() failed due to errno = %d\n", errno);
				}
			} else if (n ==  0) {
				printf("Connection closed by server\n");
			}
	    	fclose(fr);
		}
		else {
			while((n = recv(sockfd6, pbuffer, maxlen, 0)) > 0){
				printf("data received\n");
				int write_sz = fwrite(pbuffer, sizeof(char), n, fr);
				if(write_sz < n){
					error("File write failed.\n");
			  	}
				bzero(pbuffer, maxlen);
				if (n == 0 || n > buff_size){
					break;
				}
			}

			if (n < 0){
				if (errno == EAGAIN){
					printf("recv() timed out.\n");
				}
				else{
					fprintf(stderr, "recv() failed due to errno = %d\n", errno);
				}
			} else if (n ==  0) {
				printf("\nConnection closed by server\n");
			}
			// printf("Ok received from server!\n");
			fclose(fr);
		}
	}

	close(sockfd);
	close(sockfd6);
	return (0);
}
