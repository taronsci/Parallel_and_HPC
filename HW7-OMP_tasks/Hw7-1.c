#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

typedef unsigned long long ull;

ull fibonacci(const int a) {
    if (a <= 0) return 0;
    if (a == 1) return 1;

    if (a <= 10) {
        int fib0 = 0;
        int fib1 = 1;
        for (int i = 2; i <= a; i++) {
            int fib_next = fib0 + fib1;
            fib0 = fib1;
            fib1 = fib_next;
        }
        return fib1;
    }

    ull x, y;
    #pragma omp task shared(x)
    x = fibonacci(a - 1);

    #pragma omp task shared(y)
    y = fibonacci(a - 2);

    #pragma omp taskwait
    return x + y;
}

int main(void) {
    int num;

    printf("Input n: ");
    if (scanf("%d", &num) != 1) {
        printf("Invalid input, please enter an integer.\n");
        return 1;
    }
    if (num < 0) {
        printf("Please enter a non-negative integer.\n");
        return 1;
    }
    if (num > 93) {
        printf("F(%d) will cause overflow, please pick a smaller number\n", num);
        return 1;
    }

    ull result = 0;

    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            result = fibonacci(num);
        }
    }

    printf("The %d-th fibonacci number is: %llu\n", num, result);

    return 0;
}
