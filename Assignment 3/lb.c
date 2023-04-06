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
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

			/* THE SERVER PROCESS */

int main(int argc, char** argv)
{
	int			sockfd, newsockfd, clisockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr, cli_serv_addr;
	struct pollfd fds[2];
    int timeout = (5 * 1000);

	int i;
	char buf[100];		

	// Create a socket for the server

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(atoi(argv[1]));

	// Bind our local address

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	// Initialize the pollfd structure

	memset(fds, 0 , sizeof(fds));
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;
	int rc;

	// Initialize the load variables

	int L1 = 0;
	int L2 = 0;

	listen(sockfd, 2); 

	time_t T;
	time_t T1;

	while(1)
	{
		// Get the current time

		T = time(NULL);

		// Poll the socket for 5 seconds

		rc = poll(fds, 2, timeout);


		// If poll returns -1, then there is an error
		
		if (rc < 0) {
			printf("  poll() failed\n");
			break;
		}
		
		// If poll returns 0, then there is a timeout

		if (rc == 0) {
			
			// Create a socket for the client 

			if ((clisockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("Unable to create socket\n");
				exit(0);
			}	

			cli_serv_addr.sin_family	= AF_INET;
			inet_aton("127.0.0.1", &cli_serv_addr.sin_addr);
			cli_serv_addr.sin_port	= htons(atoi(argv[2]));

			// Connect to the server S1

			if ((connect(clisockfd, (struct sockaddr *) &cli_serv_addr,
								sizeof(cli_serv_addr))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}

			// Send message to the server

			strcpy(buf, "Send Load");
			send(clisockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 100; i++) buf[i] = '\0';

			// Receive the load from the server

			recv(clisockfd, buf, 100, 0);
			printf("Load received from %s %s\n", argv[2], buf);
			L1 = atoi(buf);

			// Close the socket

			close(clisockfd);

			// Create a socket for the client
			
			if ((clisockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("Unable to create socket\n");
				exit(0);
			}	

			cli_serv_addr.sin_family	= AF_INET;
			inet_aton("127.0.0.1", &cli_serv_addr.sin_addr);
			cli_serv_addr.sin_port	= htons(atoi(argv[3]));

			// Connect to the server S2

			if ((connect(clisockfd, (struct sockaddr *) &cli_serv_addr,
								sizeof(cli_serv_addr))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}

			// Send message to the server

			strcpy(buf, "Send Load");
			send(clisockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 100; i++) buf[i] = '\0';

			// Receive the load from the server

			recv(clisockfd, buf, 100, 0);
			printf("Load received from %s %s\n\n", argv[3], buf);
			L2 = atoi(buf);
			close(clisockfd);

			// Set the timeout to 5 seconds

			timeout = 5 * 1000;
			continue;
		}

		// If poll returns 1, then there is a request

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		// Fork a child process to handle the request

		int ip;
		if (fork() == 0) {

			close(sockfd);	

			// Find the server with the least load

			if(L1 < L2) {
				ip = atoi(argv[2]);
			}
			else {
				ip = atoi(argv[3]);
			}

			printf("Sending client request to %d\n\n", ip);

			// Create a socket for the client

			if ((clisockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("Unable to create socket\n");
				exit(0);
			}	

			cli_serv_addr.sin_family	= AF_INET;
			inet_aton("127.0.0.1", &cli_serv_addr.sin_addr);
			cli_serv_addr.sin_port	= htons(ip);

			// Connect to the server

			if ((connect(clisockfd, (struct sockaddr *) &cli_serv_addr,
								sizeof(cli_serv_addr))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}

			// Send message to the server

			strcpy(buf, "Send Time");
			send(clisockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 100; i++) buf[i] = '\0';

			// Receive the time from the server

			recv(clisockfd, buf, 100, 0);

			// Close the client socket

			close(clisockfd);

			// Send the time to the client

			send(newsockfd, buf, strlen(buf) + 1, 0);

			// Close the socket
			close(newsockfd);

			exit(0);
		}

		close(newsockfd);

		// Set the timeout to remaining time

		T1 = time(NULL);
		timeout = 5000 - difftime(T1, T) * 1000;
	}

	return 0;
}
			

