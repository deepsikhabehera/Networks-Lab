/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char** argv)
{
	int			sockfd, newsockfd ;
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];

	// Create a socket for the server

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(atoi(argv[1]));

	// Bind our local address 

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	// Specify size of request queue

	listen(sockfd, 5); 

	while (1) {

		// Wait for a connection, then accept() it

		clilen = sizeof(cli_addr);

		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

		for(i=0; i < 100; i++) buf[i] = '\0';

		// Receive a message from the client

		recv(newsockfd, buf, 100, 0);

		printf("\n");

		// Send a message to the client

		if(strcmp(buf, "Send Load") == 0)
		{
			time_t t1 = atoi(argv[1]);
			srand(difftime(time(NULL), t1));
			int randNum = rand() % 100;
			printf("Load Sent: %d\n", randNum);
			sprintf(buf, "%d", randNum);
		}
		else if(strcmp(buf, "Send Time") == 0)
		{
			time_t t;
			time(&t);
			strcpy(buf, ctime(&t));
		}

		send(newsockfd, buf, strlen(buf) + 1, 0);

		// Close the socket

		close(newsockfd);
	}
	return 0;
}
			

