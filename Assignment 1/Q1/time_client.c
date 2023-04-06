#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// CLIENT
int main()
{
	int sockfd;
	int i;
	struct sockaddr_in serv_addr;
	char buff[100];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
	serv_addr.sin_port = htons(10000);

	if ((connect(sockfd, (struct sockaddr *)&serv_addr,
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
	if (recv(sockfd, buff, 100, 0) < 0)
	{
		perror("Unable to read from socket\n");
		exit(0);
	}
	else
	{
		printf("Read from socket - %s\n", buff);
	}
	close(sockfd);
	return 0;
}
