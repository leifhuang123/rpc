#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include "rpc_types.h"

int create_udp_server(int port)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sockfd >= 0);
    
    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    assert(ret != -1);

    return sockfd;
}

int fill_sockaddr_in(struct sockaddr_in *addr, const char *nameorip, int port)
{
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    // Lookup the IP using the hostname provided
    struct hostent *host = gethostbyname(nameorip);
    if (host != NULL)
    {
        memcpy(&addr->sin_addr, host->h_addr_list[0], host->h_length);
        return 0;
    }
    return -1;
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

void rpc_free(return_type *rt)
{
    if (rt != NULL)
    {
        printf("rpc_free called\n");
        free(rt->return_val);
        rt->return_val = NULL;
        rt->return_size = 0;
        rt->need_free = false;
    }
}

void print_array(const char *sender, const char *buf, int size)
{
    int i;
    if (sender)
        printf("%s:\n", sender);
        
    for (i=0; i<size; ++i){
        printf("%02X ", buf[i]);
    }
    printf("\n");
}
