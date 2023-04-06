// Deepsikha Behera
// 20CS10023
// Assignment - 2: Q2
/* THE SERVER PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#define USERNAME 25
#define BUFFER 50
#define SUPERBUFFER 1024
#define ADDRESS 200

void send_in_packets(int sockfd, char *result)
{
	int j = 0;
	char *packet = (char *)malloc(BUFFER * sizeof(char));
	for (int i = 0; i < BUFFER; i++)
		packet[i] = '\0';
	for (int i = 0; i < strlen(result); i++)
	{
		packet[j++] = result[i];
		if (j == BUFFER)
		{
			send(sockfd, packet, BUFFER, 0);
			j = 0;
			for (int j = 0; j < BUFFER; j++)
				packet[j] = '\0';
		}
	}
	send(sockfd, packet, BUFFER, 0);
}

int main()
{
	int sockfd, newsockfd; /* Socket descriptors */
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;

	int i;
	char *buf = (char *)malloc(BUFFER * sizeof(char));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create socket\n");
		exit(0);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(3000);

	if (bind(sockfd, (struct sockaddr *)&serv_addr,
			 sizeof(serv_addr)) < 0)
	{
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);
	while (1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
						   &clilen);

		if (newsockfd < 0)
		{
			printf("Accept error\n");
			exit(0);
		}
		for (i = 0; i < BUFFER; i++)
			buf[i] = '\0';
		strcpy(buf, "LOGIN:");
		send(newsockfd, buf, 7, 0);

		// Receive username
		char username[USERNAME];
		for (i = 0; i < USERNAME; i++)
			username[i] = '\0';
		recv(newsockfd, username, USERNAME, 0);
		printf("Username: %s\n", username);
		FILE *fp = NULL;
		char *line = NULL;
		size_t len = 0;
		ssize_t read;
		fp = fopen("users.txt", "r");
		if (fp == NULL)
			exit(EXIT_FAILURE);
		int found = 0;
		while (getline(&line, &len, fp) != -1)
		{
			if (line[strlen(line) - 1] == '\n')
				line[strlen(line) - 1] = '\0';
			else
			{
				line[strlen(line)] = '\0';
			}
			if (strcmp(line, username) == 0)
			{
				strcpy(buf, "FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);
				found = 1;
				break;
			}
		}
		if (found == 0)
		{
			strcpy(buf, "NOT FOUND");
			send(newsockfd, buf, strlen(buf) + 1, 0);
		}
		/////////RECEIVE COMMAND AND PROCESS ACCORDINGLY IN CONCURRENT SERVER///////////
		if (fork() == 0)
		{
			close(sockfd);
			while (1)
			{
				char *packet = (char *)malloc(sizeof(char) * BUFFER);
				int cmdlen = 0;
				int size = BUFFER;
				char *input = (char *)malloc(sizeof(*input) * size);
				for (int i = 0; i < size; i++)
					input[i] = '\0';
				for (i = 0; i < BUFFER; i++)
					packet[i] = '\0';

				// LOOP TO RECEIVE INPUT CMD IN PACKETS
				do
				{
					for (i = 0; i < BUFFER; i++)
						packet[i] = '\0';
					recv(newsockfd, packet, BUFFER, 0);
					for (int j = 0; j < BUFFER; j++)
						input[cmdlen++] = packet[j];
					input = realloc(input, sizeof(*input) * (size += BUFFER));
				} while (packet[BUFFER - 1] != '\0');
				input[cmdlen - 1] = '\0';

				printf("Command: %s\n", input);
				char cmd[BUFFER];
				char cdcmd[BUFFER];

				if (strcmp(input, "exit") == 0)
					break;
				int invalidflag = 0;
				if (strlen(input) < 3 || ((input[0] != 'c' || input[1] != 'd') && strlen(input) == 2))
				{
					invalidflag = 1;
				}
				if (strlen(input) >= 3)
				{
					strncpy(cmd, input, 4);
					cdcmd[3] = '\0';
					cmd[4] = '\0';
				}
				strncpy(cdcmd, input, 3);

				for (int i = 0; i < BUFFER; i++)
					buf[i] = '\0';

				if (invalidflag)
				{
					for (i = 0; i < BUFFER; i++)
						buf[i] = '\0';
					strcpy(buf, "$$$$");
					send(newsockfd, buf, strlen(buf), 0);
				}
				else if (strcmp(input, "pwd\0") == 0 || strcmp(input, "pwd \0") == 0)
				{
					char pwdbuf[SUPERBUFFER];
					for (i = 0; i < SUPERBUFFER; i++)
						pwdbuf[i] = '\0';
					if (getcwd(pwdbuf, SUPERBUFFER) != NULL)
					{
						// printf("%s\n", pwdbuf);
					}
					else
					{
						strcpy(pwdbuf, "####");
						send(newsockfd, pwdbuf, strlen(pwdbuf) + 1, 0);
					}
					// send(newsockfd, pwdbuf, strlen(pwdbuf) + 1, 0); /////SEND IN PACKETS
					send_in_packets(newsockfd, pwdbuf);
				}
				else if (strcmp(cmd, "dir \0") == 0 || strcmp(input, "dir\0") == 0)
				{
					// check whether command is valid - if so, execute ls command
					struct dirent *de;
					char *addr = (char *)malloc(ADDRESS * sizeof(char));
					for (int i = 0; i < ADDRESS; i++)
						addr[i] = '\0';
					strncpy(addr, input + 4, strlen(input) - 4);
					if (strcmp(input, "dir\0") == 0)
						strcpy(addr, ".");
					DIR *dr = opendir(addr);
					char *buffer = (char *)malloc(sizeof(char) * SUPERBUFFER);
					for (i = 0; i < SUPERBUFFER; i++)
						buffer[i] = '\0';
					if (dr == NULL)
					{
						strcpy(buffer, "####");
						send(newsockfd, buffer, strlen(buffer) + 1, 0);
					}
					else
					{

						while ((de = readdir(dr)) != NULL)
						{
							strcat(buffer, de->d_name);
							strcat(buffer, " ");
						}
						buffer[strlen(buffer) - 2] = '\0';
						// send(newsockfd, buffer, strlen(buffer), 0); // SEND IN PACKETS
						send_in_packets(newsockfd, buffer);
					}
					closedir(dr);
				}
				else if (strcmp(cdcmd, "cd \0") == 0 || strcmp(input, "cd\0") == 0)
				{
					// check whether command is valid - if so, execute cd command
					char *addr = (char *)malloc(BUFFER * sizeof(char));
					strncpy(addr, input + 3, strlen(input) - 3);
					for (i = 0; i < BUFFER; i++)
						input[i] = '\0';

					if (chdir(addr) == 0)
					{
						strcpy(buf, "Directory Changed.\0");
						send_in_packets(newsockfd, buf);
					}
					else
					{
						strcpy(buf, "####");
						send(newsockfd, buf, strlen(buf) + 1, 0);
					}
				}
				else
				{
					// invalid command
					for (i = 0; i < BUFFER; i++)
						buf[i] = '\0';
					strcpy(buf, "$$$$");
					send(newsockfd, buf, strlen(buf) + 1, 0);
				}
			}
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
