#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

// IP header structure
struct ipheader {
    unsigned char      iph_ihl:4, iph_ver:4;
    unsigned char      iph_tos;
    unsigned short int iph_len;
    unsigned short int iph_ident;
    unsigned short int iph_flags:3, iph_offset:13;
    unsigned char      iph_ttl;
    unsigned char      iph_protocol;
    unsigned short int iph_chksum;
    unsigned int       iph_sourceip;
    unsigned int       iph_destip;
};

// UDP header structure
struct udpheader {
    unsigned short int udph_srcport;
    unsigned short int udph_destport;
    unsigned short int udph_len;
    unsigned short int udph_chksum;
};

// Calculate IP checksum
unsigned short checksum(unsigned short *ptr, int nbytes)
{
    unsigned long sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1)
        sum += *(unsigned char*)ptr;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

int main()
{
    int sockfd;
    char buffer[1024];
    struct ipheader *iph;
    struct udpheader *udph;
    struct sockaddr_in addr;

    // Create raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Set socket address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Prepare IP header
    iph = (struct ipheader *)buffer;
    iph->iph_ver = 4;
    iph->iph_ihl = 5;
    iph->iph_ttl = 64;
    iph->iph_protocol = IPPROTO_UDP;
    iph->iph_sourceip = inet_addr("192.168.1.10");
    iph->iph_destip = inet_addr("127.0.0.1");
    iph->iph_len = sizeof(struct ipheader) + sizeof(struct udpheader);
    iph->iph_ident = htons(54321);
    iph->iph_chksum = checksum((unsigned short*)buffer, sizeof(struct ipheader));

    // Prepare UDP header
    udph = (struct udpheader *)(buffer + sizeof(struct ipheader));
    udph->udph_srcport = htons(1234);
    udph->udph_destport = htons(5678);
    udph->udph_len = htons(sizeof(struct udpheader));
    udph->udph_chksum = 0;

    // Send packet
    if (sendto(sockfd, buffer, iph->iph_len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto() failed");
        exit(EXIT_FAILURE);
    }

    // Close socket
    close(sockfd);

    return 0;
}
