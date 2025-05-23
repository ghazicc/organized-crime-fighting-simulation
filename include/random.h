//
// Created by - on 3/23/2025.
//

#ifndef RANDOM_H
#define RANDOM_H

#include "gang.h"  // For NUM_ATTRIBUTES

void init_random();
float random_float(float min, float max);

// Box-Muller transform to generate univariate normal distribution
float random_normal(float mean, float stddev);

// Generate correlated attributes using multivariate Gaussian distribution
void generate_multivariate_attributes(float* attributes, const float* means, const float* stddevs, const float correlation_matrix[NUM_ATTRIBUTES][NUM_ATTRIBUTES]);

#endif //RANDOM_H
