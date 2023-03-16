#include "mysocket.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LEN 5000
#define NUM_MSGS 10
#define MAX_SEND_LEN 1000
#define T 5

/*Run the following:
    make
    gcc -o clie time_client.c mysocket.o
    gcc -o serv time_server.c mysocket.o
*/

static pthread_t R, S;

typedef struct
{
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
    Message_Table *recv_msgs = (Message_Table *)arg;
    printf("Thread R started\n");
    int sockfd = recv_msgs->sockfd;
    char buffer[MAX_SEND_LEN];
    int bytes_received;
    char message[MAX_LEN];
    int message_idx = 0;
    for (int i = 0; i < MAX_SEND_LEN; i++)
        buffer[i] = '\0';
    for (int i = 0; i < MAX_LEN; i++)
        message[i] = '\0';

    while (1)
    {
        pthread_mutex_lock(&recv_msgs->mutex);
        sockfd = recv_msgs->sockfd;        
        pthread_mutex_unlock(&recv_msgs->mutex);
        int message_len = 0;
        bytes_received = recv(sockfd, buffer, MAX_SEND_LEN, 0);
        if (bytes_received < 0)
        {
            continue;
        }
        if(bytes_received == 0){
            continue;
        }
        if(bytes_received < MAX_SEND_LEN){
            buffer[bytes_received] = '\0';
        }
        bytes_received++;
        for (int i = 0; i < bytes_received - 1; i++)
        {
            if (buffer[i] == '\0' || message_idx == MAX_LEN - 1 || bytes_received == 0)
            {
                // End of message reached
                message[message_idx] = '\0';
                message_len = message_idx;
                message_idx = 0;
                // Add the message to the Received_Message table
                pthread_mutex_lock(&recv_msgs->mutex);
                while (recv_msgs->count == NUM_MSGS)
                {
                    pthread_cond_wait(&recv_msgs->cond, &recv_msgs->mutex);
                }
                strcpy(recv_msgs->msgs[recv_msgs->rear], message);
                recv_msgs->rear = (recv_msgs->rear + 1) % NUM_MSGS;
                recv_msgs->count++;
                pthread_cond_signal(&recv_msgs->cond);
                pthread_mutex_unlock(&recv_msgs->mutex);

                // Reset the message buffer
                for (int i = 0; i < MAX_LEN; i++)
                {
                    message[i] = '\0';
                }
                message_len = 0;
                break;
            }
            else
            {
                // Add the character to the message buffer
                message[message_idx++] = buffer[i];
            }
        }
        for (int i = 0; i < MAX_SEND_LEN; i++)
            buffer[i] = '\0';
    }
    return NULL;
}

void *thread_S(void *arg)
{
    Message_Table *send_msgs = (Message_Table *)arg;
    int sockfd = send_msgs->sockfd;
    char buffer[MAX_LEN];
    char buf[MAX_SEND_LEN + 1];
    int bytes_to_send;
    int bytes_sent;
    printf("Thread S started!\n");
    for (int i = 0; i < MAX_LEN; i++)
        buffer[i] = '\0';
    for (int i = 0; i < MAX_SEND_LEN; i++)
        buf[i] = '\0';

    while (1)
    {
        // Sleep for some time T
        sleep(T);

        // Check if any message is waiting to be sent
        pthread_mutex_lock(&send_msgs->mutex);
        if (send_msgs->count > 0)
        {
            // Get the next message to send
            strcpy(buffer, send_msgs->msgs[send_msgs->front]);
            send_msgs->front = (send_msgs->front + 1) % NUM_MSGS;
            send_msgs->count--;
            sockfd = send_msgs->sockfd;
            pthread_mutex_unlock(&send_msgs->mutex);

            // Send the message in chunks of at most MAX_SEND_LEN bytes
            int total_bytes_sent = 0;
            int send_rounds = 0;
            bytes_to_send = strlen(buffer);
            while (bytes_to_send > 0)
            {
                int x = MAX_SEND_LEN;
                if (bytes_to_send < MAX_SEND_LEN)
                    x = bytes_to_send + 1;
                for (int i = 0; i < MAX_SEND_LEN + 1; i++)
                    buf[i] = '\0';
                for(int i = 0; i < x; i++)
                    buf[i] = buffer[total_bytes_sent + i];
                bytes_sent = send(sockfd, buf, x, 0);
                if (bytes_sent < 0)
                {
                    // Error occurred, exit the thread
                    pthread_exit(NULL);
                }
                send_rounds++;
                if (send_rounds == 5 || bytes_sent == 0 || buffer[total_bytes_sent + bytes_sent - 1] == '\0')
                {
                    break;
                }
                bytes_to_send -= bytes_sent;
                total_bytes_sent += bytes_sent;
            }
            for (int i = 0; i < MAX_LEN; i++)
                buffer[i] = '\0';
        }
        else
        {
            pthread_mutex_unlock(&send_msgs->mutex);
        }
    }
}

int my_socket(int domain, int type, int protocol)
{
    if (type != SOCK_MyTCP)
    {
        perror("Error: Only SOCK_MyTCP is supported.");
    }
    type = SOCK_STREAM;
    int sockfd;
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        // perror("Error: Socket creation failed.");
        return sockfd;
    }

    // Initialize send_msgs and recv_msgs
    send_msgs = (Message_Table *)malloc(sizeof(Message_Table));
    send_msgs->sockfd = sockfd;
    send_msgs->front = 0;
    send_msgs->rear = 0;
    send_msgs->count = 0;
    pthread_mutex_init(&send_msgs->mutex, NULL);
    pthread_cond_init(&send_msgs->cond, NULL);

    recv_msgs = (Message_Table *)malloc(sizeof(Message_Table));
    recv_msgs->sockfd = sockfd;
    recv_msgs->front = 0;
    recv_msgs->rear = 0;
    recv_msgs->count = 0;
    pthread_mutex_init(&recv_msgs->mutex, NULL);
    pthread_cond_init(&recv_msgs->cond, NULL);

    // Create threads R and S
    pthread_create(&R, NULL, thread_R, (void *)recv_msgs);
    pthread_create(&S, NULL, thread_S, (void *)send_msgs);

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
    sleep(T);
    if (send_msgs == NULL || recv_msgs == NULL)
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
    while (send_msgs->count == NUM_MSGS)
    {
        sleep(T);
    }

    send_msgs->sockfd = sockfd;

    // Add message to send_msgs
    strcpy(send_msgs->msgs[send_msgs->rear], buf);
    send_msgs->rear = (send_msgs->rear + 1) % NUM_MSGS;
    send_msgs->count++;

    return len;
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{
    recv_msgs->sockfd = sockfd;
    // Block if recv_msgs is empty
    while (recv_msgs->count == 0)
    {
        sleep(T);
    }
    // Remove message from recv_msgs
    strcpy(buf, recv_msgs->msgs[recv_msgs->front]);
    recv_msgs->front = (recv_msgs->front + 1) % NUM_MSGS;
    recv_msgs->count--;

    return len;
}