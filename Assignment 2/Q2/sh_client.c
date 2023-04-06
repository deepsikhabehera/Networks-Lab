// Deepsikha Behera
// 20CS10023
// Assignment - 2: Q2
/* THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define USERNAME 25
#define BUFFER 50
#define SUPERBUFFER 1024

int main()
{
	int sockfd;
	struct sockaddr_in serv_addr;

	int i;
	char buf[BUFFER];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(3000);

	if ((connect(sockfd, (struct sockaddr *)&serv_addr,
				 sizeof(serv_addr))) < 0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
	for (i = 0; i < BUFFER; i++)
		buf[i] = '\0';
	recv(sockfd, buf, BUFFER, 0);
	printf("%s\n", buf); // prints "LOGIN:"
	char username[USERNAME];
	scanf("%s", username);
	getchar();
	send(sockfd, username, strlen(username) + 1, 0);

	// Receiving username matching status
	for (i = 0; i < BUFFER; i++)
		buf[i] = '\0';
	recv(sockfd, buf, BUFFER, 0);
	if (strcmp(buf, "NOT FOUND") == 0)
	{
		perror("Invalid Username\n");
		exit(0);
	}
	for (i = 0; i < BUFFER; i++)
		buf[i] = '\0';
	while (1)
	{
		char packet[BUFFER];

		for (i = 0; i < BUFFER; i++)
			packet[i] = '\0';
		printf("~$ ");
		size_t cmdlen = 0;
		char c;
		// Read the command char by char and send in packets of BUFFER size
		while ((c = getchar()) && c != '\n')
		{
			packet[cmdlen++] = c;
			if (cmdlen == BUFFER)
			{
				send(sockfd, packet, BUFFER, 0);
				cmdlen = 0;
				for (i = 0; i < BUFFER; i++)
					packet[i] = '\0';
			}
		}
		send(sockfd, packet, BUFFER, 0); // The last packet
		if(strcmp(packet, "exit") == 0)
			break;

		// receive result in packets
		char *result = (char *)malloc(BUFFER * sizeof(*result));
		int size = BUFFER;
		for (i = 0; i < BUFFER; i++)
			packet[i] = '\0';

		int reslen = 0;
		do
		{
			for (i = 0; i < BUFFER; i++)
				packet[i] = '\0';
			recv(sockfd, packet, BUFFER, 0);
			for (int j = 0; j < BUFFER; j++)
				result[reslen++] = packet[j];
			result = realloc(result, sizeof(*result) * (size += BUFFER));
		} while (packet[BUFFER - 1] != '\0');

		// print result
		if (strcmp(result, "$$$$") == 0)
			printf("Invalid Command.\n");
		else if (strcmp(result, "####") == 0)
			printf("Error in running command.\n");
		else
			printf("%s\n", result);
	}

	close(sockfd);
	return 0;
}
