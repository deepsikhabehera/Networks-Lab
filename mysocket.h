#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct mysocket {
    int fd;
    struct sockaddr_in addr;
    struct sockaddr_in peer;
};