#include "sum_lib.h"
#include <stdlib.h>

void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    long long *total_sum = malloc(sizeof(long long));
    *total_sum = 0;

    for (unsigned int i = sum_args->begin; i < sum_args->end; i++) {
        *total_sum += sum_args->array[i];
    }

    return (void *)total_sum;
}