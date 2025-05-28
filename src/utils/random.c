#include "random.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include "gang.h"

void init_random() {
    srand(time(NULL) ^ getpid());
}

float random_float(float min, float max) {
    float scale = rand() / (float)RAND_MAX;
    return min + scale * (max - min);
}

float random_normal(float mean, float stddev) {
    static int has_spare = 0;
    static float spare;
    
    if (has_spare) {
        has_spare = 0;
        return mean + stddev * spare;
    }
    
    // keep generating u, v until s is valid
    float u, v, s;
    do {
        u = random_float(-1.0f, 1.0f);
        v = random_float(-1.0f, 1.0f);
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);
    
    s = sqrtf(-2.0f * logf(s) / s);
    spare = v * s;
    has_spare = 1;
    
    return mean + stddev * u * s;
}

// Generate correlated attributes using Cholesky decomposition for multivariate normal distribution
void generate_multivariate_attributes(float* attributes, const float* means, const float* stddevs, const float correlation_matrix[NUM_ATTRIBUTES][NUM_ATTRIBUTES]) {
    // Generate independent standard normal random variables
    float z[NUM_ATTRIBUTES];
    for (int i = 0; i < NUM_ATTRIBUTES; i++) {
        z[i] = random_normal(0.0f, 1.0f);
    }
    
    // Apply correlation using Cholesky decomposition (simplified approach)
    // This is a simplified implementation that assumes correlation_matrix is positive definite
    for (int i = 0; i < NUM_ATTRIBUTES; i++) {
        attributes[i] = means[i];
        for (int j = 0; j <= i; j++) {
            attributes[i] += correlation_matrix[i][j] * z[j] * stddevs[i];
        }
        
        // Ensure values are within [0,1] range
        if (attributes[i] < 0.0f) attributes[i] = 0.0f;
        if (attributes[i] > 1.0f) attributes[i] = 1.0f;
    }

}

int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}
