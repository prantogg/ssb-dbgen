#include "spider.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define MAX_DIM 2
#define MAX_LINE 1024

static double rand_unit() {
    return (double)rand() / RAND_MAX;
}

double random_angle(long index) {
    unsigned int seed = 42 + index; // Fixed seed + index = deterministic
    srand(seed);
    return ((double) rand() / RAND_MAX) * 2.0 * M_PI;
}

// Deterministic hash function (SplitMix64 adapted for unsigned int)
unsigned int spider_seed_for_index(unsigned long index, unsigned int global_seed) {
    unsigned long long z = index + global_seed;
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ (z >> 31);
    return (unsigned int)(z & 0xFFFFFFFF);
}

// Box-Muller transform
double rand_normal(double mu, double sigma) {
    return mu + sigma * sqrt(-2.0 * log(rand_unit())) * cos(2.0 * M_PI * rand_unit());
}

// Truncated log-normal
double rand_lognormal_trunc(double mu, double sigma, double min) {
    double val;
    do {
        val = exp(rand_normal(mu, sigma));
    } while (val < min);
    return val;
}

static int dice(int n) {
    return (rand() % n) + 1;
}

void apply_affine(double* point, const double* matrix, double* result) {
    result[0] = matrix[0] * point[0] + matrix[1] * point[1] + matrix[2];
    result[1] = matrix[3] * point[0] + matrix[4] * point[1] + matrix[5];
}

void spider_generate_to_file(
    const char* distribution,
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix
) {
    if (strcmp(distribution, "uniform") == 0) {
        spider_generate_uniform_to_file(filename, cardinality, dim, seed, affineMatrix);
    } else if (strcmp(distribution, "normal") == 0 || strcmp(distribution, "gaussian") == 0) {
        spider_generate_normal_to_file(filename, cardinality, dim, seed, affineMatrix, 0.5, 0.15); // default μ=0.5, σ=0.15
    } else if (strcmp(distribution, "diagonal") == 0) {
        spider_generate_diagonal_to_file(filename, cardinality, dim, seed, affineMatrix, 0.5, 0.1);
    } else {
        fprintf(stderr, "Unsupported distribution: %s\n", distribution);
        exit(1);
    }
}

void write_polygon(FILE* file, double* center, int maxseg, double polysize, const double* affineMatrix) {
    int minSegs = 3;
    int numSegments = (maxseg <= 3) ? 3 : dice(maxseg - minSegs) + minSegs;

    double* angles = malloc(sizeof(double) * numSegments);
    for (int i = 0; i < numSegments; i++) angles[i] = rand_unit() * 2 * M_PI;

    for (int i = 0; i < numSegments - 1; i++) {
        for (int j = i + 1; j < numSegments; j++) {
            if (angles[i] > angles[j]) {
                double tmp = angles[i]; angles[i] = angles[j]; angles[j] = tmp;
            }
        }
    }

    for (int i = 0; i < numSegments; i++) {
        double local[2] = {
            center[0] + polysize * cos(angles[i]),
            center[1] + polysize * sin(angles[i])
        };
        double world[2];
        if (affineMatrix) apply_affine(local, affineMatrix, world);
        else { world[0] = local[0]; world[1] = local[1]; }
        fprintf(file, "%lf,%lf;", world[0], world[1]);
    }

    double local0[2] = {
        center[0] + polysize * cos(angles[0]),
        center[1] + polysize * sin(angles[0])
    };
    double world0[2];
    if (affineMatrix) apply_affine(local0, affineMatrix, world0);
    else { world0[0] = local0[0]; world0[1] = local0[1]; }
    fprintf(file, "%lf,%lf\n", world0[0], world0[1]);

    free(angles);
}

void spider_generate_uniform_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix
) {
    srand(seed);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Could not open output file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < cardinality; i++) {
        double center[2] = { rand_unit(), rand_unit() };
        double output[2];
        if (affineMatrix) apply_affine(center, affineMatrix, output);
        else { output[0] = center[0]; output[1] = center[1]; }

        fprintf(fp, "%lf,%lf\n", output[0], output[1]);
    }

    fclose(fp);
}

void spider_generate_normal_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix,
    double mu,
    double sigma
) {
    srand(seed);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Could not open output file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < cardinality; i++) {
        double center[2];
        for (int d = 0; d < dim; d++) {
            center[d] = rand_normal(mu, sigma);
            if (center[d] < 0.0) center[d] = 0.0;
            if (center[d] > 1.0) center[d] = 1.0;
        }

        double output[2];
        if (affineMatrix) apply_affine(center, affineMatrix, output);
        else { output[0] = center[0]; output[1] = center[1]; }

        fprintf(fp, "%lf,%lf\n", output[0], output[1]);
    }

    fclose(fp);
}

void spider_generate_diagonal_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix,
    double percentage,
    double buffer
) {
    srand(seed);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Could not open output file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < cardinality; i++) {
        double center[2];
        double coin = rand_unit();
        if (coin < percentage) {
            double v = rand_unit();
            center[0] = v;
            center[1] = v;
        } else {
            double c = rand_unit();
            double d = rand_normal(0.0, buffer / 5.0);
            for (int x = 0; x < dim; x++) {
                double val = c + (1 - 2 * (x % 2)) * d / sqrt(2);
                if (val < 0.0) val = 0.0;
                if (val > 1.0) val = 1.0;
                center[x] = val;
            }
        }

        double output[2];
        if (affineMatrix) apply_affine(center, affineMatrix, output);
        else { output[0] = center[0]; output[1] = center[1]; }

        fprintf(fp, "%lf,%lf\n", output[0], output[1]);
    }

    fclose(fp);
}
