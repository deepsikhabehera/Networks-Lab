// Deepsikha Behera
// 20CS10023
// Assignment - 2: Q1

// A Simple UDP Server that sends a HELLO message
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXLINE 100

int main()
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    time_t ticktock;
    // Create socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8181);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("\nServer Running....\n");

    int n;
    socklen_t len;
    char buffer[MAXLINE];
    while (1)
    {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        // printf("Query: %s\n", buffer);
        for (int i = 0; i < MAXLINE; i++)
            buffer[i] = '\0';
        ticktock = time(NULL);
        strcpy(buffer, ctime(&ticktock));
        len = sizeof(cliaddr);
        n = sendto(sockfd, (char *)buffer, strlen(buffer)+1, 0,
                   (struct sockaddr *)&cliaddr, len);
    }
    close(sockfd);
    return 0;
}