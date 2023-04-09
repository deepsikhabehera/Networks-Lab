#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <sys/poll.h>

#define PACKET_SIZE 4096
#define MAX_HOPS 16
#define MAX_TRIES 3
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 0

// ICMP packet structure
struct icmp_packet
{
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

// Checksum function
unsigned short checksum(void *b, int len)
{
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

// Calculate time difference
double timediff(struct timeval start, struct timeval end)
{
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 + (double)(end.tv_usec - start.tv_usec) / 1000.0;
}

// Print ICMP headers
void print_icmp_packet(struct icmp_packet *icmp_pkt)
{
    struct icmphdr *icmp_hdr = &icmp_pkt->hdr;
    char *data = icmp_pkt->msg;

    printf("  Type: %d\n", icmp_hdr->type);
    printf("  Code: %d\n", icmp_hdr->code);
    printf("  Checksum: 0x%x\n", icmp_hdr->checksum);
    printf("  Identifier: %d\n", icmp_hdr->un.echo.id);
    printf("  Sequence Number: %d\n", icmp_hdr->un.echo.sequence);

    // Check for non Echo Request/reply or Time Exceeded packets
    if (icmp_hdr->type != ICMP_ECHOREPLY && icmp_hdr->type != ICMP_ECHO && icmp_hdr->type != ICMP_TIME_EXCEEDED)
    {
        printf("  Data:\n");
        struct iphdr *ip_hdr = (struct iphdr *)data;
        printf("    IP Header:\n");
        printf("      Version: %d\n", ip_hdr->version);
        printf("      IHL: %d\n", ip_hdr->ihl);
        printf("      TOS: %d\n", ip_hdr->tos);
        printf("      Total Length: %d\n", ntohs(ip_hdr->tot_len));
        printf("      Identification: %d\n", ntohs(ip_hdr->id));
        printf("      Flags: %d\n", ntohs(ip_hdr->frag_off) & 0xe000);
        printf("      Fragment Offset: %d\n", ntohs(ip_hdr->frag_off) & 0x1fff);
        printf("      Time to Live: %d\n", ip_hdr->ttl);

        // Determine the protocol and print the header fields accordingly
        switch (ip_hdr->protocol)
        {
        case IPPROTO_TCP:
        {
            struct tcphdr *tcp_hdr = (struct tcphdr *)(data + (ip_hdr->ihl * 4));
            printf("    TCP Header:\n");
            printf("      Source Port: %d\n", ntohs(tcp_hdr->source));
            printf("      Destination Port: %d\n", ntohs(tcp_hdr->dest));
            printf("      Sequence Number: %u\n", ntohl(tcp_hdr->seq));
            printf("      Acknowledgment Number: %u\n", ntohl(tcp_hdr->ack_seq));
            printf("      Header Length: %d\n", tcp_hdr->doff * 4);
            printf("      Flags:\n");
            printf("        Urgent: %d\n", tcp_hdr->urg);
            printf("        Acknowledgment: %d\n", tcp_hdr->ack);
            printf("        Push: %d\n", tcp_hdr->psh);
            printf("        Reset: %d\n", tcp_hdr->rst);
            printf("        Syn: %d\n", tcp_hdr->syn);
            printf("        Fin: %d\n", tcp_hdr->fin);
            printf("      Window Size: %d\n", ntohs(tcp_hdr->window));
            printf("      Checksum: 0x%x\n", ntohs(tcp_hdr->check));
            break;
        }
        case IPPROTO_UDP:
        {
            struct udphdr *udp_hdr = (struct udphdr *)(data + (ip_hdr->ihl * 4));
            printf("    UDP Header:\n");
            printf("      Source Port: %d\n", ntohs(udp_hdr->source));
            printf("      Destination Port: %d\n", ntohs(udp_hdr->dest));
            printf("      Length: %d\n", ntohs(udp_hdr->len));
            printf("      Checksum: 0x%x\n", ntohs(udp_hdr->check));
            break;
        }
        default:
            printf("    Unknown Protocol\n");
            break;
        }
    }
    printf("\n");
}

// return the rtt
double template_icmp_packet(char *msg, int ttl, int *seq, struct sockaddr_in intermediate_addr)
{
    char packet[PACKET_SIZE];
    struct icmp_packet *icmp_pkt = (struct icmp_packet *)packet;
    memset(&icmp_pkt->hdr, 0, sizeof(struct icmphdr));

    // Create socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    // Set socket options for TTL
    if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    struct timeval start, end, timeout;

    // Create ICMP packet
    bzero(&icmp_pkt->hdr, sizeof(struct icmphdr));
    icmp_pkt->hdr.type = ICMP_ECHO;
    icmp_pkt->hdr.code = 0;
    icmp_pkt->hdr.un.echo.id = getpid();
    *seq = *seq + 1;
    icmp_pkt->hdr.un.echo.sequence = *seq;
    gettimeofday(&start, NULL);
    icmp_pkt->hdr.checksum = checksum(icmp_pkt, sizeof(struct icmphdr) + strlen(icmp_pkt->msg));
    strcpy(icmp_pkt->msg, msg);

    struct sockaddr_in from_addr;
    int intermediate_node_found = 0;
    double min_rtt = -1;

    int sent = sendto(sock, icmp_pkt, sizeof(struct icmp_packet), 0, (struct sockaddr *)&intermediate_addr, sizeof(intermediate_addr));
    if (sent < 0)
    {
        perror("sendto");
        exit(1);
    }

    char buf[PACKET_SIZE];
    socklen_t from_len = sizeof(from_addr);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (select(sock + 1, &readfds, NULL, NULL, &tv) < 0)
    {
        perror("select");
        exit(1);
    }

    // Wait up to 5 seconds for a reply
    struct pollfd fd;
    fd.fd = sock;
    fd.events = POLLIN;
    int ret = poll(&fd, 1, 3000);
    if (ret == -1)
    {
        perror("poll");
        exit(1);
    }
    else if (ret == 0)
    {
        printf("Timeout waiting for reply\n");
        return -1;
    }

    int received = recvfrom(sock, buf, PACKET_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
    if (received < 0)
    {
        perror("recvfrom");
        exit(1);
    }

    printf("ICMP packet received:\n");
    print_icmp_packet((struct icmp_packet *)buf);

    gettimeofday(&end, NULL);
    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    sleep(1);
    return rtt;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <site/IP> <n> <T>\n", argv[0]);
        exit(1);
    }

    // Get command line arguments
    char *target = argv[1];
    int n = atoi(argv[2]);
    int T = atoi(argv[3]);

    struct sockaddr_in dest_addr;
    struct hostent *host;

    int sock, ttl = 1, seq = 0, tries = 0, hop_count = 1, done = 0;

    char packet[PACKET_SIZE];
    struct icmp_packet *icmp_pkt = (struct icmp_packet *)packet;

    struct timeval start, end, timeout;
    fd_set readfds;

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    // If target is not an IP address
    if (inet_addr(target) == INADDR_NONE)
    {
        // Get host information

        host = gethostbyname(target);

        if (host == NULL)
        {
            printf("Could not resolve %s\n", target);
            exit(1);
        }

        dest_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    }
    // If target is an IP address
    else
    {
        dest_addr.sin_addr.s_addr = inet_addr(target);
    }

    printf("Tracing route to %s : %s\n\n", target, inet_ntoa(dest_addr.sin_addr));

    // While not done and hop count is less than max hops
    while (!done && hop_count <= MAX_HOPS)
    {
        memset(&icmp_pkt->hdr, 0, sizeof(struct icmphdr));

        printf("Hop Count : %d.\t\n\n", hop_count);

        // Create socket
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

        if (sock < 0)
        {
            perror("socket");
            exit(1);
        }

        // Set socket options for TTL
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("setsockopt");
            exit(1);
        }

        // Create ICMP packet
        bzero(&icmp_pkt->hdr, sizeof(struct icmphdr));
        icmp_pkt->hdr.type = ICMP_ECHO;
        icmp_pkt->hdr.code = 0;
        icmp_pkt->hdr.un.echo.id = getpid();
        icmp_pkt->hdr.un.echo.sequence = seq++;
        gettimeofday(&start, NULL);
        icmp_pkt->hdr.checksum = checksum(icmp_pkt, sizeof(struct icmphdr) + strlen(icmp_pkt->msg));

        struct sockaddr_in from_addr, intermediate_addr;
        int intermediate_node_found = 0;
        double min_rtt = -1;

        printf("ICMP packet to be sent:\n");
        print_icmp_packet(icmp_pkt);

        // Send at least 5 ICMP packets to discover intermediate nodes
        for (int i = 0; i < 10; i++)
        {
            int sent = sendto(sock, icmp_pkt, sizeof(struct icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (sent < 0)
            {
                perror("sendto");
                exit(1);
            }

            printf("ICMP packet sent\n\n");

            char buf[PACKET_SIZE];
            socklen_t from_len = sizeof(from_addr);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            if (select(sock + 1, &readfds, NULL, NULL, &tv) < 0)
            {
                perror("select");
                exit(1);
            }

            // Wait up to 5 seconds for a reply
            struct pollfd fd;
            fd.fd = sock;
            fd.events = POLLIN;
            int ret = poll(&fd, 1, 3000);
            if (ret == -1)
            {
                perror("poll");
                exit(1);
            }
            else if (ret == 0)
            {
                printf("Timeout waiting for reply\n\n");
                continue;
            }

            int received = recvfrom(sock, buf, PACKET_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
            if (received < 0)
            {
                perror("recvfrom");
                exit(1);
            }

            printf("ICMP packet received:\n");
            print_icmp_packet((struct icmp_packet *)buf);

            struct timeval end;
            gettimeofday(&end, NULL);
            double rtt = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
            printf("\n%.2f ms\t\n\n", rtt);
            sleep(1);

            // Check if the intermediate node has been found
            if (from_addr.sin_addr.s_addr != dest_addr.sin_addr.s_addr)
            {
                if (!intermediate_node_found || rtt < min_rtt)
                {
                    intermediate_node_found = 1;
                    intermediate_addr = from_addr;
                    min_rtt = rtt;
                }
            }
        }

        hop_count++;

        if (from_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr)
        {
            printf("Reached destination!\n");
            done = 1;
        }

        close(sock);

        if (intermediate_node_found)
        {
            printf("Intermediate node found: %s\n", inet_ntoa(intermediate_addr.sin_addr));

            char *msg_emp = "";
            double rtt_empty = template_icmp_packet(msg_emp, ttl, &seq, intermediate_addr);
            char *msg_smol = "hello";
            double rtt_small = template_icmp_packet(msg_smol, ttl, &seq, intermediate_addr);
            char *msg_larg = "delilah let me be the one to light a fire inside your heart";
            double rtt_large = template_icmp_packet(msg_larg, ttl, &seq, intermediate_addr);
            printf("Latency for intermediate node <%s>: %f ms\n", inet_ntoa(intermediate_addr.sin_addr), rtt_empty);
            double bandwidth = 1.0 * (strlen(msg_larg) - strlen(msg_smol)) / (rtt_large - rtt_small);
            printf("Bandwidth of this intermediate link: %f\n", bandwidth);
        }
        else if (!done)
        {
            printf("Intermediate node not found\n");
        }

        ttl++;
    }

    return 0;
}