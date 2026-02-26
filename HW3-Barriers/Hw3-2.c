#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define N 4

pthread_barrier_t barrierWaitRoom;

void* getReady(void* arg) {
    int num = *(int*) arg;
    
    printf("Thread %d is getting ready\n", num);
    sleep(rand() % 3);
    
    printf("Thread %d is ready, waiting in barrier\n", num);
    pthread_barrier_wait(&barrierWaitRoom);
    printf("Game Started!\n");

    return NULL;
}

int main(void) {
    pthread_barrier_init(&barrierWaitRoom, NULL, N + 1);
    
    pthread_t th[N];
    int thread[N];
    for (int i = 0; i < N; i++) {
        thread[i] = i;
        if (pthread_create(&th[i], NULL, getReady, &thread[i]) != 0) {
            perror("Failed to create thread");
        }
    }

    pthread_barrier_wait(&barrierWaitRoom);
    
    for (int i = 0; i < N; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    pthread_barrier_destroy(&barrierWaitRoom);

    return 0;
}
