#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#define SIZE 50000000
#define N 10

int *arr = NULL;

void* threadSum(void* args) {
    int tNum = *(int*) args;
    long long sum = 0;

    int offset = tNum * SIZE/N;
    int end = offset + SIZE/N;

    for (int i = offset; i < end; i++) {
        sum += arr[i];
    }

    return (void*)(intptr_t)sum;
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

    // count sum sequentially
    long long sequentialSum = 0;
    for (int i = 0; i < SIZE; i++) {
        sequentialSum += arr[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Total Sum: %lld\n", 0, elapsed_time, sequentialSum);

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // count sum using threads
    pthread_t threads[N];
    int threadArgs[N];
    for (int i = 0; i < N; i++) {
        threadArgs[i] = i;
        pthread_create(&threads[i], NULL, threadSum, &threadArgs[i]);
    }

    long long partialSums[N];
    for (int i = 0; i < N; i++) {
        void *retval_ptr;
        pthread_join(threads[i], &retval_ptr);
        partialSums[i] = (long long)(intptr_t) retval_ptr;
    }

    long long threadsSum = 0;
    for (int i = 0; i < N; i++) {
        threadsSum += partialSums[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Total Sum: %lld\n", N, elapsed_time, sequentialSum);

    free(arr);
    return 0;
}
