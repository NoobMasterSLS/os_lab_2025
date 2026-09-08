#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>


long long result = 1;     
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int mod;

typedef struct {
    int start;
    int end;
} range_t;

void* calc_partial(void* arg) {
    range_t *r = (range_t*)arg;
    long long partial = 1;
    for (int i = r->start; i <= r->end; i++) {
        partial = (partial * i) % mod;
    }
   
    pthread_mutex_lock(&mutex);
    result = (result * partial) % mod;
    pthread_mutex_unlock(&mutex);
    free(r);
    return NULL;
}

int main(int argc, char** argv) {
    int k = -1;
    int pnum = -1;
    mod = -1;

  
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i+1 < argc) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Usage: %s -k <k> --pnum=<threads> --mod=<mod>\n", argv[0]);
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * pnum);
    int chunk = k / pnum;
    int remainder = k % pnum;
    int start = 1;

    for (int i = 0; i < pnum; i++) {
        int end = start + chunk - 1;
        if (i < remainder) end++;
        if (end > k) end = k;

        range_t *r = malloc(sizeof(range_t));
        r->start = start;
        r->end = end;
        pthread_create(&threads[i], NULL, calc_partial, r);

        start = end + 1;
        if (start > k) break; 
    }


    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%d! mod %d = %lld\n", k, mod, result);

    free(threads);
    return 0;
}