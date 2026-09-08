#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "utils.h"
#include "sum_lib.h"

typedef struct {
    int *array;
    int start;
    int end;
    long long result;
} ThreadArg;

void* thread_func(void *arg) {
    ThreadArg *targ = (ThreadArg*)arg;
    targ->result = sum_range(targ->array, targ->start, targ->end);
    return NULL;
}

int main(int argc, char **argv) {
    int threads_num = -1;
    int seed = -1;
    int array_size = -1;


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--threads_num") == 0 && i+1 < argc) {
            threads_num = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i+1 < argc) {
            seed = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--array_size") == 0 && i+1 < argc) {
            array_size = atoi(argv[++i]);
        }
    }

    if (threads_num <= 0 || seed == -1 || array_size <= 0) {
        printf("Usage: %s --threads_num N --seed S --array_size M\n", argv[0]);
        return 1;
    }


    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);


    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    ThreadArg *args = malloc(sizeof(ThreadArg) * threads_num);

    int chunk = array_size / threads_num;
    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].start = i * chunk;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk;
        args[i].result = 0;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    long long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        pthread_join(threads[i], NULL);
        total_sum += args[i].result;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Total sum: %lld\n", total_sum);
    printf("Elapsed time: %f seconds\n", elapsed);

    free(array);
    free(threads);
    free(args);
    return 0;
}