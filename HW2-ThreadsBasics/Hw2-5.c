#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sched.h>

#define SIZE 500000
#define N 10

void* loop(void* args) {
    int tNum = *(int*) args;
    
    for (int i = 0; i < SIZE; i++) {
        printf("Thread %d, CPU core %d\n", tNum, sched_getcpu());
    }
    return NULL;
}

int main(void) {
    pthread_t threads[N];
    int threadArgs[N];
    for (int i = 0; i < N; i++) {
        threadArgs[i] = i;
        pthread_create(&threads[i], NULL, loop, &threadArgs[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }
}
