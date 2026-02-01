#include <stdio.h>

size_t str_length(char *str);

int main(void) {
    char *str = "Hello";

    char *str1 = str;
    for (; *str1 != '\0'; str1++) {
        printf("%c", *str1);
    }
    printf("\n");

    size_t len = str_length(str);
    printf("Length of string is %ld\n", len);

    return 0;
}

size_t str_length(char *str) {
    char *str1 = str;
    while (*str1 != '\0') {
        str1++;
    }
    return str1 - str;
}

