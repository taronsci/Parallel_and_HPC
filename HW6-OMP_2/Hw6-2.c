#include <float.h>
#include <math.h>
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define N 50000000

int main(void) {
    double *A = malloc(sizeof(double) * N);
    if(!A){
	printf("malloc error");
        return 1;
    }

    srand(0);
    for (int i = 0; i < N; i++) {
        A[i] = ((double) rand() / RAND_MAX) * 100000;
    }

    double min_diff = DBL_MAX;

    #pragma omp parallel for reduction(min : min_diff)
    for (int i = 1; i < N; i++) {
        double diff = fabs(A[i] - A[i - 1]);
        if (diff < min_diff)
            min_diff = diff;
    }

    printf("min absolute difference is %f\n", min_diff);

    free(A);
    return 0;
}
