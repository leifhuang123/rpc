#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "epoll.h"

/* set fd to nonblock */
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void epoll_addfd(int epollfd, int fd, bool oneshot)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (oneshot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void epoll_delfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
}

void epoll_reset_oneshot(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

ssize_t sendall(int sockfd, const unsigned char *buf, size_t len, int flags)
{
    size_t sent = 0;
    while (sent < len)
    {
        ssize_t tmp = send(sockfd, buf + sent, len - sent, flags);
        if (tmp < 1)
        {
            return -1;
        }
        sent += (size_t)tmp;
    }
    return sent;
}

ssize_t recvall(int sockfd, unsigned char *buf, size_t bufsz, int flags)
{
    const unsigned char *const start = buf;
    ssize_t rv;
    do
    {
        rv = recv(sockfd, buf, bufsz, flags);
        if (rv > 0)
        {
            /* successfully read bytes from the socket */
            buf += rv;
            bufsz -= rv;
        }
        else if (rv < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            /* an error occurred that wasn't "nothing to read". */
            return -1;
        }
    } while (rv > 0);

    return buf - start;
}

/** Notice:
 * in epoll et mode, all sockets are non-blocking,
 * so accept/recv/send must in a loop to ensure the operation finished.
 * if acception is not dealt with properly, clients may not connect to the server.
 */
