#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

#define N 10000
typedef enum {
    HIGH, MEDIUM
} priority;

typedef struct {
    int order_id;
    priority priority;
    double distance_km;
} Order ;

int main(void) {
    Order *order = malloc(sizeof(Order) * N);
    if (!order) {
        printf("malloc error");
        return 1;
    }

    srand(0);
    for (int i = 0; i < N; i++) {
        order[i].order_id = i;
        order[i].priority = 0;
        order[i].distance_km = (double) (rand() % 40);
    }

    int distance_threshold;
    int thread_high_count[4] = {0};

    #pragma omp parallel num_threads(4)
    {
        int loc_high = 0;
        int id = omp_get_thread_num();

        #pragma omp single
        {
            distance_threshold = 20;
        }

        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (order[i].distance_km < distance_threshold){
                order[i].priority = HIGH;
            } else {
                order[i].priority = MEDIUM;
            }
        }
        // implicit barrier here

        #pragma omp single
        {
            printf("priorities are set\n");
        }

        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (order[i].priority == HIGH){
                loc_high++;
            }
        }
        // implicit barrier here

        thread_high_count[id] = loc_high;
        #pragma omp barrier

        #pragma omp single
        {
            int high_count = 0;
            for (int i = 0; i < 4; i++) {
                high_count += thread_high_count[i];
                printf("thread high count for %d is %d\n", i, thread_high_count[i]);
            }

            printf("total high count is %d\n", high_count);
        }
    }

    free(order);
    return 0;
}
