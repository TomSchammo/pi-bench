#ifndef DATA_PROCESSING_H
#define DATA_PROCESSING_H

#include "./bench.h"
#include "./stats.h"
#include <stddef.h>
#include <stdint.h>

void calculate_stats(benchmark_result_t *results, size_t size) {
  uint64_t *samples = results->samples;

  results->median = median(samples, size, insertion_sort);
  results->mean = mean(samples, size);
  results->stddev = stddev(samples, size);

  uint64_t min = samples[0], max = samples[0];
  for (size_t i = 1; i < size; i++) {
    uint64_t val = samples[i];
    if (val > max)
      max = val;
    else if (val < min)
      min = val;
  }

  results->min = min;
  results->max = max;
}

void print_result(benchmark_t *results) {
  if (results == NULL || results->results == NULL) {
    printf("Error: Invalid benchmark results\n");
    return;
  }

  benchmark_result_t *data = results->results;

  printf("\n");
  printf("========================================\n");
  printf("Benchmark: %s\n", results->name);
  printf("========================================\n");
  printf("Iterations: %zu warmup, %zu timed\n", results->warmup_iterations,
         results->timed_iterations);
  printf("Baseline: %s\n", results->is_baseline ? "Yes" : "No");
  printf("\n");
  printf("Statistical Results:\n");
  printf("  Median: %llu cycles\n", data->median);
  printf("  Mean:   %.2f cycles\n", data->mean);
  printf("  StdDev: %.2f cycles\n", data->stddev);
  printf("  Min:    %llu cycles\n", data->min);
  printf("  Max:    %llu cycles\n", data->max);
  printf("========================================\n");
  printf("\n");
}

void print_results(benchmark_t **results, size_t count) {
  if (results == NULL || count == 0) {
    printf("Error: No benchmark results to display\n");
    return;
  }

  // Find the baseline benchmark
  benchmark_t *baseline = NULL;
  for (size_t i = 0; i < count; i++) {
    if (results[i] != NULL && results[i]->is_baseline) {
      baseline = results[i];
      break;
    }
  }

  if (baseline == NULL || baseline->results == NULL) {
    printf("Error: No baseline benchmark found\n");
    return;
  }

  // Create array of indices for sorting
  size_t *indices = malloc(count * sizeof(size_t));
  for (size_t i = 0; i < count; i++) {
    indices[i] = i;
  }

  // Sort by performance: worst to best (higher cycles = worse performance)
  for (size_t i = 0; i < count - 1; i++) {
    for (size_t j = i + 1; j < count; j++) {
      benchmark_t *bench_i = results[indices[i]];
      benchmark_t *bench_j = results[indices[j]];

      // Skip if either benchmark is invalid
      if (bench_i == NULL || bench_j == NULL || bench_i->results == NULL ||
          bench_j->results == NULL) {
        continue;
      }

      // Sort by performance (higher cycles = worse performance)
      if (bench_j->results->median > bench_i->results->median) {
        size_t temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
      }
    }
  }

  // Print header
  printf("\n");
  printf("========================================\n");
  printf("BENCHMARK RESULTS SUMMARY\n");
  printf("========================================\n");
  printf("Baseline: %s (%.2f cycles)\n", baseline->name,
         baseline->results->mean);
  printf("\n");

  // Print results in sorted order
  for (size_t i = 0; i < count; i++) {
    benchmark_t *bench = results[indices[i]];
    if (bench == NULL || bench->results == NULL) {
      continue;
    }

    benchmark_result_t *data = bench->results;
    double relative_performance = 1.0;

    if (!bench->is_baseline) {
      relative_performance =
          (double)data->median / (double)baseline->results->median;
      if (relative_performance < 1.0) {
        double speed_increase = 1.0 / relative_performance;
        printf("%-20s: %8llu cycles (%.2fx) - %.1fx faster\n", bench->name,
               data->median, relative_performance, speed_increase);
        continue;
      }
    } else {
      relative_performance = 1.0;
    }

    printf("%-20s: %8llu cycles (%.2fx)%s\n", bench->name, data->median,
           relative_performance, bench->is_baseline ? " - baseline" : "");
  }

  printf("========================================\n");
  printf("\n");

  free(indices);
}

#endif // DATA_PROCESSING_H
