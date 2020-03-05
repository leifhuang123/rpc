#ifndef _EPOLL_H
#define _EPOLL_H

#include <stdbool.h>
#include <sys/epoll.h>

int setnonblocking(int fd);

void epoll_addfd(int epollfd, int fd, bool oneshot);

void epoll_delfd(int epollfd, int fd);

void epoll_reset_oneshot(int epollfd, int fd);

ssize_t sendall(int sockfd, const unsigned char *buf, size_t len, int flags);

ssize_t recvall(int sockfd, unsigned char *buf, size_t bufsz, int flags);

#endif
