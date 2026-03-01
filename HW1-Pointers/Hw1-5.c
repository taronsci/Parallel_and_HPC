#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *a = malloc(sizeof(int));
    *a = 5;
    printf("%d \n", *a);

    int *arr = malloc(*a * sizeof(int));
    for (int i = 0; i < 5; i++) {
        *(arr+i) = i + 1;
        printf("%d ", *(arr+i));
    }
    printf("\n");

    free(arr);
    free(a);
    return 0;
}
