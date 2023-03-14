#include "mysocket.h"
#include <pthread.h>

#define MAX_RECV_LEN 5000
#define MAX_SEND_LEN 1000
#define MAX_SEND_MSGS 100

typedef struct {
    int sockfd;
    char msgs[MAX_SEND_MSGS][MAX_SEND_LEN];
    int front, rear;
} Send_Message;

typedef struct {
    int sockfd;
    char msgs[MAX_RECV_LEN];
    int front, rear;
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
    recv_msgs->front = 0;
    recv_msgs->rear = 0;

    // Create threads R and S
    pthread_t thread_R, thread_S;
    pthread_create(&thread_R, NULL, thread_R, (void *) recv_msgs);
    pthread_create(&thread_S, NULL, thread_S, (void *) send_msgs);

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
    return close(sockfd);
}