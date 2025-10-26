#ifndef UTILS_H
#define UTILS_H

#include "./bench.h"
#include "./data_processing.h"
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
  bool gt_freed = false;

  if (free_output_buffer) {
    free(benchmark->results->output_buffer);
  }

  if (benchmark->is_baseline && !gt_freed) {
    free(benchmark->results->gt);
  }

  free(benchmark->results->samples);
  free(benchmark->results->cache_miss_rates);
  free(benchmark->results);
  free(benchmark);
}

#define BENCHMARK_TIME_PINNED(name, is_baseline, validate, output_buffer,      \
                              size, core, func)                                \
  +1
#define BENCHMARK_TIME(name, is_baseline, validate, output_buffer, size, func) \
  +1
#define BENCHMARK_CYCLES_PINNED(name, is_baseline, validate, output_buffer,    \
                                size, core, func)                              \
  +1
#define BENCHMARK_CYCLES(name, is_baseline, validate, output_buffer, size,     \
                         func)                                                 \
  +1
enum { BENCHMARK_COUNT = (0 BENCHMARKS) };
#undef BENCHMARK_TIME_PINNED
#undef BENCHMARK_TIME
#undef BENCHMARK_CYCLES_PINNED
#undef BENCHMARK_CYCLES

static benchmark_t *_benchmark_array[BENCHMARK_COUNT];
static size_t _benchmark_idx = 0;

#define BENCHMARK_TIME_PINNED(name, is_baseline, validate, output_buffer,      \
                              size, core, func)                                \
  {                                                                            \
                                                                               \
    printf("\n=== %s Benchmark ===\n", name);                                  \
                                                                               \
    benchmark_t *benchmark =                                                   \
        setup_benchmark(name, WARMUP_RUNS, TIMED_RUNS, is_baseline, validate,  \
                        output_buffer, size);                                  \
                                                                               \
    if (is_baseline) {                                                         \
      gt = benchmark->results->gt;                                             \
    } else {                                                                   \
      benchmark->results->gt = gt;                                             \
    }                                                                          \
                                                                               \
    BENCHMARK_FUNC_PINNED(func, benchmark, core);                              \
                                                                               \
    memset(output_buffer, 0, size);                                            \
    _benchmark_array[_benchmark_idx++] = benchmark;                            \
  }

#define BENCHMARK_TIME(name, is_baseline, validate, output_buffer, size, func) \
  {                                                                            \
                                                                               \
    printf("\n=== %s Benchmark ===\n", name);                                  \
                                                                               \
    benchmark_t *benchmark =                                                   \
        setup_benchmark(name, WARMUP_RUNS, TIMED_RUNS, is_baseline, validate,  \
                        output_buffer, size);                                  \
                                                                               \
    if (is_baseline) {                                                         \
      gt = benchmark->results->gt;                                             \
    } else {                                                                   \
      benchmark->results->gt = gt;                                             \
    }                                                                          \
                                                                               \
    BENCHMARK_FUNC(func, benchmark);                                           \
                                                                               \
    memset(output_buffer, 0, size);                                            \
    _benchmark_array[_benchmark_idx++] = benchmark;                            \
  }

#define BENCHMARK_CYCLES_PINNED(name, is_baseline, validate, output_buffer,    \
                                size, core, func)                              \
  {                                                                            \
                                                                               \
    printf("\n=== %s Benchmark ===\n", name);                                  \
                                                                               \
    benchmark_t *benchmark =                                                   \
        setup_benchmark(name, WARMUP_RUNS, TIMED_RUNS, is_baseline, validate,  \
                        output_buffer, size);                                  \
                                                                               \
    if (is_baseline) {                                                         \
      gt = benchmark->results->gt;                                             \
    } else {                                                                   \
      benchmark->results->gt = gt;                                             \
    }                                                                          \
                                                                               \
    BENCHMARK_FUNC_CYCLES_PINNED(func, benchmark, core);                       \
                                                                               \
    memset(output_buffer, 0, size);                                            \
    _benchmark_array[_benchmark_idx++] = benchmark;                            \
  }

#define BENCHMARK_CYCLES(name, is_baseline, validate, output_buffer, size,     \
                         func)                                                 \
  {                                                                            \
                                                                               \
    printf("\n=== %s Benchmark ===\n", name);                                  \
                                                                               \
    benchmark_t *benchmark =                                                   \
        setup_benchmark(name, WARMUP_RUNS, TIMED_RUNS, is_baseline, validate,  \
                        output_buffer, size);                                  \
                                                                               \
    if (is_baseline) {                                                         \
      gt = benchmark->results->gt;                                             \
    } else {                                                                   \
      benchmark->results->gt = gt;                                             \
    }                                                                          \
                                                                               \
    BENCHMARK_FUNC_CYCLES(func, benchmark);                                    \
                                                                               \
    memset(output_buffer, 0, size);                                            \
    _benchmark_array[_benchmark_idx++] = benchmark;                            \
  }

#define RUN_BENCHMARKS() BENCHMARKS

#define PRINT_RESULTS_INDIVIDUAL()                                             \
  do {                                                                         \
    printf("\n=== Individual Benchmark Results ===\n");                        \
    if (BENCHMARK_COUNT > 0) {                                                 \
      for (size_t i = 0; i < BENCHMARK_COUNT; i++) {                           \
        benchmark_t *b = _benchmark_array[i];                                  \
        calculate_stats(b->results, b->timed_iterations);                      \
        print_result(b);                                                       \
      }                                                                        \
    }                                                                          \
  } while (0)

#define PRINT_RESULTS_GROUP(print_invalid)                                     \
  do {                                                                         \
    printf("=== Comparative Results ===\n");                                   \
    print_results(_benchmark_array, BENCHMARK_COUNT, print_invalid);           \
  } while (0)

#define SAVE(dir)                                                              \
  do {                                                                         \
    to_csv(_benchmark_array, BENCHMARK_COUNT, dir);                            \
  } while (0)

#define CLEANUP()                                                              \
  do {                                                                         \
    if (BENCHMARK_COUNT > 0) {                                                 \
      for (size_t i = 0; i < BENCHMARK_COUNT; i++) {                           \
        cleanup_benchmark(_benchmark_array[i], false);                         \
      }                                                                        \
    }                                                                          \
  } while (0)

#endif // UTILS_H
