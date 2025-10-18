
#include "./include/bench.h"
#include "./include/data_processing.h"
#include "./include/stats.h"
#include "./include/system.h"
#include <stdio.h>
#include <stdlib.h>

// Example function to benchmark
static void example_function(void) {
  volatile int sum = 0;
  for (int i = 0; i < 1000; i++) {
    sum += i;
  }
}

// Another example function
static void another_function(void) {
  volatile double result = 0.0;
  for (int i = 0; i < 500; i++) {
    result += (double)i * 0.5;
  }
}

// A third function for comparison
static void slow_function(void) {
  volatile int sum = 0;
  for (int i = 0; i < 2000; i++) {
    for (int j = 0; j < 100; j++) {
      sum += i * j;
    }
  }
}

int main() {
  printf("=== C-Bench Library Example ===\n\n");

  // Display system information
  printf("System Information:\n");
  get_system_status();
  printf("\n");

  // Check CPU cores
  int cpu_cores = get_cpu_cores();
  printf("Available CPU cores: %d\n\n", cpu_cores);

  // Example 1: Basic benchmark
  printf("=== Basic Benchmark ===\n");
  benchmark_t benchmark1 = {.name = "Example Function",
                            .warmup_iterations = 100,
                            .timed_iterations = 1000,
                            .results = malloc(sizeof(benchmark_result_t)),
                            .is_baseline = true};

  benchmark1.results->samples =
      malloc(benchmark1.timed_iterations * sizeof(uint64_t));

  // Run the benchmark
  BENCHMARK_FUNC(example_function(), (&benchmark1));

  // Example 2: Pinned benchmark
  printf("=== Pinned Benchmark (Core 3) ===\n");
  benchmark_t benchmark2 = {.name = "Pinned Function",
                            .warmup_iterations = 50,
                            .timed_iterations = 500,
                            .results = malloc(sizeof(benchmark_result_t)),
                            .is_baseline = false};

  benchmark2.results->samples =
      malloc(benchmark2.timed_iterations * sizeof(uint64_t));

  // Run pinned benchmark on core 3
  BENCHMARK_FUNC_PINNED(another_function(), (&benchmark2), 3);

  // Example 3: Another benchmark for comparison
  printf("=== Third Benchmark ===\n");
  benchmark_t benchmark3 = {.name = "Slow Function",
                            .warmup_iterations = 50,
                            .timed_iterations = 500,
                            .results = malloc(sizeof(benchmark_result_t)),
                            .is_baseline = false};

  benchmark3.results->samples =
      malloc(benchmark3.timed_iterations * sizeof(uint64_t));

  // Run the third benchmark
  BENCHMARK_FUNC(slow_function(), (&benchmark3));

  // Calculate statistics for all benchmarks
  printf("\n=== Calculating Statistics ===\n");
  calculate_stats(benchmark1.results, benchmark1.timed_iterations);
  calculate_stats(benchmark2.results, benchmark2.timed_iterations);
  calculate_stats(benchmark3.results, benchmark3.timed_iterations);

  // Print individual results
  printf("\n=== Individual Benchmark Results ===\n");
  print_result(&benchmark1);
  print_result(&benchmark2);
  print_result(&benchmark3);

  // Create array of benchmarks for summary
  benchmark_t *benchmarks[] = {&benchmark1, &benchmark2, &benchmark3};
  size_t benchmark_count = 3;

  // Print comparative results
  printf("=== Comparative Results ===\n");
  print_results(benchmarks, benchmark_count);

  // Cleanup
  free(benchmark1.results->samples);
  free(benchmark1.results);
  free(benchmark2.results->samples);
  free(benchmark2.results);
  free(benchmark3.results->samples);
  free(benchmark3.results);

  printf("Example completed successfully!\n");

  return 0;
}
