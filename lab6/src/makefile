CC=gcc
CFLAGS=-I.

all: server client

server: server.c common.c common.h
	$(CC) -o server server.c common.c -lpthread $(CFLAGS)

client: client.c common.c common.h
	$(CC) -o client client.c common.c -lpthread $(CFLAGS)

clean:
	rm -f server client *.o