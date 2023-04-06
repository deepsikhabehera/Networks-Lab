#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <errno.h>

#define PORT 8012
#define BUFFER 100

char *recv_in_packets(int sockfd)
{
    int len = 0;
    int size = BUFFER;
    char *buff = malloc(sizeof(*buff) * BUFFER);
    char *str = malloc(sizeof(*str) * size);

    do
    {
        for (int i = 0; i < BUFFER; i++)
            buff[i] = '\0';
        recv(sockfd, buff, BUFFER, 0);
        for (int j = 0; j < BUFFER; j++)
        {
            str[len++] = buff[j];
            if (buff[j] == '\0')
                break;
        }
        str = realloc(str, sizeof(*str) * (size += BUFFER));
    } while (buff[BUFFER - 1] != '\0');
    free(buff);
    return str;
}

void send_in_packets(int sockfd, char *str)
{
    int len = 0;
    char buff[BUFFER];
    for (int i = 0; i < BUFFER; i++)
        buff[i] = '\0';
    for (int i = 0; i < strlen(str); i++)
    {
        buff[len++] = str[i];
        if (len == BUFFER)
        {
            send(sockfd, buff, BUFFER, 0);
            len = 0;
            for (int j = 0; j < BUFFER; j++)
                buff[j] = '\0';
        }
    }
    send(sockfd, buff, BUFFER, 0);
    return;
}

char *get_expires()
{
    char *result = (char *)malloc(15 * sizeof(char));
    time_t current_time;
    time(&current_time);
    current_time += 3 * 24 * 60 * 60;
    struct tm *time_info = localtime(&current_time);
    strftime(result, 15, "%d%m%y:%H%M%S", time_info);
    return result;
}

void bad_request_response(int sockfd, char *content_type)
{
    char *response_body = "Bad Request";
    int content_length = strlen(response_body);
    char *expires = get_expires();
    char *response = (char *)malloc(1000 * sizeof(char));
    sprintf(response, "HTTP/1.1 400 Bad Request\r\n"
                      "Expires: <%s>\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Language: en-us\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: %s\r\n"
                      "\r\n"
                      "%s",
            expires, content_length, content_type, response_body);
    printf("Response message sent: \n");
    char *body_start = strstr(response, "\r\n\r\n");
    int request_length = body_start - response;
    for (int i = 0; i < request_length; i++) {
        printf("%c", response[i]);  
    }
    printf("\n\n");
    send_in_packets(sockfd, response);
    return;
}

void not_found_request_response(int sockfd, char *content_type)
{
    char *response_body = "404 Not Found";
    int content_length = strlen(response_body);
    char *expires = get_expires();
    char *response = (char *)malloc(1000 * sizeof(char));
    sprintf(response, "HTTP/1.1 404 Not Found\r\n"
                      "Expires: <%s>\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Language: en-us\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: %s\r\n"
                      "\r\n"
                      "%s",
            expires, content_length, content_type, response_body);
    printf("Response message sent: \n");
    char *body_start = strstr(response, "\r\n\r\n");
    int request_length = body_start - response;
    for (int i = 0; i < request_length; i++) {
        printf("%c", response[i]);  
    }
    printf("\n\n");
    send_in_packets(sockfd, response);
    return;
}

void ok_request_response(int sockfd, char *content_type, char *last_modified, char *file_content)
{
    char *response = (char *)malloc(1000 * sizeof(char));
    int content_length = strlen(file_content);
    char *expires = get_expires();
    sprintf(response, "HTTP/1.1 200 OK\r\n"
                      "Expires: <%s>\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Language: en-us\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: %s\r\n"
                      "Last-Modified: <%s>\r\n"
                      "\r\n"
                      "%s",
            expires, content_length, content_type, last_modified, file_content);
    printf("Response message sent: \n");
    char *body_start = strstr(response, "\r\n\r\n");
    int request_length = body_start - response;
    for (int i = 0; i < request_length; i++) {
        printf("%c", response[i]);  
    }
    printf("\n\n");
    send_in_packets(sockfd, response);
    return;
}

void no_permission_request_response(int sockfd, char *content_type)
{
    char *response_body = "Forbidden File - Couldn't open file";
    int content_length = strlen(response_body);
    char *expires = get_expires();
    char *response = (char *)malloc(1000 * sizeof(char));
    sprintf(response, "HTTP/1.1 403 Forbidden\r\n"
                      "Expires: <%s>\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Language: en-us\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: %s\r\n"
                      "\r\n"
                      "%s",
            expires, content_length, content_type, response_body);
    printf("Response message sent: \n"); 
    char *body_start = strstr(response, "\r\n\r\n");
    int request_length = body_start - response;
    for (int i = 0; i < request_length; i++) {
        printf("%c", response[i]);  
    }
    printf("\n\n");
    send_in_packets(sockfd, response);
    return;
}

void process_GET_request(int sockfd, char *path, char *content_type)
{
    // open the file given in the relative path
    FILE *fp = fopen(path, "r");
    struct stat file_stat;
    if (stat(path, &file_stat) == -1)
    {
        // if file not accessible, send 403 response
        no_permission_request_response(sockfd, content_type);
    }
    struct tm *timeinfo = localtime(&file_stat.st_mtime);
    char *last_modified = (char *)malloc(15 * sizeof(char));
    sprintf(last_modified, "%02d%02d%02d:%02d%02d%02d",
            timeinfo->tm_mday,
            timeinfo->tm_mon + 1,
            timeinfo->tm_year % 100,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);

    if (fp == NULL)
    {
        if (errno = EACCES)
        {
            // if file not accessible, send 403 response
            no_permission_request_response(sockfd, content_type);
            return;
        }
        else
        {
            // if file not found, send 404 response
            not_found_request_response(sockfd, content_type);
            return;
        }
    }
    // read the contents and store in a character array
    // increase file size if the buffer is full
    char *file_content = (char *)malloc(BUFFER * sizeof(char));
    int i = 0;
    int j = 0;
    char ch;
    while ((ch = fgetc(fp)) != EOF)
    {
        if (j == BUFFER)
        {
            file_content = realloc(file_content, sizeof(*file_content) * (BUFFER + i));
            j = 0;
        }
        file_content[i++] = (char)ch;
        j++;
    }
    file_content[i] = '\0';
    ok_request_response(sockfd, content_type, last_modified, file_content);
    return;
}

void process_PUT_request(int newsockfd, char *path, char *content_type, char *file_content)
{
    // Get current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char *date_time = (char *)malloc(15 * sizeof(char));
    strftime(date_time, 15, "%d%m%y:%H%M%S", &tm);
    // open the file given in the relative path
    FILE *fp = fopen(path, "w");
    if (fp == NULL)
    {
        if (errno = EACCES)
        {
            // if file not found, send 404 response
            no_permission_request_response(newsockfd, content_type);
            return;
        }
        else
        {
            // if file not accessible, send 403 response
            not_found_request_response(newsockfd, content_type);
            return;
        }
    }
    fprintf(fp, "%s", file_content);
    fclose(fp);
    char *response_body = "File Created and Written successfully";
    int content_length = strlen(response_body);
    char *response = (char *)malloc(1000 * sizeof(char));
    char *expires = get_expires();
    sprintf(response, "HTTP/1.1 200 OK\r\n"
                      "Expires: %s\r\n"
                      "Cache-Control: no-store\r\n"
                      "Content-Language: en-us\r\n"
                      "Content-Length: %d\r\n"
                      "Content-Type: %s\r\n"
                      "Last-Modified: <%s>\r\n"
                      "\r\n"
                      "%s",
            expires, content_length, content_type, date_time, response_body);
    printf("%s\n", response);
    send_in_packets(newsockfd, response);
    return;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    struct sockaddr_in server_addr, client_addr;
    char buff[BUFFER];
    int client_addr_len;

    // Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error creating socket");
        return 1;
    }

    // Set the server address information
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socket to the address and port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error binding socket");
        return 1;
    }

    // Listen for incoming connections
    listen(sockfd, 5);
    printf("Listening for incoming connections on port %d...\n", PORT);

    FILE *fp_;
    fp_ = fopen("AccessLog.txt", "a");

    while (1)
    {
        // Accept incoming connections
        client_addr_len = sizeof(client_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (newsockfd < 0)
        {
            perror("Error accepting connection");
            continue;
        }

        if (fork() == 0)
        {
            close(sockfd);
            // Receive the incoming request
            char *request_ = recv_in_packets(newsockfd);

            char *body_start = strstr(request_, "\r\n\r\n");
            int request_length = body_start - request_;
            printf("Request Message Received:\n");
            for (int i = 0; i < request_length; i++) {
                printf("%c", request_[i]);  
            }
            printf("\n\n");
            
            request_[strlen(request_)] = '\0';
            char *request = (char *)malloc((strlen(request_) + 1) * sizeof(char));
            strcpy(request, request_);

            // Parse the request
            char *protocol = NULL, *method = NULL, *path = NULL, *accept_ = NULL, *host = NULL, *connection = NULL, *content_type = NULL, *content_language = NULL, *if_modified_since = NULL, *accept_language = NULL;
            char *date = NULL, *expires = NULL;
            int content_length = 0;
            char *request_body = NULL, *response_body = NULL;
            int bad_req_flag = 0;

            char *requestcopy = malloc(strlen(request) + 1);
            strcpy(requestcopy, request);

            // Parse the request
            char *line1 = strtok(request, "\r\n");
            method = strtok(line1, " ");
            path = strtok(NULL, " ");
            protocol = strtok(NULL, " ");
            if (path[0] = '/')
            {
                path = path + 1;
            }
            char *access_log = (char *)malloc(1000 * sizeof(char));
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char *date_time = (char *)malloc(15 * sizeof(char));
            strftime(date_time, 15, "%d%m%y:%H%M%S", &tm);
            strcat(access_log, date_time);
            strcat(access_log, ":");

            if (strcmp(method, "GET") == 0)
            {
                char *line = strtok(requestcopy, "\r\n");
                while (line != NULL)
                {
                    if (strncmp(line, "Host:", 5) == 0)
                    {
                        host = line + 6;
                        //Complete Access Log
                        strcat(access_log, host);
                        strcat(access_log, ":");
                        strcat(access_log, method);
                        strcat(access_log, ":");
                        strcat(access_log, path);
                        fprintf(fp_, "%s\n", access_log);
                        fclose(fp_);
                        if (host == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Connection:", 11) == 0)
                    {
                        connection = line + 12;
                        if (connection == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Accept:", 7) == 0)
                    {
                        accept_ = line + 8;
                        if (accept_ == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Accept-Language:", 16) == 0)
                    {
                        accept_language = line + 17;
                        if (accept_language == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "If-Modified-Since:", 18) == 0)
                    {
                        if_modified_since = line + 19;
                        if (if_modified_since == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Content-Language:", 17) == 0)
                    {
                        content_language = line + 18;
                        if (content_language == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Content-Length:", 15) == 0)
                    {
                        content_length = atoi(line + 16);
                    }
                    else if (strncmp(line, "Content-Type:", 13) == 0)
                    {
                        content_type = line + 14;
                    }
                    line = strtok(NULL, "\r\n");
                }
            }
            else if (strcmp(method, "PUT") == 0)
            {
                response_body = (char *)malloc((content_length + 1) * sizeof(char));
                int response_body_len = 0;
                char *line = strtok(requestcopy, "\r\n");
                while (line != NULL)
                {
                    if (strncmp(line, "Host:", 5) == 0)
                    {
                        host = line + 6;
                        //Complete Access Log
                        strcat(access_log, host);
                        strcat(access_log, ":");
                        strcat(access_log, method);
                        strcat(access_log, ":");
                        strcat(access_log, path);
                        fprintf(fp_, "%s\n", access_log);
                        fclose(fp_);
                        if (host == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Connection:", 11) == 0)
                    {
                        connection = line + 12;
                        if (connection == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Content-Length:", 15) == 0)
                    {
                        content_length = atoi(line + 16);
                        if (content_length == 0)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "Content-Type:", 13) == 0)
                    {
                        content_type = line + 14;
                        if (content_type == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strncmp(line, "PUT", 3) == 0)
                    {
                        if (path == NULL || protocol == NULL)
                        {
                            bad_req_flag = 1;
                            break;
                        }
                    }
                    else if (strlen(line) > 0)
                    {
                        if (strcmp(line, "\r\n") == 0)
                        {
                            continue;
                        }
                        else
                        {
                            strcpy(response_body + response_body_len, line);
                            response_body_len += strlen(line);
                            strcpy(response_body + response_body_len, "\n");
                            response_body_len++;
                        }
                    }
                    line = strtok(NULL, "\r\n");
                }
                response_body[--response_body_len ] = '\0';
                if (response_body_len != content_length)
                {
                    bad_req_flag = 1;
                }
            }
            else
            {
                bad_req_flag = 1;
            }
            if (bad_req_flag == 1)
            {
                bad_request_response(newsockfd, content_type);
            }
            else if (strcmp(method, "GET") == 0)
            {
                process_GET_request(newsockfd, path, content_type);
            }
            else if (strcmp(method, "PUT") == 0)
            {
                process_PUT_request(newsockfd, path, content_type, response_body);
            }
            close(newsockfd);
            exit(0);
        }
        // Close the socket
        close(newsockfd);
    }
    return 0;
}