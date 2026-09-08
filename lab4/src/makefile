CC=gcc
CFLAGS=-I.

all: parallel_min_max process_memory parallel_sum

parallel_min_max: parallel_min_max.c find_min_max.c utils.c find_min_max.h utils.h
	$(CC) -o parallel_min_max parallel_min_max.c find_min_max.c utils.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

libsum.a: sum_lib.o
	ar rcs libsum.a sum_lib.o

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) -c sum_lib.c -o sum_lib.o $(CFLAGS)

parallel_sum: parallel_sum.c utils.c libsum.a
	$(CC) parallel_sum.c utils.c -L. -lsum -lpthread -o parallel_sum $(CFLAGS)

clean:
	rm -f parallel_min_max process_memory parallel_sum zombie *.o *.a