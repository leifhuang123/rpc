target: server client

server: myserver.c rpc_server_stub.c rpc_pal.c
	gcc -o $@ $^

client: myclient.c rpc_client_stub.c rpc_pal.c
	gcc -o $@ $^

clean:
	rm -rf server client
