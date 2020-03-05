target: server client

server: myserver.c rpc_server_stub.c rpc_pal.c coroutine.c
	gcc -g -o $@ $^ -lpthread

client: myclient.c rpc_client_stub.c rpc_pal.c coroutine.c
	gcc -g -o $@ $^ -lpthread

clean:
	rm -f server client
