#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SOCK_MyTCP 1

int my_socket(int domain, int type, int protocol);

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int my_listen(int sockfd, int backlog);

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags);

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags);

int my_close(int sockfd);
