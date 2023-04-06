#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Client

int main()
{
	int sockfd;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	else{
		printf("Socket created\n");
	}
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(12000);
	if ((connect(sockfd, (struct sockaddr *)&serv_addr,
				 sizeof(serv_addr))) < 0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	else{
		printf("Connected to the server\n");
	}
	// for (i = 0; i < 100; i++)
	// 	buff[i] = '\0';
	// recv(sockfd, buff, 100, 0);

	int len;
	char *buff = (char *)malloc(10 * sizeof(char));
	printf("Enter expression: ");
	getline(&buff, &len, stdin); //buffer will be reallocated space
	send(sockfd, buff, len, 0);
	/////////////////////////////
	double ans = 0.0;
	recv(sockfd, &ans, sizeof(ans), 0);
	printf("Answer recieved: %lf\n", ans);
	close(sockfd);
	return 0;
}
