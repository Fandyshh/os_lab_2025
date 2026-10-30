#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>

#include "utils.h"
#include "sum_lib.h"

int main(int argc, char **argv) {
    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;

    while (true) {
        static struct option options[] = {
            {"threads_num", required_argument, 0, 0},
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        threads_num = atoi(optarg);
                        break;
                    case 1:
                        seed = atoi(optarg);
                        break;
                    case 2:
                        array_size = atoi(optarg);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);

    unsigned int chunk_size = array_size / threads_num;

    // Начало замера времени вычислений (генерация массива не включена)
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * chunk_size;
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;

        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
            perror("pthread_create failed");
            return 1;
        }
    }

    long long total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        long long *sub_sum = NULL;
        pthread_join(threads[i], (void **)&sub_sum);
        if (sub_sum != NULL) {
            total_sum += *sub_sum;
            free(sub_sum);
        }
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(threads);
    free(args);

    printf("Total sum: %lld\n", total_sum);
    printf("Elapsed time: %fms\n", elapsed_time);

    return 0;
}