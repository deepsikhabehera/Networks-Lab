#include "mysocket.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LEN 5000
#define NUM_MEGS 100

static pthread_t R, S;

typedef struct {
    int sockfd;
    char msgs[NUM_MEGS][MAX_LEN];
    int front, rear, count;
} Send_Message;

typedef struct {
    int sockfd;
    char msgs[MAX_LEN];
    int isempty;
} Received_Message;

Send_Message *send_msgs;
Received_Message *recv_msgs;

void *thread_R(void *arg);
void *thread_S(void *arg);

int my_socket(int domain, int type, int protocol)
{
    if(type != SOCK_MyTCP)
    {
        return -1;
    }
    type = SOCK_STREAM;
    int sockfd;
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        return -1;
    }

    // Initialize send_msgs and recv_msgs
    send_msgs = (Send_Message *) malloc(sizeof(Send_Message));
    send_msgs->sockfd = sockfd;
    send_msgs->front = 0;
    send_msgs->rear = 0;

    recv_msgs = (Received_Message *) malloc(sizeof(Received_Message));
    recv_msgs->sockfd = sockfd;
    recv_msgs->isempty = 1;

    // Create threads R and S
    pthread_create(&R, NULL, thread_R, (void *) recv_msgs);
    pthread_create(&S, NULL, thread_S, (void *) send_msgs);

    return sockfd;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

int my_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return accept(sockfd, addr, addrlen);
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect(sockfd, addr, addrlen);
}

int my_close(int sockfd)
{
    // Terminate threads R and S
    pthread_kill(R, SIGTERM);
    pthread_kill(S, SIGTERM);

    // Clean up Send_Message and Received_Message tables
    free(send_msgs);
    free(recv_msgs);

    return close(sockfd);
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags)
{
    // Block if send_msgs is full
    while(send_msgs->count == NUM_MEGS)
    {
        sleep(5);
    }

    // Add message to send_msgs
    strcpy(send_msgs->msgs[send_msgs->rear], buf);
    send_msgs->rear = (send_msgs->rear + 1) % NUM_MEGS;
    send_msgs->count++;

    return len;
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{
    // Block if recv_msgs is empty
    while(recv_msgs->isempty)
    {
        sleep(5);
    }

    // Remove message from recv_msgs
    strcpy(buf, recv_msgs->msgs);
    recv_msgs->isempty = 1;

    return len;
}