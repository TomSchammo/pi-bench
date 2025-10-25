#ifndef UTILS_H
#define UTILS_H

#include "./bench.h"
#include <stdint.h>
#include <stdlib.h>

static inline benchmark_t *setup_benchmark(const char *name,
                                           size_t warmup_iterations,
                                           size_t timed_iterations,
                                           bool is_baseline, bool validate,
                                           void *output_buffer, size_t size) {
  benchmark_t *benchmark = (benchmark_t *)malloc(sizeof(benchmark_t));
  benchmark->name = name;
  benchmark->warmup_iterations = warmup_iterations;
  benchmark->timed_iterations = timed_iterations;
  benchmark->is_baseline = is_baseline;
  benchmark->validate = validate;
  benchmark->is_valid = false;

  benchmark_result_t *results =
      (benchmark_result_t *)malloc(sizeof(benchmark_result_t));
  results->samples = (uint64_t *)calloc(timed_iterations, sizeof(uint64_t));
  results->cache_miss_rates =
      (double *)calloc(timed_iterations, sizeof(double));

  if (is_baseline) {
    results->gt = calloc(size, 1);
  } else {
    results->gt = NULL;
  }

  if (output_buffer != NULL) {
    results->output_buffer = output_buffer;
  } else {
    results->output_buffer = calloc(size, 1);
  }

  results->size = size;
  benchmark->results = results;

  return benchmark;
}

static inline void cleanup_benchmark(benchmark_t *benchmark,
                                     bool free_output_buffer) {
  if (free_output_buffer) {
    free(benchmark->results->output_buffer);
  }
  free(benchmark->results->samples);
  free(benchmark->results->cache_miss_rates);
  free(benchmark->results);
  free(benchmark);
}

#endif // UTILS_H
