#ifndef DATA_PROCESSING_H
#define DATA_PROCESSING_H

#include "./bench.h"
#include "./stats.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *get_validation_buffer(const void *const gt, size_t size) {
  void *buffer = malloc(size);
  memcpy(buffer, gt, size);

  return buffer;
}

void validate_result(benchmark_t *benchmark, const void *const result,
                     const void *const gt, size_t size) {

  if (memcmp(gt, result, size) == 0) {
    benchmark->is_valid = true;
    printf("\033[32mResult of '%s' is valid!\033[0m\n", benchmark->name);
  } else {

    benchmark->is_valid = false;
    printf("\033[33mResult of '%s' is not valid!\033[0m\n", benchmark->name);
  }
}

void calculate_stats(benchmark_result_t *results, size_t size) {
  uint64_t *samples = results->samples;
  double *cmrs = results->cache_miss_rates;

  results->median_time = median(samples, size, selection_sort);
  results->mean_time = mean(samples, size);
  results->stddev_time = stddev(samples, size);

  results->median_cmr = median(cmrs, size, selection_sort);
  results->mean_cmr = mean(cmrs, size);
  results->stddev_cmr = stddev(cmrs, size);

  uint64_t min_time = samples[0], max_time = samples[0];
  double min_cmr = cmrs[0], max_cmr = cmrs[0];
  for (size_t i = 1; i < size; i++) {
    uint64_t val = samples[i];
    double cmr = cmrs[i];

    if (val > max_time)
      max_time = val;
    else if (val < min_time)
      min_time = val;

    if (cmr > max_cmr)
      max_cmr = cmr;
    else if (cmr < min_cmr)
      min_cmr = cmr;
  }

  results->min_time = min_time;
  results->max_time = max_time;

  results->min_cmr = min_cmr;
  results->max_cmr = max_cmr;
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
  printf("Time:\n");
  printf("  Median: %llu %s\n", data->median_time,
         results->results->is_cycles ? "cycles" : "us");
  printf("  Mean:   %.2f %s\n", data->mean_time,
         results->results->is_cycles ? "cycles" : "us");
  printf("  StdDev: %.2f %s\n", data->stddev_time,
         results->results->is_cycles ? "cycles" : "us");
  printf("  Min:    %llu %s\n", data->min_time,
         results->results->is_cycles ? "cycles" : "us");
  printf("  Max:    %llu %s\n", data->max_time,
         results->results->is_cycles ? "cycles" : "us");
  printf("\nCache-Miss Rate:\n");
  printf("  Median: %.2f%% \n", data->median_cmr);
  printf("  Mean:   %.2f%% \n", data->mean_cmr);
  printf("  StdDev: %.2f%% \n", data->stddev_cmr);
  printf("  Min:    %.2f%% \n", data->min_cmr);
  printf("  Max:    %.2f%% \n", data->max_cmr);
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
  size_t *indices = (size_t *)malloc(count * sizeof(size_t));
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
      if (bench_j->results->median_time > bench_i->results->median_time) {
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
  printf("Baseline: %s (%.2f %s)\n", baseline->name,
         baseline->results->mean_time,
         baseline->results->is_cycles ? "cycles" : "us");
  printf("\n");

  // Print results in sorted order
  for (size_t i = 0; i < count; i++) {
    benchmark_t *bench = results[indices[i]];
    if (bench == NULL || bench->results == NULL || !bench->is_valid) {
      continue;
    }

    benchmark_result_t *data = bench->results;
    double relative_performance = 1.0;

    if (!bench->is_baseline) {
      relative_performance =
          (double)data->median_time / (double)baseline->results->median_time;
      if (relative_performance < 1.0) {
        double speed_increase = 1.0 / relative_performance;
        printf("%-20s: %8llu %s (%.2fx) - %.1fx faster\n", bench->name,
               data->median_time, data->is_cycles ? "cycles" : "us",
               relative_performance, speed_increase);
        continue;
      }
    } else {
      relative_performance = 1.0;
    }

    printf("%-20s: %8llu %s (%.2fx)%s\n", bench->name, data->median_time,
           bench->results->is_cycles ? "cycles" : "us", relative_performance,
           bench->is_baseline ? " - baseline" : "");
  }

  printf("========================================\n");
  printf("\n");

  free(indices);
}

bool to_csv(benchmark_t **benchmarks, size_t num, const char *dir) {

  for (size_t i = 0; i < num; i++) {
    benchmark_t *benchmark = benchmarks[i];
    benchmark_result_t *results = benchmark->results;
    uint64_t *samples = results->samples;
    double *cmr = results->cache_miss_rates;
    const char *name = benchmark->name;
    size_t len = strlen(name) + 1;

    char *filename = (char *)malloc(len);
    if (filename == NULL) {
      fprintf(stderr, "Error: Could not allocate memory for header\n");
      return false;
    }

    for (size_t cidx = 0; cidx < len; cidx++) {
      char c = name[cidx];
      if (c == ' ')
        filename[cidx] = '_';
      else
        filename[cidx] = c;
    }

    char path[256];

    int written =
        snprintf(path, sizeof(path), "%s/benchmark_%s.csv", dir, filename);
    if (written >= sizeof(path)) {
      fprintf(stderr, "Error: Path too long\n");
      free(filename);
      return false;
    }

    FILE *csv = fopen(path, "w");
    if (csv == NULL) {
      fprintf(stderr, "Error: Could not open file %s for writing\n", dir);
      return false;
    }

    fprintf(csv,
            "# name: %s\n# timing format: %s\n# is valid: %s\n# warmup runs: "
            "%lu\n# timed runs: %lu\n\ntiming,cache_miss_rate\n",
            name, benchmark->results->is_cycles ? "cycles" : "microseconds",
            benchmark->validate ? (benchmark->is_valid ? "Yes" : "No")
                                : "Not Validated",
            benchmark->warmup_iterations, benchmark->timed_iterations);

    for (size_t i = 0; i < benchmark->timed_iterations; i++) {
      fprintf(csv, "%llu,%0.2f\n", samples[i], cmr[i]);
    }

    fclose(csv);
    free(filename);
  }

  return true;
}
#endif // DATA_PROCESSING_H
