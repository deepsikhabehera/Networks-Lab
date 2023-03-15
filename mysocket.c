#include "mysocket.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LEN 5000
#define NUM_MSGS 10
#define MAX_SEND_LEN 1000 

static pthread_t R, S;

typedef struct {
    int sockfd;
    char msgs[NUM_MSGS][MAX_LEN];
    int front, rear, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Message_Table;

Message_Table *send_msgs = NULL;
Message_Table *recv_msgs = NULL;

void *thread_R(void *arg)
{
    Message_Table *recv_msgs = (Message_Table *) arg;
    char message[MAX_LEN]="";
    char buffer[MAX_SEND_LEN];
    int bytes_read;
    int count = 0;
    //Receiving packets of 1000 bytes, exits if 5 packets are received at max
    while((bytes_read = recv(recv_msgs->sockfd, buffer, MAX_SEND_LEN, 0))>0)
    {
        strncat(message, buffer, bytes_read);
        count++;
        if(count==5)
        {
            break;
        }
    }
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
    char message[MAX_LEN];
    int bytes_sent;
    // Remove message from the Send_Message table
    pthread_mutex_lock(&send_msgs->mutex);
    while (send_msgs->count == 0) {
        pthread_cond_wait(&send_msgs->cond, &send_msgs->mutex);
    }
    strcpy(message, send_msgs->msgs[send_msgs->front]);
    send_msgs->front = (send_msgs->front + 1) % NUM_MSGS;
    send_msgs->count--;
    pthread_cond_signal(&send_msgs->cond);
    pthread_mutex_unlock(&send_msgs->mutex);

    // Send the message in packets of 1000 bytes
    for(int i=0; i<strlen(message); i+=MAX_SEND_LEN)
    {
        if(message[i]=="\0" || message[i]==NULL || i>=strlen(message) || i>=MAX_LEN)
        {
            break;
        }
        bytes_sent = send(send_msgs->sockfd, message+i, MAX_SEND_LEN, 0);
        i+=MAX_SEND_LEN;
    }
}

int my_socket(int domain, int type, int protocol)
{
    if(type != SOCK_MyTCP)
    {
        perror("Error: Only SOCK_MyTCP is supported.");
    }
    type = SOCK_STREAM;
    int sockfd;
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        perror("Error: Socket creation failed.");
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
    if(send_msgs==NULL || recv_msgs==NULL)
    {
        perror("Error: Socket hasn't been initialized.");
    }

    // Terminate threads R and S
    pthread_kill(R, SIGTERM);
    pthread_kill(S, SIGTERM);

    // Clean up Send_Message and Received_Message tables
    pthread_mutex_destroy(&send_msgs->mutex);
    pthread_cond_destroy(&send_msgs->cond);
    free(send_msgs);
    send_msgs = NULL;
    pthread_mutex_destroy(&recv_msgs->mutex);
    pthread_cond_destroy(&recv_msgs->cond);
    free(recv_msgs);
    recv_msgs = NULL;

    return close(sockfd);
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags)
{
    // Block if send_msgs is full
    while(send_msgs->count == NUM_MSGS)
    {
        sleep(5);
    }

    // Add message to send_msgs
    strcpy(send_msgs->msgs[send_msgs->rear], buf);
    send_msgs->rear = (send_msgs->rear + 1) % NUM_MSGS;
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
    recv_msgs->front = (recv_msgs->front + 1) % NUM_MSGS;
    recv_msgs->count--;

    return len;
}