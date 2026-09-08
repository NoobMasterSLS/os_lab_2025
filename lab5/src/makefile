CC=gcc
CFLAGS=-I.

all: mutex factorial deadlock

mutex: mutex.c
	$(CC) -o mutex mutex.c -lpthread

factorial: factorial.c
	$(CC) -o factorial factorial.c -lpthread

deadlock: deadlock.c
	$(CC) -o deadlock deadlock.c -lpthread

clean:
	rm -f mutex factorial deadlock