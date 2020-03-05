#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "rpc_pal.h"
#include "threadpool.h"
#include "epoll.h"

// Database structure to store procedure properties
struct proc_map_db
{
    const char *proc_name;
    int n_params;
    fp_type fp;
};

// Database declaration
static struct proc_map_db proc_db[RPC_PROC_DB_SIZE];
static int proc_db_index = 0;

/**
 * Registers a new procedure to the procedure database.
 */
bool register_procedure(const char *procedure_name, const int nparams, fp_type fnpointer)
{
    if (proc_db_index >= RPC_PROC_DB_SIZE)
    {
        LOG("procedure is full\n");
        return false;
    }

    int i = 0;
    while (i < proc_db_index)
    {
        if ((strcmp(proc_db[i].proc_name, procedure_name) == 0) && (nparams == proc_db[i].n_params))
        {
            LOG("procedure already exits\n");
            return false;
        }
        i++;
    }

    proc_db[proc_db_index].proc_name = procedure_name;
    proc_db[proc_db_index].n_params = nparams;
    proc_db[proc_db_index].fp = fnpointer;
    proc_db_index++;

    return true;
}

/**
 * Deserializes buffer received from client.
 */
return_type deserialize(unsigned char *buffer)
{
    return_type ret;
    ret.return_val = NULL;
    ret.return_size = 0;

    // Gets size of procedure name
    int proc_size = *(int *)buffer;
    unsigned char *aliased_buff = buffer + sizeof(int);

    if (proc_size <= 1)
    {
        LOG("\nProcedure size was zero. Ret not set.\n");
        return ret;
    }

    // Gets actual procedure name
    char recv_proc_name[proc_size];
    memset(recv_proc_name, 0, sizeof(recv_proc_name));
    char *dummyaliased_buff = (char *)aliased_buff;

    int i = 0;
    while (i < proc_size)
    {
        recv_proc_name[i] = dummyaliased_buff[i];
        i++;
    }

    aliased_buff += proc_size;

    // Gets number of parameters for procedure
    int recv_num_args = *(int *)aliased_buff;
    aliased_buff = aliased_buff + 4;

    // Find the procedure from the database
    fp_type fp;
    arg_type *arg_list;
    int proc_found = 0;

    i = 0;
    while (i < proc_db_index)
    {
        if (strcmp(proc_db[i].proc_name, recv_proc_name) == 0 &&
            proc_db[i].n_params == recv_num_args)
        {
            fp = proc_db[i].fp;
            proc_found = 1;
        }
        i++;
    }

    // Proceeds to get arguments if procedure was found in database
    if (proc_found == 1)
    {
        arg_type *head_node = (arg_type *)malloc(sizeof(arg_type));
        arg_type *curr_node = head_node;

        // Build linked list of arguments as we unpack from buffer
        i = 0;
        while (i < recv_num_args)
        {
            int recv_arg_size = *(int *)aliased_buff;

            aliased_buff += sizeof(int);

            unsigned char *recv_arg_val = (unsigned char *)malloc(recv_arg_size);

            int j = 0;
            while (j < recv_arg_size)
            {
                recv_arg_val[j] = aliased_buff[j];
                j++;
            }

            arg_type *ptr = (arg_type *)malloc(sizeof(arg_type));
            ptr->next = NULL;

            curr_node->arg_size = recv_arg_size;
            curr_node->arg_val = (void *)recv_arg_val;
            curr_node->next = ptr;
            curr_node = ptr;

            aliased_buff += recv_arg_size;
            i++;
        }

        arg_list = head_node;

        // Call function ptr to compute and return result
        ret = fp(recv_num_args, arg_list);

        // free
        arg_type *p = NULL;
        while (arg_list != NULL)
        {
            p = arg_list;
            arg_list = arg_list->next;
            free(p);
        }
    }
    else
    {
        LOG("\nCould not find function in database. Ret not set.\n");
    }

    return ret;
}

typedef struct fds
{
    int epollfd;
    int connfd;
} fds_t;

static void worker(void *arg)
{
    fds_t *in_fds = (fds_t *)arg;
    assert(in_fds != NULL);

    int connfd = in_fds->connfd;
    int epollfd = in_fds->epollfd;
    LOG("new worker on fd %d\n", connfd);

    unsigned char buf[RPC_BUFFER_SIZE];

    while (1)
    {
        memset(buf, 0, sizeof(buf));
        int ret = recv(connfd, buf, RPC_BUFFER_SIZE, 0);
        if (ret < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                epoll_reset_oneshot(epollfd, connfd);
                break;
            }
        }
        else if (ret == 0)
        {
            close(connfd);
            LOG("connection on fd %d closed\n", connfd);
            break;
        }
        else
        {
            return_type ret;

            ret = deserialize(buf);

            // Process and send result back to client
            unsigned char *tmp = int_serialize(buf, ret.return_size);
            if (ret.return_size > 0)
                memcpy(tmp, ret.return_val, ret.return_size);

            sendall(connfd, buf, ret.return_size + sizeof(int), 0);
            LOGA("send", buf, ret.return_size + sizeof(int));

            // Notice: 可能是因为写时复制，所以在send之后才能释放，否则对方只收到return_size而没有return_val
            if (ret.need_free)
                rpc_free(&ret);
        }
    }
    free(in_fds);
    LOG("end worker on fd %d\n", connfd);
}

/**
 * Main loop that keeps server running and processing incoming procedure calls.
 */
#define MAX_EVENT_NUMBER    1024

void launch_server(int thread_number, int listen_number)
{
    // Creates a TCP socket
    int listenfd = rpc_create_server(RPC_SERVER_PORT, listen_number);

    // create thread pool
    threadpool_t *pool = threadpool_create(thread_number);

    // epoll register
    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(listen_number);
    epoll_addfd(epollfd, listenfd, false);

    while (1)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0)
        {
            perror("epoll failed");
            break;
        }
        int i;
        for (i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                (events[i].events & EPOLLRDHUP))
            {
                LOG("close connection on fd %d\n", sockfd);  //在这里关闭socket，避免再次触发EPOLLIN而进入线程池
                close(sockfd);
                continue;
            }
            else if (sockfd == listenfd)
            {
                while (1)
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_addrlen = sizeof(client_addr);
                    int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
                    if (connfd == -1)
                    {
                        break;
                    }
                    epoll_addfd(epollfd, connfd, true);
                    LOG("new connection on fd %d\n", connfd);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                LOG("EPOLLIN on fd %d\n", sockfd);
                fds_t *new_fds = (fds_t *)malloc(sizeof(fds_t));
                new_fds->epollfd = epollfd;
                new_fds->connfd = sockfd;
                threadpool_append(pool, worker, new_fds);
            }
            else
            {
                LOG("something else happened\n");
            }
        }
    }
    threadpool_destroy(pool);
    close(epollfd);
    close(listenfd);
}
