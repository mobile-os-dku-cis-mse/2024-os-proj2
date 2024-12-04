#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
typedef enum {
    DISTRIBUTION_UNIFORM,
    DISTRIBUTION_GAUSSIAN,
    DISTRIBUTION_ZIPFIAN
} distribution_t;

extern distribution_t current_distribution;

uint16_t generate_uniform_vaddr();
uint16_t generate_gaussian_vaddr();
uint16_t generate_zipfian_vaddr();
uint16_t generate_vaddr();
#endif 