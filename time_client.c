#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysocket.h"
#define CONSTANTTT 12347
// CLIENT
int main()
{
	int sockfd;
	int i;
	struct sockaddr_in serv_addr;
	char buff[100];

	if ((sockfd = my_socket(AF_INET, SOCK_MyTCP, 0)) < 0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	else
	{
		printf("Socket created\n");
	}

	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(CONSTANTTT);

	if ((my_connect(sockfd, (struct sockaddr *)&serv_addr,
				 sizeof(serv_addr))) < 0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	else
	{
		printf("Connected to the server\n");
	}
	for(int i=0; i<100; i++){
		buff[i] = '\0';
	}
	printf("Reading from socket...\n");
	int x = my_recv(sockfd, buff, 100, 0);
	printf("x = %d\n", x);
	if (x < 0)
	{
		perror("Unable to read from socket\n");
		exit(0);
	}
	else
	{
		printf("Read from socket - %s\n", buff);
	}
	my_close(sockfd);
	return 0;
}
