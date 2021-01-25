build: client server

client: client.c
	mpicc client.c -o client

server: server.c
	gcc server.c -o server

.PHONY: clean
clean:
	rm -rf client server
