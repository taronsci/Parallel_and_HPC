#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define S 4

pthread_barrier_t barrierWeatherStation;

void* collectData(void* arg) {
    int num = *(int*) arg;

    printf("Thread %d is collecting data\n", num);
    sleep(rand() % 3);
    int data = rand() % (40 + 15 + 1) - 15;

    printf("thread %d has collected data %d\n", num, data);
    pthread_barrier_wait(&barrierWeatherStation);

    return (void*)(intptr_t)data;
}

int main(void) {
    srand(time(NULL));
    pthread_barrier_init(&barrierWeatherStation, NULL, S + 1);

    pthread_t th[S];
    int thread[S];
    for (int i = 0; i < S; i++) {
        thread[i] = i;
        if (pthread_create(&th[i], NULL, collectData, &thread[i]) != 0) {
            perror("Failed to create thread");
        }
    }

    pthread_barrier_wait(&barrierWeatherStation);
    
    int tempData[S];
    for (int i = 0; i < S; i++) {
        void *retval_ptr;
        if (pthread_join(th[i], &retval_ptr) != 0) {
            perror("Failed to join thread");
        }
        tempData[i] = (int)(intptr_t) retval_ptr;
    }

    double tempAverage = 0;
    for (int i = 0; i < S; i++) {
        tempAverage += tempData[i];
    }
    tempAverage /= S;

    printf("Temp Average = %.2f\n", tempAverage);

    pthread_barrier_destroy(&barrierWeatherStation);

    return 0;
}
