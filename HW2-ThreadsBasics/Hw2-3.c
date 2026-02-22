#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define SIZE 50000000
#define N 4

int *arr = NULL;

struct Args {
    int tNum;
    int localMax;
};

void* findMax(void* args) {
    struct Args *arg = args;

    int offset = arg->tNum * SIZE/N;
    int end = offset + SIZE/N;

    int max = arr[offset];
    for (int i = offset + 1; i < end; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    arg->localMax = max;

    return NULL;
}

int main(void) {
    // initialize array
    arr = malloc(SIZE * sizeof(int));
    if (arr == NULL) {
        perror("Malloc failed");
        return 1;
    }

    srandom(time(NULL));
    for (int i = 0; i < SIZE; i++) {
        arr[i] = (int) random() - RAND_MAX / 2;
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // find max sequentially
    int sequentialMax = arr[0];
    for (int i = 1; i < SIZE; i++) {
        if (arr[i] > sequentialMax)
            sequentialMax = arr[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Max: %d\n", 0, elapsed_time, sequentialMax);

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // find max using threads
    pthread_t threads[N];
    struct Args threadArgs[N];
    for (int i = 0; i < N; i++) {
        threadArgs[i].tNum = i;
        pthread_create(&threads[i], NULL, findMax, &threadArgs[i]);
    }

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    int threadsMax = threadArgs[0].localMax;
    for (int i = 1; i < N; i++) {
        if (threadArgs[i].localMax > threadsMax)
            threadsMax = threadArgs[i].localMax;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Max: %d\n", N, elapsed_time, threadsMax);

    free(arr);
    return 0;
}