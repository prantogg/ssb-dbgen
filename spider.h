#ifndef SPIDER_H
#define SPIDER_H

void spider_generate_uniform_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix // Can be NULL
);

void spider_generate_normal_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix,
    double mu,
    double sigma
);

void spider_generate_diagonal_to_file(
    const char* filename,
    int cardinality,
    int dim,
    unsigned int seed,
    const double* affineMatrix,
    double percentage,
    double buffer
);

unsigned int spider_seed_for_index(unsigned long index, unsigned int global_seed);

double rand_normal(double mean, double stddev);

double rand_lognormal_trunc(double mu, double sigma, double min);

#endif