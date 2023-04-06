#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    while (i < strlen(expr))
    {
        if (expr[i] == ' ')
            i++;
        else if (expr[i] == '(')
        {
            i++;
            char *expr2 = (char *)malloc(strlen(expr)*sizeof(char));
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
    char *expr;
    unsigned long int len;
    // printf("Enter the length of the expression:\n");
    // scanf("%ld", &len);
    expr = (char *)malloc(len*sizeof(char));

    printf("Enter the expression:\n");
    getline(&expr, &len, stdin);
    // printf("done..\n\n");
    // scanf("%s", expr);

    printf("%f\n", evaluate_expression(expr));
}