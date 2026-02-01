#include <stdio.h>

int main(void) {
    int a = 5;
    int *p1 = &a;
    printf("%p \n", p1);
    printf("%p \n", &a);
    *p1 = 10;
    printf("%d \n", a);

    return 0;
}
