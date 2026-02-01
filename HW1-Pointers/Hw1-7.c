#include <stdio.h>

int main(void) {
    char *arr[] = {"string1", "string2", "string3", "string4"};

    for (int i = 0; i < 4; i++) {
        printf("%s ", *(arr + i));
    }
    printf("\n");

    arr[2] = "imposter";
    for (int i = 0; i < 4; i++) {
        printf("%s ", *(arr + i));
    }
    printf("\n");

    return 0;
}
