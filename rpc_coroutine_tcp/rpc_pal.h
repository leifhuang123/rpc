#ifndef _RPC_PAL_H
#define _RPC_PAL_H

#include <stdbool.h>
#include <assert.h>

#define RPC_PROC_DB_SIZE 100
#define RPC_SERVER_PORT 10000
#define RPC_BUFFER_SIZE 512
// used for debug
#define DEBUG 1
#define ADD_TIMESTAMP 1

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
/* extern declarations 											  */
/******************************************************************/

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
extern void launch_server(int coroutine_number, int listen_number);

/* The following needs to be implemented in the client stub. This is a
 * procedure with a variable number of arguments that the app programmer's
 * client code uses to invoke. The arguments should be self-explanatory.
 *
 * For each of the nparams parameters, we have two arguments: size of the
 * argument, and a (void *) to the argument. */
extern return_type make_remote_call(int sockfd,
									const char *procedure_name,
									const int nparams,
									...);

// socket related below
extern int rpc_create_server(int server_port, int listen_number);

extern int rpc_create_client(const char *server_nameorip, int server_port, int recv_timeout_s);

extern void rpc_close(int sockfd);

// malloc/free return_type
extern void rpc_malloc(return_type *rt, size_t size);

extern void rpc_free(return_type *rt);

extern unsigned char *int_serialize(unsigned char *buffer, int value);

/******************************************************************/
/* used for debug 											  	  */
/******************************************************************/

extern void print_array(const char *sender, const char *buf, int size);

extern void print(const char *sender, const char *fmt, ...);

#ifdef DEBUG
#define LOG(fmt, ...) print(__func__, fmt, ##__VA_ARGS__)
#define LOGA(sender, buf, size) print_array(sender, buf, size)
#else
#define LOG(fmt, ...)
#define LOGA(sender, buf, size)
#endif

#endif
