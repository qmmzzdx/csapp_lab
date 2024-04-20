#include "csapp.h"
#define NTHREADS 4
#define SBUFSIZE 16
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE (MAX_CACHE_SIZE / MAX_OBJECT_SIZE)

struct Uri
{
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
};

typedef struct
{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

typedef struct
{
    int used;
    char uri[MAXLINE];
    char server_message[MAX_OBJECT_SIZE];
} Cache;

int read_cache(int connfd, const char *uri);
void write_cache(const char *uri, const char *message);
void *thread(void *vargp);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);
void doit(int connfd);
void set_request_header(rio_t *rio, char *header, char *host);
void parse_uri(char *uri, struct Uri *uri_data);

static sbuf_t sbuf;
static int lrup;
static int readcnt;
static sem_t w;
static sem_t mutex;
static Cache data[MAX_CACHE];
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    pthread_t tid;
    int i, listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    Signal(SIGPIPE, SIG_IGN);
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    for (i = 0; i < NTHREADS; ++i)
    {
        Pthread_create(&tid, NULL, thread, NULL);
    }
    Sem_init(&w, 0, 1);
    Sem_init(&mutex, 0, 1);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }

    return 0;
}

int read_cache(int connfd, const char *uri)
{
    int i, flag = -1;
    P(&mutex);
    if (++readcnt == 1)
    {
        P(&w);
    }
    V(&mutex);
    for (i = 0; i < MAX_CACHE; ++i)
    {
        if (!strcmp(data[i].uri, uri))
        {
            Rio_writen(connfd, data[i].server_message, sizeof(data[i].server_message));
            printf("Proxy sends %d bytes to client.\n", (int)strlen(data[i].server_message));
            data[i].used = 1;
            flag = 1;
            break;
        }
    }
    P(&mutex);
    if (--readcnt == 0)
    {
        V(&w);
    }
    V(&mutex);
    return flag;
}

void write_cache(const char *uri, const char *message)
{
    int i;
    
    P(&w);
    while (data[lrup].used != 0)
    {
        data[lrup].used = 0;
        lrup = (lrup + 1) % MAX_CACHE;
    }
    i = lrup;
    data[i].used = 1;
    strcpy(data[i].uri, uri);
    strcpy(data[i].server_message, message);
    V(&w);
}

void *thread(void *vargp)
{
    Pthread_detach(pthread_self());

    while (1)
    {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

void doit(int connfd)
{
    ssize_t n, size_buf;
    int i, serverfd;
    rio_t rio, rio_server;
    char buf[MAXLINE], header[MAXLINE];
    char cache_uri[MAXLINE], cache_message[MAX_OBJECT_SIZE];
    char method[MAXLINE], uri[MAXLINE * 4], version[MAXLINE];

    Rio_readinitb(&rio, connfd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
    {
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(cache_uri, uri);
    if (strcasecmp(method, "GET"))
    {
        printf("Proxy doesn't implement the method.\n");
        return;
    }
    if (!strcmp(version, "HTTP/1.1"))
    {
        strcpy(version, "HTTP/1.0");
    }
    struct Uri *uri_data = (struct Uri *)Malloc(sizeof(struct Uri));
    if ((i = read_cache(connfd, uri)) != -1)
    {
        return;
    }
    parse_uri(uri, uri_data);
    sprintf(uri, "%s %s %s\r\n", method, uri_data->path, version);
    strcpy(header, uri);
    set_request_header(&rio, header, uri_data->host);

    serverfd = Open_clientfd(uri_data->host, uri_data->port);
    Rio_readinitb(&rio_server, serverfd);
    Rio_writen(serverfd, header, strlen(header));
    memset(cache_message, 0, sizeof(cache_message));
    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0)
    {
        size_buf += n;
        if (size_buf < MAX_OBJECT_SIZE)
        {
            strcat(cache_message, buf);
        }
        Rio_writen(connfd, buf, n);
        printf("Proxy receives %d bytes to client.\n", (int)n);
    }
    if (size_buf < MAX_OBJECT_SIZE)
    {
        write_cache(cache_uri, cache_message);
    }
    Free(uri_data);
    Close(serverfd);
}

void set_request_header(rio_t *rio, char *header, char *host)
{
    char buf[MAXLINE], others[MAXLINE];

    memset(others, 0, sizeof(others));
    while (Rio_readlineb(rio, buf, MAXLINE) > 0 && strcmp(buf, "\r\n") != 0)
    {
        printf("%s", buf);
        if (strstr(header, "Host:") != NULL || strstr(header, "User-Agent:") != NULL)
        {
            continue;
        }
        if (strstr(header, "Connection:") != NULL || strstr(header, "Proxy-Connection:") != NULL)
        {
            continue;
        }
        strcat(others, buf);
    }
    strcat(header, "Host: ");
    strcat(header, host);
    strcat(header, "\r\n");
    strcat(header, user_agent_hdr);
    strcat(header, "Connection: false\r\n");
    strcat(header, "Proxy-Connection: false\r\n");
    if (strlen(others) > 0)
    {
        strcat(header, others);
    }
    strcat(header, "\r\n");
    printf("%s\n", header);
}

void parse_uri(char *uri, struct Uri *uri_data)
{
    const char *hostp = strstr(uri, "//");

    if (hostp == NULL)
    {
        const char *pathp = strstr(uri, "/");
        if (pathp != NULL)
        {
            strcpy(uri_data->path, pathp);
        }
        strcpy(uri_data->port, "80");
    }
    else
    {
        char *portp = strstr(hostp + 2, ":");
        if (portp != NULL)
        {
            int uri_port;
            sscanf(portp + 1, "%d%s", &uri_port, uri_data->path);
            sprintf(uri_data->port, "%d", uri_port);
            *portp = '\0';
        }
        else
        {
            char *pathp = strstr(hostp + 2, "/");
            if (pathp != NULL)
            {
                strcpy(uri_data->path, pathp);
                strcpy(uri_data->port, "80");
                *pathp = '\0';
            }
        }
        strcpy(uri_data->host, hostp + 2);
    }
}
