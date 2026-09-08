#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    printf("thread 1: пытаюсь захватить mutex1\n");
    pthread_mutex_lock(&mutex1);
    printf("thread 1: захватил mutex1, сплю 1 сек\n");
    sleep(1);
    printf("thread 1: пытаюсь захватить mutex2\n");
    pthread_mutex_lock(&mutex2);
    printf("thread 1: захватил mutex2\n");
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void* thread2_func(void* arg) {
    printf("thread 2: пытаюсь захватить mutex2\n");
    pthread_mutex_lock(&mutex2);
    printf("thread 2: захватил mutex2, сплю 1 сек\n");
    sleep(1);
    printf("thread 2: пытаюсь захватить mutex1\n");
    pthread_mutex_lock(&mutex1);
    printf("thread 2: захватил mutex1\n");
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("программа завершилась без deadlock\n");
    return 0;
}