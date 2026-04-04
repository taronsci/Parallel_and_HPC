#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define N 1000000

int main(void) {
    long *A = malloc(sizeof(long)* N);
    if (!A) {
	printf("malloc error");
        return 1;
    }

    srand(0);
    for (int i = 0; i < N; i++) {
        A[i] = rand() % 10000;
    }

    long max_val = A[0];

    #pragma omp parallel for reduction(max : max_val)
    for (int i = 1; i < N; i++) {
        max_val = max_val < A[i] ? A[i] : max_val;
    }

    double threshold = 0.8 * (double) max_val;

    printf("Max val is %ld\n", max_val);

    long sum = 0;
    #pragma omp parallel for reduction(+ : sum)
    for (int i = 0; i < N; i++) {
        if (threshold < A[i]) {
            sum += A[i];
        }
    }

    printf("Sum of elements > %0.2f is %ld\n", threshold, sum);


    free(A);
    return 0;
}
