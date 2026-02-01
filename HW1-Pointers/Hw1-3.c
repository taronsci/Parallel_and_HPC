#include <stdio.h>

void swap(int *, int *);

int main(void) {
    int arr[] = {3, 4};

    printf("%d ", *arr); //3
    printf("%d \n", *(arr + 1)); //4

    swap(arr, arr + 1);

    printf("%d ", *arr); //4
    printf("%d \n", *(arr + 1)); //3
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}
