#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define M 4

pthread_barrier_t pipelineBarrier;

void* work(void* arg) {
    int num = *(int*)arg;

    printf("Thread %d in stage 1\n", num);
    pthread_barrier_wait(&pipelineBarrier);

    printf("Thread %d in stage 2\n", num);
    pthread_barrier_wait(&pipelineBarrier);

    printf("Thread %d in stage 3\n", num);
    pthread_barrier_wait(&pipelineBarrier);

    return NULL;
}
int main(void) {
    srand(time(NULL));
    pthread_barrier_init(&pipelineBarrier, NULL, M + 1);

    pthread_t th[M];
    int thread[M];
    for (int i = 0; i < M; i++) {
        thread[i] = i;
        if (pthread_create(&th[i], NULL, work, &thread[i]) != 0) {
            perror("Failed to create thread");
        }
    }
    
    pthread_barrier_wait(&pipelineBarrier);
    
    pthread_barrier_wait(&pipelineBarrier);

    pthread_barrier_wait(&pipelineBarrier);

    printf("All Stages completed\n");
    
    for (int i = 0; i < M; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    pthread_barrier_destroy(&pipelineBarrier);

    return 0;
}
