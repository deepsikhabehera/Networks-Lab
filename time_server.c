#include <stdio.h>
#include "mysocket.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define CONSTANTTT 12318

// SERVER
int main()
{
	int sockfd, newsockfd; // Socket descriptors
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;
	time_t ticktock; // will contain the current time
	char buf[100]; // will contain the message we send to the client
	if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
	}
	else{
		printf("Socket created\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(CONSTANTTT);

	// Using serv_addr, we associate the server with its port using bind()
	
	if (my_bind(sockfd, (struct sockaddr *)&serv_addr,
			 sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}
	else{
		printf("Bind successful\n");
	}

	my_listen(sockfd, 5); //Up to 5 clients can be queued

	//Iterative Server
	while (1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = my_accept(sockfd, (struct sockaddr *)&cli_addr,
						   &clilen);

		if (newsockfd < 0)
		{
			perror("Accept error\n");
			exit(0);
		}
		else{
			printf("Request Accepted!\n");
		}


		// we copy the current time into the buffer and send it to the client
		printf("Sending time to client...\n");
		ticktock = time(NULL);
		strcpy(buf, ctime(&ticktock));
		my_send(newsockfd, buf, strlen(buf) + 1, 0);
		printf("Time sent! %s\n", buf);

		my_close(newsockfd);
	}
	my_close(sockfd);
	return 0;
}
