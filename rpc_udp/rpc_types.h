#ifndef _RPC_TYPES_H
#define _RPC_TYPES_H

#include <stdbool.h>
#include <arpa/inet.h>

#define RPC_SERVER_PORT 10000
#define RPC_CLIENT_PORT 10069
#define RPC_RCVTIMEO_S	3
#define RPC_BUFFER_SIZE	512

/* returnType */
typedef struct
{
	void *return_val;
	int return_size;
	bool need_free;
} return_type;

/* arg list */
typedef struct arg
{
	void *arg_val;
	int arg_size;
	struct arg *next;
} arg_type;

/* Type for the function pointer with which server code registers with the
 * server stub. The first const int argument is the # of arguments, i.e.,
 * array entries in the second argument. */
typedef return_type (*fp_type)(const int, arg_type *);

/******************************************************************/
/* extern declarations -- you need to implement these 4 functions */
/******************************************************************/

/* The following need to be implemented in the server stub */

/* register_procedure() -- invoked by the app programmer's server code
 * to register a procedure with this server_stub. Note that more than
 * one procedure can be registered */
extern bool register_procedure(const char *procedure_name,
							   const int nparams,
							   fp_type fnpointer);

/* launch_server() -- used by the app programmer's server code to indicate that
 * it wants start receiving rpc invocations for functions that it registered
 * with the server stub.
 *
 * IMPORTANT: the first thing that should happen when launch_server() is invoked
 * is that it should print out, to stdout, the IP address/domain name and
 * port number on which it listens.
 *
 * launch_server() simply runs forever. That is, it waits for a client request.
 * When it receives a request, it services it by invoking the appropriate 
 * application procedure. It returns the result to the client, and goes back
 * to listening for more requests.
 */
extern void launch_server();

/* The following needs to be implemented in the client stub. This is a
 * procedure with a variable number of arguments that the app programmer's
 * client code uses to invoke. The arguments should be self-explanatory.
 *
 * For each of the nparams parameters, we have two arguments: size of the
 * argument, and a (void *) to the argument. */
extern return_type make_remote_call(const char *servernameorip,
									const int serverportnumber,
									const char *procedure_name,
									const int nparams,
									...);

// create rpc client socket
extern int rpc_client_init();

// close rpc client socket
extern void rpc_client_close();

// free return_type after make_remote_call
extern void rpc_free(return_type *rt);

extern int create_udp_server(int port);

extern int fill_sockaddr_in(struct sockaddr_in *addr, const char *nameorip, int port);

extern unsigned char *int_serialize(unsigned char *buffer, int value);

extern void print_array(const char *sender, const char *buf, int size);

#endif
