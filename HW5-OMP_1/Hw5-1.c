#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define N 20000

typedef struct {
    int request_id;
    int user_id;
    double response_time_ms;
} Log ;

int main(void) {
    Log *logs = malloc(sizeof(Log) * N);
    if (!logs) {
        printf("malloc error");
        return 1;
    }

    int FAST = 0;
    int MEDIUM = 0;
    int SLOW = 0;

    srand(0);
    #pragma omp parallel num_threads(4)
    {
        int loc_fast = 0;
        int loc_mid = 0;
        int loc_slow = 0;

        #pragma omp single
        {
            for (int i = 0; i < N; i++) {
                logs[i].request_id = i;
                logs[i].user_id = i;
                logs[i].response_time_ms = (double) (rand() % 500);
            }
        }
        // no need for barrier here. omp single has implicit barrier

        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (logs[i].response_time_ms < 100){
                loc_fast++;
            } else if (logs[i].response_time_ms > 300){
                loc_slow++;
            } else {
                loc_mid++;
            }
        }
        // no need for barrier here either, omp for also has implicit barrier

        #pragma omp atomic
        FAST += loc_fast;

        #pragma omp atomic
        SLOW += loc_slow;

        #pragma omp atomic
        MEDIUM += loc_mid;

        // barrier here to make sure all threads have updated
        #pragma omp barrier

        #pragma omp single
        {
            printf("fast is %d\n", FAST);
            printf("medium is %d\n", MEDIUM);
            printf("slow is %d\n", SLOW);
        }
    }

    free(logs);
    return 0;
}
