#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

// SERVER

int main()
{
	int sockfd, newsockfd; // Socket descriptors
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;
	time_t ticktock; // will contain the current time
	char buf[100]; // will contain the message we send to the client
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
	}
	else{
		printf("Socket created\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(10000);

	// Using serv_addr, we associate the server with its port using bind()
	
	if (bind(sockfd, (struct sockaddr *)&serv_addr,
			 sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}
	else{
		printf("Bind successful\n");
	}

	listen(sockfd, 5); //Up to 5 clients can be queued

	//Iterative Server
	while (1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
						   &clilen);
		printf("Request Accepted!\n");

		if (newsockfd < 0)
		{
			perror("Accept error\n");
			exit(0);
		}
		else{
			printf("Request Accepted!\n");
		}


		// we copy the current time into the buffer and send it to the client
		ticktock = time(NULL);
		strcpy(buf, ctime(&ticktock));
		send(newsockfd, buf, strlen(buf) + 1, 0);
		close(newsockfd);
	}
	close(sockfd);
	return 0;
}
