#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "rpc_pal.h"

/**
 * socket related below
 */
static int fill_sockaddr_in(struct sockaddr_in *addr, const char *nameorip, int port)
{
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    // Lookup the IP using the hostname provided
    struct hostent *host = gethostbyname(nameorip);
    if (host == NULL)
    {
        return -1;
    }
    memcpy(&addr->sin_addr, host->h_addr_list[0], host->h_length);
    return 0;
}

int rpc_create_server(int server_port, int listen_number)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server_port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);

    ret = listen(sockfd, listen_number);
    assert(ret != -1);

    LOG("create rpc server on port %d\n", server_port);
    return sockfd;
}

int rpc_create_client(const char *server_nameorip, int server_port, int recv_timeout_s)
{
    struct sockaddr_in server_addr;
    fill_sockaddr_in(&server_addr, server_nameorip, server_port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    struct timeval timeout;
    timeout.tv_sec = recv_timeout_s;
    timeout.tv_usec = 0;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    assert(ret != -1);

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        perror("connect error");
        return -1;
    }
    LOG("create rpc client\n");
    return sockfd;
}

void rpc_close(int sockfd)
{
    if (sockfd >= 0)
        close(sockfd);
}

void rpc_malloc(return_type *rt, size_t size)
{
    if (rt != NULL)
    {
        rt->return_size = size;
        rt->return_val = (void *)malloc(size);
        rt->need_free = true;
    }
}
void rpc_free(return_type *rt)
{
    if (rt != NULL)
    {
        free(rt->return_val);
        rt->return_val = NULL;
        rt->return_size = 0;
        rt->need_free = false;
    }
}

/**
 * Serializes int values into a character buffer.
 */
unsigned char *int_serialize(unsigned char *buffer, int value)
{

    int shift = 0;
    const int shift_eight = 8;
    const int shift_sixteen = 16;
    const int shift_twentyfour = 24;

    buffer[shift] = value >> shift;
    buffer[++shift] = value >> shift_eight;
    buffer[++shift] = value >> shift_sixteen;
    buffer[++shift] = value >> shift_twentyfour;

    return buffer + shift + 1;
}

/**
 * used for debug
 */

// statically allocated struct which might be overwritten.
static char *get_local_time()
{
    static char buf[20] = {0};
    time_t tt;
    tzset();//设置时间环境变量-时区
    tt = time(NULL);//time(&tt);
    struct tm *local_time =  localtime(&tt);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", 
            local_time->tm_year+1900, local_time->tm_mon+1, local_time->tm_mday,
            local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
    return (char *)&buf[0];
}

void print_array(const char *sender, const char *buf, int size)
{
#ifdef ADD_TIMESTAMP
    printf("[%s] ", get_local_time());
#endif
    printf("%s: ", sender);

    int i;
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

void print(const char *sender, const char *fmt, ...)
{
#ifdef ADD_TIMESTAMP
    printf("[%s] ", get_local_time());
#endif
    printf("%s: ", sender);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}