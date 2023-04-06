#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/poll.h>

#define BUFFER 100
#define PORT 8012

// Get the port number from the URL
int parse_port(char *url) {
    int port = 80;
    char *colon = strrchr(url, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }
    if(port == 0) {
        port = 80;
    }
    return port;
}

// Get ip address from the URL
char *parse_ip(char *url) {
    char *ip = malloc(16);
    char *colon = strchr(url, ':');
    if (colon) {
        *colon = '\0';
    }
    strcpy(ip, url);
    return ip;
}

// Handles the status code
void handle_status_code(int status_code) {
    switch (status_code) {
        case 200:
            printf("Successful request\n");
            break;
        case 400:
            printf("Bad request\n");
            break;
        case 403:
            printf("Forbidden\n");
            break;
        case 404:
            printf("Not found\n");
            break;
        default:
            printf("Unknown error: %d\n", status_code);
            break;
    }
    printf("\n");
}

// Get the request from the URL
char* get_request_from_url(const char* url, int port) {
    char host[100];
    char path[100];

    char* pos = strstr(url, "://");
    if (pos == NULL) {
        return NULL;
    }
    pos += 3;
    int i = 0;
    while (*pos != '/' && *pos != '\0' && *pos != ':') {
        host[i++] = *pos++;
    }
    host[i] = '\0';
    i = 0;
    while (*pos != '\0') {
        path[i++] = *pos++;
    }
    path[i] = '\0';

    char* ext = strrchr(path, '.');
    if (ext != NULL) {
        ext++;
        char* port_str = strstr(ext, ":");
        if (port_str != NULL) {
            *port_str = '\0';
        }
    }

    time_t now;
    time(&now);
    now -= 2 * 24 * 60 * 60;
    struct tm* tm_now = gmtime(&now);
    char if_modified_since[100];
    strftime(if_modified_since, sizeof(if_modified_since), "%d%m%y:%H%M%S", tm_now);

    char* content_type = "text/*";
    if (ext != NULL) {
        if (strcmp(ext, "html") == 0) {
            content_type = "text/html";
        } else if (strcmp(ext, "pdf") == 0) {
            content_type = "application/pdf";
        } else if (strcmp(ext, "jpg") == 0) {
            content_type = "image/jpeg";
        }
    }

    char* request;
    request = malloc(1000 * sizeof(char));
    sprintf(request, "GET %s HTTP/1.1\r\n"
                     "Host: %s:%d\r\n"
                     "Connection: close\r\n"
                     "Accept: %s\r\n"
                     "Accept-Language: en-us,en;q=0.8\r\n"
                     "If-Modified-Since: %s\r\n"
                     "Content-Language: en-us\r\n"
                     "Content-Length: 0\r\n"
                     "Content-Type: %s\r\n"
                     "\r\n", path, host, port, content_type, if_modified_since, content_type);
    return strdup(request);
}

// Get file path from the response
char *get_request_to_file(char *response) {
    char *responsecopy = strdup(response);
    char *responsecopy2 = strdup(response);

    // Get the status code
    char *status_line = strtok(response, "\r\n");
    char *status_code_str = strtok(status_line, " ");
    int status_code = atoi(status_code_str + 9);
    char *status_message = strtok(NULL, "\r\n");
    printf("Status : ");
    handle_status_code(status_code);

    if(status_code != 200) {
        return NULL;
    }

    char *content_type = NULL;
    int content_length = 0;
    char *last_modified = NULL;

    char *line = strtok(responsecopy, "\r\n");
    
    while (line != NULL) {
        if (strncmp(line, "Content-Type:", 13) == 0) {
            content_type = line + 14;
        } 
        else if (strncmp(line, "Content-Length:", 15) == 0) {
            content_length = atoi(line + 16);
        } 
        else if (strncmp(line, "Last-Modified:", 14) == 0) {
            last_modified = line + 15;
        }
        line = strtok(NULL, "\r\n");
    }

    char *extension = strrchr(content_type, '/');
    char *file_path = (char *) malloc(100);
    strcpy(file_path, "output_file");

    char *content_start = responsecopy2 + strlen(responsecopy2) - content_length;
    char *content_body = strndup(content_start, content_length);

    if (strcmp(extension, "/html") == 0) {
        strcat(file_path, ".html");
    } else if (strcmp(extension, "/pdf") == 0) {
        strcat(file_path, ".pdf");
    } else if (strcmp(extension, "/jpeg") == 0) {
        strcat(file_path, ".jpg");
    } else {
        strcat(file_path, ".txt");
    }

    FILE *file = fopen(file_path, "w");
    fwrite(content_body, sizeof(char), content_length, file);
    fclose(file);

    return file_path;
}

// Put request from url 
char *put_request_from_url(char *url, char *file_path, int port) {
    char buffer[1000];
    char *content_type = NULL;
    char *extension = strrchr(file_path, '.');
    char *request = NULL;
    char host[100];
    char path[100];

    char* pos = strstr(url, "://");
    if (pos == NULL) {
        return NULL;
    }
    pos += 3;
    int i = 0;
    while (*pos != '/' && *pos != '\0' && *pos != ':') {
        host[i++] = *pos++;
    }
    host[i] = '\0';
    i = 0;
    while (*pos != '\0') {
        path[i++] = *pos++;
    }

    char* port_str = strstr(path, ":");
    if (port_str != NULL) {
        *port_str = '\0';
    }

    strcat(path, "/");
    strcat(path, file_path);


    if (extension == NULL) {
        content_type = "text/*";
    } else if (strcmp(extension, ".html") == 0) {
        content_type = "text/html";
    } else if (strcmp(extension, ".pdf") == 0) {
        content_type = "application/pdf";
    } else if (strcmp(extension, ".jpg") == 0) {
        content_type = "image/jpeg";
    } else {
        content_type = "text/*";
    }

    int content_length = 0;
    char *content_body = NULL;
    FILE *file = fopen(file_path, "r");
    if (file != NULL) {
        fseek(file, 0, SEEK_END);
        content_length = ftell(file);
        fseek(file, 0, SEEK_SET);
        content_body = malloc(content_length + 1);
        fread(content_body, content_length, 1, file);
        fclose(file);
    }

    sprintf(buffer, "PUT %s HTTP/1.1\r\n"
                 "Host: %s:%d\r\n"
                 "Connection: close\r\n"
                 "Content-Length: %d\r\n"
                 "Content-Type: %s\r\n"
                 "\r\n", path, host, port, content_length - 2, content_type);

    request = malloc(strlen(buffer) + content_length + 1);
    strcpy(request, buffer);
    if (content_body != NULL) {
        strcat(request, content_body);
    }
    request[strlen(request)] = '\0';
    return request;
}

// Parse the response
void put_request_response(char *response) {
    char *status_line = strtok(response, "\r\n");
    char *status_code_str = strtok(status_line, " ");
    int status_code = atoi(status_code_str + 9);
    char *status_message = strtok(NULL, "\r\n");
    printf("Status : ");
    handle_status_code(status_code);
    return;
}


// Receive the response from the server
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
            if(buff[j] == '\0')
                break;
        }
        str = realloc(str, sizeof(*str) * (size += BUFFER));
    } while (buff[BUFFER - 1] != '\0');
    free(buff);
    return str;
}

// Send the request to the server
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
}


// Open the file in the appropriate application
void open_file(char *file_path) {
    pid_t pid = fork();
    if (pid == -1) {
        // Error in fork
        return;
    } else if (pid == 0) {
        // Child process
        char *extension = strrchr(file_path, '.');
        if (!extension) {
            char *args[] = {"gedit", file_path, NULL};
            execvp("gedit", args);
            return;
        }
        if (strcmp(extension, ".pdf") == 0) {
            char *args[] = {"evince", file_path, NULL};
            execvp("evince", args);
            return;
        }
        if (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("Current working dir: %s\n", cwd);
            } else {
                perror("getcwd() error");
            }
            strcat(cwd, "/");
            strcat(cwd, file_path);
            char *args[] = {"google-chrome", cwd, NULL};
            execvp("google-chrome", args);
            return;
        }
        if (strcmp(extension, ".jpeg") == 0 || strcmp(extension, ".jpg") == 0) {
            char *args[] = {"display", file_path, NULL};
            execvp("display", args);
            return;
        }
        char *args[] = {"gedit", file_path, NULL};
        execvp("gedit", args);
    } else {
        // Parent process
        wait(NULL);
    }
}

int main()
{
	int	sockfd ;
	struct sockaddr_in	serv_addr;

    int i;
	char buf[100];

    struct pollfd fds[2];
    int timeout = (3 * 1000);

    printf("\n");

    while(1)
    {
        printf("MyOwnBrowser> ");

        char *input = NULL;
        size_t input_size = 0;
        size_t input_len = 0;

        // Get input from user
        while(1){
            input_size += BUFFER;
            input = realloc(input, input_size);

            if (!input) {
            perror("Error allocating memory");
            exit(1);
            }

            fgets(input + input_len, BUFFER, stdin);
            input_len = strlen(input);

            if (input[input_len - 1] == '\n') {
                input[input_len - 1] = '\0';
                break;
            }
        }

        // Exit client program
        if(strcmp(input, "QUIT") == 0)
            break;

        // GET or PUT request
        char *token = strtok(input, " ");

        if(strcmp(token, "GET") == 0)
        {
            // GET request

            // Parse the URL
            token = strtok(NULL, " ");
            char *url = token;
            char *url_to_convert = strdup(url);

            int port = parse_port(url);
            char *ip = parse_ip(url);
            if(!ip) {
                ip = "127.0.0.1";
            }

            // Create socket
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Unable to create socket\n");
            exit(0);
            }

            // Connect to server

            serv_addr.sin_family	= AF_INET;
            inet_aton("127.0.0.1", &serv_addr.sin_addr);
            serv_addr.sin_port	= htons(PORT);

            if ((connect(sockfd, (struct sockaddr *) &serv_addr,
                                sizeof(serv_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }

            // Initialize the pollfd structure
            memset(fds, 0 , sizeof(fds));
            fds[1].fd = sockfd;
            fds[1].events = POLLIN;

            for(i=0; i < 100; i++) 
                buf[i] = '\0';

            // Create the request message
            char *request = get_request_from_url(url_to_convert, port);

            if (request == NULL) {
                printf("Invalid URL\n");
                continue;
            }

            printf("\nRequest Message Sent:\n%s\n", request);

            // Send the request to the server
            send_in_packets(sockfd, request);

            // Wait for response from server
            int ret = poll(fds, 2, timeout);

            if (ret == -1) {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            // Exit if timeout occurs
            if (ret == 0) {
                printf("Timeout occurred!  No data after %d seconds.\n", (timeout/1000));
                close(sockfd);
                break;
            }

            // Receive the response from the server
            char *response = recv_in_packets(sockfd);

            // Print the response message
            char *body_start = strstr(response, "\r\n\r\n");
            int response_length = body_start - response;
            printf("Response Message Received:\n");
            for (int i = 0; i < response_length; i++) {
                printf("%c", response[i]);  
            }
            printf("\n\n");

            // Get the file path from the response
            char *file_path = get_request_to_file(response);
            if(file_path) {
                open_file(file_path);
            }

            free(file_path);

            // Close the socket
            close(sockfd);
        }
        else if (strcmp(token, "PUT") == 0)
        {
            // PUT request

            // Parse the URL
            token = strtok(NULL, " ");
            char *url = strdup(token);
            char *url_to_convert = strdup(token);

            token = strtok(NULL, " ");
            char *file_path = malloc(strlen(token) + 1);
            strcpy(file_path, token);

            int port = parse_port(url);
            char *ip = parse_ip(url);

            if(!ip) {
                ip = "80";
            }

            // Create socket
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("Unable to create socket\n");
                exit(0);
            }

            // Connect to server
            serv_addr.sin_family	= AF_INET;
            inet_aton("127.0.0.1", &serv_addr.sin_addr);
            serv_addr.sin_port	= htons(PORT);

            if ((connect(sockfd, (struct sockaddr *) &serv_addr,
                                sizeof(serv_addr))) < 0) {
                perror("Unable to connect to server\n");
                exit(0);
            }

            // Initialize the pollfd structure
            memset(fds, 0 , sizeof(fds));
            fds[1].fd = sockfd;
            fds[1].events = POLLIN;

            for(i=0; i < 100; i++) 
                buf[i] = '\0';

            // Create the request message
            char *request = put_request_from_url(url_to_convert, file_path, port);

            if (request == NULL) {
                printf("Invalid URL\n");
                continue;
            }

            // Print the request message
            char *body_start = strstr(request, "\r\n\r\n");
            int request_length = body_start - request;
            printf("\nRequest Message Sent:\n");
            for (int i = 0; i < request_length; i++) {
                printf("%c", request[i]);
            }
            printf("\n\n");            

            // Send the request to the server
            send_in_packets(sockfd, request);

            // Wait for response from server
            int ret = poll(fds, 2, timeout);

            if (ret == -1) {
                perror("poll");
                exit(EXIT_FAILURE);
            }

            // Exit if timeout occurs
            if (ret == 0) {
                printf("Timeout occurred!  No data after %d seconds.\n", (timeout/1000));
                break;
            }

            // Receive the response from the server
            char *response = recv_in_packets(sockfd);   

            // Print the response message
            printf("Response Message Received:\n%s\n", response);

            // Parse the response message
            put_request_response(response);

            // Close the socket
            close(sockfd);

        }
        else {
            printf("Invalid command\n");
        }
    }

	return 0;
}
