#ifndef SUM_LIB_H
#define SUM_LIB_H

#include <stdint.h>

struct SumArgs {
    const int *array;
    unsigned int begin;
    unsigned int end;
};

// Функция, выполняемая отдельным потоком
void *ThreadSum(void *args);

#endif