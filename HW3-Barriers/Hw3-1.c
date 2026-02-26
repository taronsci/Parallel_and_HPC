#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define THREAD_NUM 8
#define R 5

int game = 0;
int dice_values[THREAD_NUM];
int status[THREAD_NUM] = { 0 };
int wins[THREAD_NUM] = { 0 };

pthread_barrier_t barrierRolledDice;
pthread_barrier_t barrierCalculated;

void* roll(void* args) {
    int index = *(int*)args;
    free(args);

    while (game < R) {
        dice_values[index] = rand() % 6 + 1;
        pthread_barrier_wait(&barrierRolledDice);
	pthread_barrier_wait(&barrierCalculated);

        if (status[index] == 1){
	    wins[index] += 1;
            printf("(%d rolled %d) I won\n", index, dice_values[index]);
        } else {
            printf("(%d rolled %d) I lost\n", index, dice_values[index]);
        }
  }
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    pthread_t th[THREAD_NUM];
    int i;
    pthread_barrier_init(&barrierRolledDice, NULL, THREAD_NUM + 1);
    pthread_barrier_init(&barrierCalculated, NULL, THREAD_NUM + 1);
    for (i = 0; i < THREAD_NUM; i++) {
        int* a = malloc(sizeof(int));
        *a = i;
        if (pthread_create(&th[i], NULL, &roll, (void*) a) != 0) {
            perror("Failed to create thread");
        }
    }
    
    while (game < R) {
        pthread_barrier_wait(&barrierRolledDice);
        // Calculate winner

        game++;
	int max = 0;
        for (i = 0; i < THREAD_NUM; i++) {
            if (dice_values[i] > max) {
                max = dice_values[i];
            }
        }

        for (i = 0; i < THREAD_NUM; i++) {
            if (dice_values[i] == max) {
                status[i] = 1;
            } else {
                status[i] = 0;
            }
        }
        printf("==== New round starting ====\n");
        pthread_barrier_wait(&barrierCalculated);
    }

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    int maxWins = 0;
    for (i = 1; i < THREAD_NUM; i++) {
        if (wins[i] > wins[maxWins]) {
            maxWins = i;
        }
    }
    printf("\nThe results are\n");
    for (i = 0; i < THREAD_NUM; i++) {
        printf("Thread %d - %d wins\n", i, wins[i]);
    }
    printf("Winner of the games is Thread %d with %d wins\n", maxWins, wins[maxWins]);

    pthread_barrier_destroy(&barrierRolledDice);
    pthread_barrier_destroy(&barrierCalculated);
    return 0;
}
