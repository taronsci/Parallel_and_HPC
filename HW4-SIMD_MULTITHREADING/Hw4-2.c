#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <immintrin.h>
#include <ctype.h>

#define MB (1024 * 1024)
const size_t size = 512 * MB;

unsigned char *buffer1;
unsigned char *buffer2;
unsigned char *buffer3;

#define thread_count 4


//utils
static double now_sec(void);

//multithreaded
void *to_upper_case(void *arg);
void pthreadUpper();

//simd
void simd_converter(size_t start, size_t end, unsigned char *buffer);

//SIMD + multithreading
void *threadSimd(void *arg);
void threadSimdConverter();


int main(void) {
    const char charset[] =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

    buffer1 = malloc(size);
    if (buffer1 == NULL) {
        perror("malloc 1");
        return 1;
    }
    buffer2 = malloc(size);
    if (buffer2 == NULL) {
        perror("malloc 2");
        return 1;
    }
    buffer3 = malloc(size);
    if (buffer3 == NULL) {
        perror("malloc 3");
        return 1;
    }

    srand(time(NULL));
    int charset_size = sizeof(charset) - 1;
    for (int i = 0; i < size; i++) {
        buffer3[i] = buffer2[i] = buffer1[i] = charset[rand() % charset_size];
    }

    //threads
    double t1 = now_sec();
    pthreadUpper();
    double t2 = now_sec();

    //simd
    double t3 = now_sec();
    simd_converter(0, size, buffer2);
    double t4 = now_sec();

    //threads + simd
    double t5 = now_sec();
    threadSimdConverter();
    double t6 = now_sec();


    printf("Buffer size: %lu MB\n", size / MB);
    printf("Threads used: %d\n", thread_count);
    printf("\n");
    printf("Multithreading time:        %f sec\n", t2 - t1);
    printf("SIMD time:                  %f sec\n", t4 - t3);
    printf("SIMD + Multithreading time: %f sec\n", t6 - t5);


    free(buffer1);
    free(buffer2);
    free(buffer3);
    return 0;
}


// utils
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9;
}

// multithreading
void *to_upper_case(void *arg) {
    int num = *(int *) arg;

    const size_t chunk = size / thread_count;
    const size_t start = num * chunk;
    const size_t end = num == thread_count - 1 ? size : start + chunk;

    for (size_t i = start; i < end; i++) {
        if (isalpha(buffer1[i]) && islower(buffer1[i])) {
            buffer1[i] = toupper(buffer1[i]);
        }
    }

    return NULL;
}

void pthreadUpper() {
    pthread_t threads[thread_count];
    int arguments[thread_count];

    for (int i = 0; i < thread_count; i++) {
        arguments[i] = i;
        pthread_create(&threads[i], NULL, to_upper_case, &arguments[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}


//simd
void simd_converter(size_t start, size_t end, unsigned char *buffer) {
    const __m256i t1 = _mm256_set1_epi8('a' - 1); // lower bound for lowercase letters
    const __m256i t2 = _mm256_set1_epi8('z' + 1); // upper bound for lowercase letters

    size_t i = start;
    size_t limit = end & ~(size_t) 31; // round down to multiple of 32

    for (; i < limit; i += 32) {
        __m256i vector = _mm256_loadu_si256((__m256i *) (buffer + i));

        __m256i gea = _mm256_cmpgt_epi8(vector, t1); // 1 if vector[i] >= 'a'
        __m256i lez = _mm256_cmpgt_epi8(t2, vector); // 1 if vector[i] <= 'z'
        __m256i mask = _mm256_and_si256(gea, lez);  // and: checks if vector[i] is lowercase letter

        __m256i bit6 = _mm256_set1_epi8(0x20);      // 0010 0000 - difference between lower- and upper-case ascii letter is the 6th bit.
        __m256i flip = _mm256_and_si256(mask, bit6); // For lowercase letters keep 0x20
        vector = _mm256_xor_si256(vector, flip);    //xor-ing flips that bit and we get uppercase

        _mm256_storeu_si256((__m256i *) (buffer + i), vector);
    }

    for (; i < end; i++) {
        if (isalpha(buffer[i]) && islower(buffer[i])) {
            buffer[i] = toupper(buffer[i]);
        }
    }
}

//SIMD + multithreading
void *threadSimd(void *arg) {
    int num = *(int *) arg;

    const size_t chunk = size / thread_count;
    const size_t start = num * chunk;
    const size_t end = num == thread_count - 1 ? size : start + chunk;

    simd_converter(start, end, buffer3);

    return NULL;
}

void threadSimdConverter() {
    pthread_t threads[thread_count];
    int arguments[thread_count];
    for (int i = 0; i < thread_count; i++) {
        arguments[i] = i;
        pthread_create(&threads[i], NULL, threadSimd, &arguments[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
}
