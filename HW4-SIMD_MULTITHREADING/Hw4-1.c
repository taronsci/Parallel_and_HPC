#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <immintrin.h>
#include <stdint.h>

#define MB (1024 * 1024)
const size_t size = 512 * MB;

unsigned char *dnaSequence;
#define thread_count 2

// general utils
void printCounts();
void resetGlobalCounts();
static double now_sec(void);

// multithreading
uint64_t A = 0;
uint64_t C = 0;
uint64_t G = 0;
uint64_t T = 0;

pthread_spinlock_t spinlock[4]; // using spinlock will be faster
void *counter(void *arg);
void pthreadCounter();

// SIMD
typedef struct {
    int index;
    uint64_t A, C, G, T;
} DNA_Counts;

DNA_Counts simdCounter(size_t start, size_t end);

//SIMD + multithreading
void *threadSimd(void *arg);
void threadSimdCounter();

//sequential
void countNucleotides();
int verify(DNA_Counts *sequential, DNA_Counts *other);

int main(void) {
    unsigned char nucleotide[] = {'A', 'C', 'G', 'T'};

    dnaSequence = malloc(size);
    if (dnaSequence == NULL) {
        perror("malloc");
        return 1;
    }

    srand(1); //time(NULL)
    for (int i = 0; i < size; i++) {
        dnaSequence[i] = nucleotide[rand() % 4];
    }

    //sequential
    double t0 = now_sec();
    countNucleotides();
    double t00 = now_sec();
    printCounts();
    DNA_Counts seq = {0, A, C, G, T};
    resetGlobalCounts();

    //threads
    double t1 = now_sec();
    pthreadCounter();
    double t2 = now_sec();
    printCounts();
    DNA_Counts thr = {0, A, C, G, T};
    if (verify(&seq, &thr)  != 0) {
        printf("error in threads version");
    }
    resetGlobalCounts();

    //simd
    double t3 = now_sec();
    DNA_Counts c = simdCounter(0, size);
    if (c.index == -1) {
        fprintf(stderr, "error in sequence!\n");
    }
    A = c.A;
    C = c.C;
    G = c.G;
    T = c.T;
    double t4 = now_sec();
    printCounts();
    DNA_Counts simd = {0, A, C, G, T};
    if (verify(&seq, &simd)  != 0) {
        printf("error in SIMD version");
    }
    resetGlobalCounts();

    //threads + simd
    double t5 = now_sec();
    threadSimdCounter();
    double t6 = now_sec();
    printCounts();
    DNA_Counts thrsmd = {0, A, C, G, T};
    if (verify(&seq, &thrsmd)  != 0) {
        printf("error in SIMD+threads version");
    }
    resetGlobalCounts();

    printf("\n");
    printf("DNA size: %lu MB\n", size / MB);
    printf("Threads used: %d\n", thread_count);
    printf("\n");
    printf("Sequential time:            %f sec\n", t00 - t0);
    printf("Multithreading time:        %f sec\n", t2 - t1);
    printf("SIMD time:                  %f sec\n", t4 - t3);
    printf("SIMD + Multithreading time: %f sec\n", t6 - t5);

    free(dnaSequence);
    return 0;
}

// general utils
void resetGlobalCounts() {
    A = 0;
    C = 0;
    G = 0;
    T = 0;
}

void printCounts() {
    printf("A : %lu ", A);
    printf("C : %lu ", C);
    printf("G : %lu ", G);
    printf("T : %lu ", T);
    printf("\n");
}

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9;
}

int verify(DNA_Counts *sequential, DNA_Counts *other) {
    if (sequential->A != other->A ||
        sequential->C != other->C ||
        sequential->G != other->G ||
        sequential->T != other->T)
        return 1;

    return 0;
}

// multithreading
void *counter(void *arg) {
    int *num = (int *) arg;

    const size_t chunk = size / thread_count;
    const size_t start = *num * chunk;
    const size_t end = *num == thread_count - 1 ? size : start + chunk;

    for (size_t i = start; i < end; i++) {
        switch (dnaSequence[i]) {
            case 'A':
                pthread_spin_lock(&spinlock[0]);
                A++;
                pthread_spin_unlock(&spinlock[0]);
                break;
            case 'C':
                pthread_spin_lock(&spinlock[1]);
                C++;
                pthread_spin_unlock(&spinlock[1]);
                break;
            case 'G':
                pthread_spin_lock(&spinlock[2]);
                G++;
                pthread_spin_unlock(&spinlock[2]);
                break;
            case 'T':
                pthread_spin_lock(&spinlock[3]);
                T++;
                pthread_spin_unlock(&spinlock[3]);
                break;
            default:
                printf("error!\n");
                return NULL;
        }
    }

    return NULL;
}

void pthreadCounter() {
    for (int i = 0; i < 4; i++) {
        if (pthread_spin_init(&spinlock[i], PTHREAD_PROCESS_PRIVATE) != 0) {
	    perror("pthread_spin_inti");
	}
    }

    pthread_t threads[thread_count];
    int arguments[thread_count];
    for (int i = 0; i < thread_count; i++) {
        arguments[i] = i;
        pthread_create(&threads[i], NULL, counter, &arguments[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < 4; i++) {
        pthread_spin_destroy(&spinlock[i]);
    }
}

// SIMD
DNA_Counts simdCounter(size_t start, size_t end) {
    DNA_Counts counts = {0, 0, 0, 0, 0};

    const __m256i targetA = _mm256_set1_epi8('A');
    const __m256i targetC = _mm256_set1_epi8('C');
    const __m256i targetG = _mm256_set1_epi8('G');
    const __m256i targetT = _mm256_set1_epi8('T');

    size_t i = start;
    size_t limit = end & ~(size_t) 31; // round down to multiple of 32

    for (; i < limit; i += 32) {
        __m256i vector = _mm256_loadu_si256((const __m256i *) (dnaSequence + i));
        __m256i eqA = _mm256_cmpeq_epi8(vector, targetA);
        __m256i eqC = _mm256_cmpeq_epi8(vector, targetC);
        __m256i eqG = _mm256_cmpeq_epi8(vector, targetG);
        __m256i eqT = _mm256_cmpeq_epi8(vector, targetT);

        unsigned maskA = (unsigned) _mm256_movemask_epi8(eqA);
        unsigned maskC = (unsigned) _mm256_movemask_epi8(eqC);
        unsigned maskG = (unsigned) _mm256_movemask_epi8(eqG);
        unsigned maskT = (unsigned) _mm256_movemask_epi8(eqT);

        counts.A += (uint64_t) __builtin_popcount(maskA);
        counts.C += (uint64_t) __builtin_popcount(maskC);
        counts.G += (uint64_t) __builtin_popcount(maskG);
        counts.T += (uint64_t) __builtin_popcount(maskT);
    }

    for (; i < end; i++) {
        switch (dnaSequence[i]) {
            case 'A': counts.A++; break;
            case 'C': counts.C++; break;
            case 'G': counts.G++; break;
            case 'T': counts.T++; break;
            default:
                printf("error!\n");
                counts.index = -1;
                return counts;
        }
    }

    return counts;
}

//SIMD + multithreading
void *threadSimd(void *arg) {
    DNA_Counts *counts = (DNA_Counts *) arg;
    size_t num = counts->index;

    const size_t chunk = size / thread_count;
    const size_t start = num * chunk;
    const size_t end = num == thread_count - 1 ? size : start + chunk;

    *counts = simdCounter(start, end);
    if (counts->index == -1) {
        fprintf(stderr, "error!\n");
    }

    return NULL;
}

void threadSimdCounter() {
    pthread_t threads[thread_count];
    DNA_Counts arguments[thread_count];
    for (int i = 0; i < thread_count; i++) {
        arguments[i].index = i;
        pthread_create(&threads[i], NULL, threadSimd, &arguments[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    for (int i = 0; i < thread_count; i++) {
        A += arguments[i].A;
        C += arguments[i].C;
        G += arguments[i].G;
        T += arguments[i].T;
    }
}

//sequential
void countNucleotides() {
    DNA_Counts counts = {0, 0, 0, 0, 0};
    for (size_t i = 0; i < size; i++) {
        switch (dnaSequence[i]) {
            case 'A': counts.A++; break;
            case 'C': counts.C++; break;
            case 'G': counts.G++; break;
            case 'T': counts.T++; break;
            default:
                printf("error!\n");
                return;
        }
    }
    A = counts.A;
    C = counts.C;
    G = counts.G;
    T = counts.T;
}
