#include "mysocket.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LEN 5000
#define NUM_MEGS 10

static pthread_t R, S;

typedef struct {
    int sockfd;
    char msgs[NUM_MEGS][MAX_LEN];
    int front, rear, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Message_Table;

Message_Table *send_msgs;
Message_Table *recv_msgs;

void *thread_R(void *arg)
{
    Message_Table *recv_msgs = (Message_Table *) arg;
    char message[MAX_LEN];

    // Receive message from socket

    // Add the message to the Received_Message table
    pthread_mutex_lock(&recv_msgs->mutex);
    while (recv_msgs->count == NUM_MSGS) {
        pthread_cond_wait(&recv_msgs->cond, &recv_msgs->mutex);
    }
    strcpy(recv_msgs->msgs[recv_msgs->rear], message);
    recv_msgs->rear = (recv_msgs->rear + 1) % NUM_MSGS;
    recv_msgs->count++;
    pthread_cond_signal(&recv_msgs->cond);
    pthread_mutex_unlock(&recv_msgs->mutex);
}

void *thread_S(void *arg)
{
    Message_Table *send_msgs = (Message_Table *) arg;
}

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
    send_msgs = (Message_Table *) malloc(sizeof(Message_Table));
    send_msgs->sockfd = sockfd;
    send_msgs->front = 0;
    send_msgs->rear = 0;
    send_msgs->count = 0;
    pthread_mutex_init(&send_msgs->mutex, NULL);
    pthread_cond_init(&send_msgs->cond, NULL);

    recv_msgs = (Message_Table *) malloc(sizeof(Message_Table));
    recv_msgs->sockfd = sockfd;
    recv_msgs->front = 0;
    recv_msgs->rear = 0;
    recv_msgs->count = 0;
    pthread_mutex_init(&recv_msgs->mutex, NULL);
    pthread_cond_init(&recv_msgs->cond, NULL);

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
    sleep(5);
    
    // Terminate threads R and S
    pthread_kill(R, SIGTERM);
    pthread_kill(S, SIGTERM);

    // Clean up Send_Message and Received_Message tables
    pthread_mutex_destroy(&send_msgs->mutex);
    pthread_cond_destroy(&send_msgs->cond);
    free(send_msgs);
    pthread_mutex_destroy(&recv_msgs->mutex);
    pthread_cond_destroy(&recv_msgs->cond);
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
    while(recv_msgs->count == 0)
    {
        sleep(5);
    }

    // Remove message from recv_msgs
    strcpy(buf, recv_msgs->msgs[recv_msgs->front]);
    recv_msgs->front = (recv_msgs->front + 1) % NUM_MEGS;
    recv_msgs->count--;

    return len;
}