#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define N 100000000

int check(const int*a, const int*b) {
    for (int i = 0; i < 256; i++) {
        if (a[i] != b[i]) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    unsigned char *A = malloc(N);
    if(!A){
	printf("malloc error");
        return 1;
    }

    srand(0);
    for (int i = 0; i < N; i++) {
        A[i] = (unsigned char) (rand() % 256);
    }

    int hist_naive[256] = {0};
    int hist_critical[256] = {0};
    int hist_reduction[256] = {0};

    #pragma omp parallel num_threads(4)
    {
        #pragma omp for
        for (int i = 0; i < N; i++) {
            hist_naive[A[i]]++;
        }

        #pragma omp for
        for (int i = 0; i < N; i++) {
            #pragma omp critical
            hist_critical[A[i]]++;
        }

        #pragma omp for reduction(+ : hist_reduction[:256])
        for (int i = 0; i < N; i++) {
            hist_reduction[A[i]]++;
        }
    }

    int a = check(hist_critical, hist_reduction);
    printf("crit vs red: %d\n", a);

    int b = check(hist_critical, hist_naive);
    printf("crit vs naive: %d\n", b);

    free(A);
    return 0;
}
