#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
// Server
double operator(char op, double num1, double num2)
{
	switch (op)
	{
	case '+':
		return (num1 + num2);
		break;
	case '-':
		return (num1 - num2);
		break;
	case '*':
		return (num1 * num2);
		break;
	case '/':
		return (num1 / num2);
		break;
	}
	return 0.0;
}

double evaluate_expression(char *expr)
{
	double num1 = 0, num2 = 0;
	int index = 0;
	float temp = 0;
	while ((expr[index] >= '0' && expr[index] <= '9') || expr[index] == '.' || expr[index] == ' ')
	{
		while (expr[index] == ' ')
			index++;
		if (expr[index] >= '0' && expr[index] <= '9')
			temp = temp * 10 + expr[index++] - '0';
		if (expr[index] == '.')
		{
			index++;
			float x = 1;
			while ((expr[index] >= '0' && expr[index] <= '9')) // assuming no spaces in the middle of a number
			{
				x *= 10;
				temp += (expr[index++] - '0') / x;
			}
		}
	}
	num1 = temp;
	int i = index;
	char op;
	while (i < strlen(expr) && expr[i]!='\0')
	{
		if (expr[i] == ' ')
			i++;
		else if (expr[i] == '(')
		{
			i++;
			char *expr2 = (char *)malloc(strlen(expr) * sizeof(char));
			for (int j = 0;; j++)
			{
				expr2[j] = expr[i++];
				if (expr2[j] == ')')
				{
					expr2[j] = '\0';
					break;
				}
			}
			num2 = evaluate_expression(expr2);
			num1 = operator(op, num1, num2);
		}
		else if (expr[i] == '/' || expr[i] == '*' || expr[i] == '+' || expr[i] == '-' || expr[i] == '%')
			op = expr[i++];
		else if ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.')
		{
			temp = 0;
			while ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.')
			{
				if (expr[i] >= '0' && expr[i] <= '9')
					temp = temp * 10 + expr[i++] - '0';
				if (expr[i] == '.')
				{
					i++;
					float x = 1;
					while ((expr[i] >= '0' && expr[i] <= '9'))
					{
						x *= 10;
						temp += (expr[i++] - '0') / x;
					}
				}
			}
			num2 = temp;
			num1 = operator(op, num1, num2);
		}
		else
			i++;
	}
	return num1;
}

int main()
{
	int sockfd, newsockfd; /* Socket descriptors */
	int clilen;
	double ans;
	struct sockaddr_in cli_addr, serv_addr;

	int i;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
	}
	else
	{
		printf("Socket created\n");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(12000);
	if (bind(sockfd, (struct sockaddr *)&serv_addr,
			 sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}
	else
	{
		printf("Address bound\n");
	}

	listen(sockfd, 5);
	while (1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
						   &clilen);

		if (newsockfd < 0)
		{
			perror("Accept error\n");
			exit(0);
		}
		else
		{
			printf("Request Accepted!\n");
		}

		char *buff = (char *)malloc(100 * sizeof(char));
		for(int i=0; i<100; i++)
			buff[i] = '\0';
		recv(newsockfd, buff, 100, 0);
		printf("Expression recieved: %s\n", buff);
		// calculate the expression and then save to double ans
		ans = evaluate_expression(buff);
		send(newsockfd, &ans, sizeof(ans), 0);
		close(newsockfd);
	}
	return 0;
}
