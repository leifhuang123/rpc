#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rpc_types.h"

// Database structure to store procedure properties
struct proc_map_db
{
    const char *proc_name;
    int n_params;
    fp_type fp;
};

// Database declaration
#define RPC_PROC_DB_SIZE 100
static struct proc_map_db proc_db[RPC_PROC_DB_SIZE];
static int proc_db_index = 0;

/**
 * Registers a new procedure to the procedure database.
 */
bool register_procedure(const char *procedure_name, const int nparams, fp_type fnpointer)
{
    if (proc_db_index >= RPC_PROC_DB_SIZE)
    {
        printf("procedure is full\n");
        return false;
    }

    int i = 0;
    while (i < proc_db_index)
    {
        if ((strcmp(proc_db[i].proc_name, procedure_name) == 0) && (nparams == proc_db[i].n_params))
        {
            printf("procedure already exits\n");
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
        printf("\nProcedure size was zero. Ret not set.\n");
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
        printf("\nCould not find function in database. Ret not set.\n");
    }

    return ret;
}

/**
 * Main loop that keeps server running and processing incoming procedure calls.
 */
void launch_server()
{
    unsigned char buffer[RPC_BUFFER_SIZE];
    int received_size;

    // Creates a UDP socket
    int sockfd = create_udp_server(RPC_SERVER_PORT);

    for (;;)
    {
        memset(buffer, 0, sizeof(buffer));

        struct sockaddr_in remote_addr;
        socklen_t addr_len = sizeof(remote_addr);
        // Populate buffer with data from client
        received_size = recvfrom(sockfd, (void *)buffer, RPC_BUFFER_SIZE,
                                 0, (struct sockaddr *)&remote_addr, &addr_len);

        // If we recieved data from client, move onto deserializing it
        if (received_size > 0)
        {
            return_type ret;
            ret = deserialize(buffer);

            // Process and send result back to client
            unsigned char *tmp = int_serialize(buffer, ret.return_size);
            if (ret.return_size > 0)
                memcpy(tmp, ret.return_val, ret.return_size);

            sendto(sockfd, buffer, ret.return_size + sizeof(int), 0, (struct sockaddr *)&remote_addr, addr_len);

            if (ret.need_free)
                rpc_free(&ret);
        }
    }
}
