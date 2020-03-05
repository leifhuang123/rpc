#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rpc_pal.h"


/**
 * Makes a remote procedure call on the specific server.
 */
return_type make_remote_call(int sockfd,
                             const char *procedure_name,
                             const int nparams, ...)
{
    return_type rt;
    rt.return_val = NULL;
    rt.return_size = 0;
	rt.need_free = false;

    /**
     * Pack data into buffer
     *
     * Buffer serialization format:
     * [functionSize | functionName]
     * [argumentSize | argument]
     * [argumentSize | argument]
     */

    unsigned char buffer[RPC_BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    // Pack function name and size in buffer
    int function_name_size = strlen(procedure_name) + 1;
    unsigned char *serial_result = int_serialize(buffer, function_name_size);
    unsigned char *function_name = (unsigned char *)procedure_name;

    int i = 0;
    while (i < function_name_size)
    {
        serial_result[i] = function_name[i];
        i++;
    }

    serial_result = serial_result + function_name_size;
    serial_result = int_serialize(serial_result, nparams);

    // Pack argument size and argument in buffer
    va_list var_args;
    va_start(var_args, nparams);
    i = 0;

    while (i < nparams)
    {
        // Put argument size in buffer
        int arg_size = va_arg(var_args, int);
        serial_result = int_serialize(serial_result, arg_size);

        // Put argument in buffer
        void *arg = va_arg(var_args, void *);
        unsigned char *char_arg = (unsigned char *)arg;

        int j = 0;
        while (j < arg_size)
        {
            serial_result[j] = char_arg[j];
            j++;
        }

        serial_result = serial_result + arg_size;
        i++;
    }

    va_end(var_args);

    int send_length = serial_result - &buffer[0];
    // Make remote procedure call
    send(sockfd, buffer, send_length, 0);

    // Listen for the result of remote procedure call
    memset(buffer, 0, sizeof(buffer));
    int receive_length = recv(sockfd, buffer, RPC_BUFFER_SIZE, 0);

    if (receive_length > 0)
    {
        LOGA("recv", buffer, receive_length);
        rt.return_size = *(int *)buffer;
        if ((rt.return_size > 0) && (receive_length == rt.return_size + sizeof(int)))
        {
            rpc_malloc(&rt, rt.return_size);
            memcpy(rt.return_val, buffer + sizeof(int), rt.return_size);
        }
		else
            LOG("unexpected return_type, size=%d\n", rt.return_size);
    }
    
    return rt;
}
