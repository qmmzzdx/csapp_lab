/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void)
{
    char *buf, *p, calc[MAXLINE];
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL)
    {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    /* Make the response body */
    strcat(content, "Welcome to add.com: ");
    strcat(content, "THE Internet addition portal.\r\n<p>");
    strcat(content, "The answer is:");
    sprintf(calc, "%d + %d = %d\r\n<p>", n1, n2, n1 + n2);
    strcat(content, calc);
    strcat(content, "Thanks for visiting!\r\n");

    /* Generate the HTTP response */
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
/* $end adder */
