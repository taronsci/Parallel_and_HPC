#include <stdio.h>

int main(void) {
    int arr[] = {1, 2, 3, 4, 5};
    int *p2 = arr;

    for (int i = 0; i < 5; i++) {
        printf("%d ", *(p2 + i));
        *(p2 + i) += 2;
    }
    printf("\n");

    for (int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
        printf("%d ", *(p2 + i));
    }
    printf("\n");

    return 0;
}
