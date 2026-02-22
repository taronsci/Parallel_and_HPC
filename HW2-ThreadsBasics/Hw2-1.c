#include <stdio.h>
#include <pthread.h>

void* print(void *args) {
    printf("Thread %ld is running\n", pthread_self());
    return NULL;
}

int main(void) {
    pthread_t thread1, thread2, thread3;

    pthread_create(&thread1, NULL, print, NULL);
    pthread_create(&thread2, NULL, print, NULL);
    pthread_create(&thread3, NULL, print, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return 0;
}
