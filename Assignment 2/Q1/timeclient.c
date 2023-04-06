// Deepsikha Behera
// 20CS10023
// Assignment - 2: Q1

// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define BUFF 100

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    inet_aton("127.0.0.1", &servaddr.sin_addr);

    int n;
    socklen_t len;
    // char *hello = "CLIENT:HELLO";

    // sendto(sockfd, (const char *)hello, strlen(hello), 0,
    // 		(const struct sockaddr *) &servaddr, sizeof(servaddr));
    // printf("Hello message sent from client\n");
    char *request = "Requesting time from server...";
    struct pollfd fds[1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;
    n = 5;
    int result;
    char buffer[BUFF];
    len = sizeof(servaddr);
    while (n)
    {
        sendto(sockfd, (const char *)request, strlen(request), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        int ret = poll(fds, 1, 3000);
        if (ret > 0)
        {
            result = recvfrom(sockfd, (char *)buffer, BUFF, 0, (struct sockaddr *)&servaddr, &len);
            if (result < 0)
            {
                perror("ERROR: Couldn't receive data from server..\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        if(n>1) printf("Trial %d FAILED - Trying again....\n", 6 - n);
        else printf("Trial %d FAILED\n", 5);
        n--;
    }

    if (n == 0)
    {
        printf("Timeout exceeded.\n");
    }
    else
    {
        buffer[result] = '\0';
        printf("Time Received: %s\n", buffer);
        exit(EXIT_SUCCESS);
    }
    close(sockfd);
    return 0;
}