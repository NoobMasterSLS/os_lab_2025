#include "sum_lib.h"

long long sum_range(int *array, int start, int end) {
    long long sum = 0;
    for (int i = start; i < end; i++) {
        sum += array[i];
    }
    return sum;
}
