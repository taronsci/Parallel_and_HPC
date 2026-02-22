#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SIZE 20000000
#define N 10

int *arr = NULL;
uint8_t *sieve = NULL;

void set_bit(uint8_t *array, uint32_t i) {
    array[i / 8] |= 1 << (i % 8);
}

int get_bit(uint8_t *array, uint32_t i) {
    return array[i / 8] >> (i % 8) & 1;
}

void clear_bit(uint8_t *array, uint32_t i) {
    array[i / 8] &= ~(1 << (i % 8));
}

int countPrimesSequentially() {
    int count = 0;

    for (uint32_t i = 2; i * i <= SIZE; i++) {
        if (get_bit(sieve, i)) {
            for (uint32_t j = i * i; j <= SIZE; j += i) {
                clear_bit(sieve, j);
            }
        }
    }

    for (uint32_t i = 2; i <= SIZE; i++) {
        if (get_bit(sieve, i)) {
            count++;
        }
    }

    return count;
}

int isPrime(uint32_t n) {
    if (n == 0 || n == 1)
        return 0;

    for (int i = 2; i*i <= n; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

void *countPrimes(void *args) {
    int tNum = *(int*) args;
    int count = 0;

    int offset = tNum * SIZE/N;
    int end = offset + SIZE/N;

    for (uint32_t i = offset; i < end; i++) {
        if (isPrime(i)) {
            count++;
        }
    }

    return (void*)(intptr_t)count;
}

int main(void) {
    // count primes sequentially
    sieve = malloc((SIZE + 7) / 8);
    memset(sieve, 0xFF, (SIZE + 7) / 8);

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int c = countPrimesSequentially();
    free(sieve);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Number of primes: %d\n", 0, elapsed_time, c);


    clock_gettime(CLOCK_MONOTONIC, &start_time);
    // count primes using threads
    pthread_t threads[N];
    int threadArgs[N];
    for (int i = 0; i < N; i++) {
        threadArgs[i] = i;
        pthread_create(&threads[i], NULL, countPrimes, &threadArgs[i]);
    }

    int count[N];
    for (int i = 0; i < N; i++) {
        void *retval_ptr;
        pthread_join(threads[i], &retval_ptr);
        count[i] = (int)(intptr_t) retval_ptr;
    }
    int totalCount = 0;
    for (int i = 0; i < N; i++) {
        totalCount += count[i];
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Threads: %d | Time: %.6f sec | Number of primes: %d\n", N, elapsed_time, totalCount);

    return 0;
}

