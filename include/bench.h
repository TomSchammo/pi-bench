#ifndef BENCH_H
#define BENCH_H

#define _GNU_SOURCE
#include "./system.h"
#include <assert.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

/**
 * @brief Structure to hold file descriptors for L1 cache access and miss
 * counters.
 */
typedef struct {
  int refs_fd;
  int miss_fd;
} cache_counter_t;

/**
 * @brief Structure containing benchmark measurement results and statistics.
 *
 * This structure holds the raw timing samples and computed statistical
 * metrics from a benchmark run, including central tendency and dispersion
 * measures.
 *
 * samples:             Raw timing samples in CPU cycles
 * median:              Median timing value in CPU cycles
 * cache_miss_rates:    L1 cache miss rates for every timed iteration
 * mean:                Mean timing value in CPU cycles
 * stddev:              Standard deviation of timing values
 * min:                 Minimum timing value in CPU cycles
 * max:                 Maximum timing value in CPU cycles
 */
typedef struct {
  uint64_t *samples;
  uint64_t median;
  uint64_t min, max;
  double *cache_miss_rates;
  double mean, stddev;
  bool is_cycles;
} benchmark_result_t;

/**
 * @brief Structure defining a benchmark configuration and its results.
 *
 * This structure contains all the parameters needed to run a benchmark,
 * including the benchmark name, iteration counts, and result storage.
 *
 * name:                Human-readable name of the benchmark
 * warmup_iterations:   Number of warmup iterations to run
 * timed_iterations:    Number of timed iterations to measure
 * results:             Pointer to store benchmark results
 * is_baseline:         Flag indicating if this is a baseline benchmark
 * validate:            Flag indicating if the benchmark result is valid or not
 */
typedef struct {
  const char *name;
  size_t warmup_iterations;
  size_t timed_iterations;
  benchmark_result_t *results;
  bool is_baseline;
  bool is_valid;
} benchmark_t;

/**
 * @brief Macro for running a benchmark with CPU core pinning and real-time
 * scheduling.
 *
 * This macro provides a complete benchmark execution framework that:
 * - Pins the process to a specific CPU core
 * - Sets real-time scheduling priority
 * - Blocks signals to prevent interruption
 * - Performs warmup and timed iterations
 * - Measures cycle counts with overhead compensation
 * - Restores original system settings
 *
 * @param func_call The function call to benchmark (e.g., my_function())
 * @param benchmark Pointer to benchmark_t structure with configuration
 * @param core CPU core number to pin the benchmark to
 *
 * @note Requires appropriate privileges for CPU affinity and real-time
 * scheduling
 * @note Automatically handles system state restoration
 * @note Should be used for single-core benchmarks
 * @note Uses cycle counting for high-precision timing
 * @note Includes thermal monitoring and system status reporting
 */
#define BENCHMARK_FUNC_PINNED(func_call, benchmark, core)                      \
  do {                                                                         \
                                                                               \
    size_t warmup_iterations = benchmark->warmup_iterations;                   \
    size_t timed_iterations = benchmark->timed_iterations;                     \
                                                                               \
    uint64_t *samples = malloc((timed_iterations) * sizeof(uint64_t));         \
                                                                               \
    printf("\033[32mRunning benchmark: %s\033[0m\n", benchmark->name);         \
                                                                               \
    if (benchmark->is_baseline) {                                              \
      printf("\033[32mThis is a baseline run!\033[0m\n");                      \
    }                                                                          \
                                                                               \
    printf("\033[32mRunning %lu warmup iterations, followed by %lu timed "     \
           "iterations...\033[0m\n",                                           \
           warmup_iterations, timed_iterations);                               \
                                                                               \
    disable_cpu_scaling(core);                                                 \
    printf("\033[33mDisabled CPU scaling for core %d!\033[0m\n", core);        \
                                                                               \
    system_wait();                                                             \
                                                                               \
    /* save old cpu set */                                                     \
    cpu_set_t old_set;                                                         \
    CPU_ZERO(&old_set);                                                        \
    sched_getaffinity(0, sizeof(cpu_set_t), &old_set);                         \
                                                                               \
    /* pin process to a core */                                                \
    cpu_set_t cpuset;                                                          \
    CPU_ZERO(&cpuset);                                                         \
    CPU_SET(core, &cpuset);                                                    \
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {               \
      perror("Failed to set affinity!");                                       \
    }                                                                          \
                                                                               \
    printf("\033[33mPinned process to core %d!\033[0m\n", core);               \
                                                                               \
    /* save old schduling policy */                                            \
    int old_policy = sched_getscheduler(0);                                    \
    struct sched_param old_sp;                                                 \
    sched_getparam(0, &old_sp);                                                \
                                                                               \
    /* set priority to highest */                                              \
    struct sched_param sp = {.sched_priority = 99};                            \
    sched_setscheduler(0, SCHED_FIFO, &sp);                                    \
                                                                               \
    printf("\033[33mSet scheduling settings!\033[0m\n");                       \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mBlocking signals in current thread!\033[0m\n");            \
                                                                               \
    struct timespec start, end;                                                \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
    get_system_status();                                                       \
                                                                               \
    /* Warmup */                                                               \
    for (size_t i = 0; i < warmup_iterations; i++) {                           \
      func_call;                                                               \
    }                                                                          \
                                                                               \
    /* Measure */                                                              \
    for (size_t i = 0; i < (timed_iterations); i++) {                          \
      COMPILER_BARRIER();                                                      \
      cache_counter_t counter = start_l1_cache_miss_counter();                 \
      clock_gettime(CLOCK_MONOTONIC, &start);                                  \
      func_call;                                                               \
      clock_gettime(CLOCK_MONOTONIC, &end);                                    \
      double miss_rate = stop_l1_cache_miss_counter(&counter);                 \
      COMPILER_BARRIER();                                                      \
      samples[i] = (end.tv_sec - start.tv_sec) * 1000000 +                     \
                   (end.tv_nsec - start.tv_nsec) / 1000;                       \
      cache_miss_rates[i] = miss_rate;                                         \
    }                                                                          \
                                                                               \
    printf("\033[32mCollected %lu samples!\033[0m\n", timed_iterations);       \
                                                                               \
    get_system_status();                                                       \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
                                                                               \
    benchmark->results->samples = samples;                                     \
    benchmark->results->is_cycles = false;                                     \
                                                                               \
    enable_cpu_scaling(core);                                                  \
    printf("\033[33mRe-enabled CPU scaling for core %d!\033[0m\n", core);      \
                                                                               \
    sched_setaffinity(0, sizeof(cpu_set_t), &old_set);                         \
    printf("\033[33mRestored CPU affinity!\033[0m\n");                         \
                                                                               \
    sched_setscheduler(0, old_policy, &old_sp);                                \
                                                                               \
    printf("\033[33mRestored scheduling settings!\033[0m\n");                  \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mUnblocking signals in current thread!\033[0m\n");          \
  } while (0)

/**
 * @brief Macro for running a benchmark without CPU core pinning.
 *
 * This macro provides a benchmark execution framework that:
 * - Blocks signals to prevent interruption
 * - Performs warmup and timed iterations
 * - Measures cycle counts with overhead compensation
 * - Does NOT pin to specific CPU cores (allows for OS or custom scheduling)
 *
 * @param func_call The function call to benchmark (e.g., my_function())
 * @param benchmark Pointer to benchmark_t structure with configuration
 *
 * @note Should be used for multi-core benchmarks
 * @note Automatically handles signal blocking/unblocking
 * @note Uses cycle counting for high-precision timing
 * @note Includes thermal monitoring and system status reporting
 */
#define BENCHMARK_FUNC(func_call, benchmark)                                   \
  do {                                                                         \
                                                                               \
    size_t warmup_iterations = benchmark->warmup_iterations;                   \
    size_t timed_iterations = benchmark->timed_iterations;                     \
                                                                               \
    uint64_t *samples = malloc((timed_iterations) * sizeof(uint64_t));         \
                                                                               \
    printf("\033[32mRunning benchmark: %s\033[0m\n", benchmark->name);         \
                                                                               \
    if (benchmark->is_baseline) {                                              \
      printf("\033[32mThis is a baseline run!\033[0m\n");                      \
    }                                                                          \
                                                                               \
    printf("\033[32mRunning %lu warmup iterations, followed by %lu timed "     \
           "iterations...\033[0m\n",                                           \
           warmup_iterations, timed_iterations);                               \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mBlocking signals in current thread!\033[0m\n");            \
                                                                               \
    struct timespec start, end;                                                \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
    get_system_status();                                                       \
                                                                               \
    /* Warmup */                                                               \
    for (size_t i = 0; i < warmup_iterations; i++) {                           \
      func_call;                                                               \
    }                                                                          \
                                                                               \
    /* Measure */                                                              \
    for (size_t i = 0; i < (timed_iterations); i++) {                          \
      COMPILER_BARRIER();                                                      \
      clock_gettime(CLOCK_MONOTONIC, &start);                                  \
      func_call;                                                               \
      clock_gettime(CLOCK_MONOTONIC, &end);                                    \
      COMPILER_BARRIER();                                                      \
      samples[i] = (end.tv_sec - start.tv_sec) * 1000000 +                     \
                   (end.tv_nsec - start.tv_nsec) / 1000;                       \
    }                                                                          \
                                                                               \
    printf("\033[32mCollected %lu samples!\033[0m\n", timed_iterations);       \
                                                                               \
    get_system_status();                                                       \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
                                                                               \
    benchmark->results->samples = samples;                                     \
    benchmark->results->is_cycles = false;                                     \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mUnblocking signals in current thread!\033[0m\n");          \
                                                                               \
  } while (0)

/**
 * @brief Compiler memory barrier.
 *
 * Prevents the compiler from reordering memory operations across this point,
 * ensuring accurate timing measurements by preventing optimization artifacts.
 */
#define COMPILER_BARRIER() __asm__("" ::: "memory")

/**
 * @brief Temperature limit at which thermal throttling starts
 *
 * Used for thermal throttling warnings during benchmark execution.
 * Benchmarks results may be skewed if temperature exceeds this threshold.
 */
#define MAX_TEMP 70

/**
 * @brief Macro for running a benchmark with CPU core pinning and real-time
 * scheduling.
 *
 * This macro provides a complete benchmark execution framework that:
 * - Pins the process to a specific CPU core
 * - Sets real-time scheduling priority
 * - Blocks signals to prevent interruption
 * - Performs warmup and timed iterations
 * - Measures cycle counts with overhead compensation
 * - Restores original system settings
 *
 * @param func_call The function call to benchmark (e.g., my_function())
 * @param benchmark Pointer to benchmark_t structure with configuration
 * @param core CPU core number to pin the benchmark to
 *
 * @note Requires appropriate privileges for CPU affinity and real-time
 * scheduling
 * @note Automatically handles system state restoration
 * @note Should be used for single-core benchmarks
 * @note Uses cycle counting for high-precision timing
 * @note Includes thermal monitoring and system status reporting
 */
#define BENCHMARK_FUNC_PINNED_CYCLES(func_call, benchmark, core)               \
  do {                                                                         \
                                                                               \
    size_t warmup_iterations = benchmark->warmup_iterations;                   \
    size_t timed_iterations = benchmark->timed_iterations;                     \
                                                                               \
    uint64_t *samples = malloc((timed_iterations) * sizeof(uint64_t));         \
                                                                               \
    printf("\033[32mRunning benchmark: %s\033[0m\n", benchmark->name);         \
                                                                               \
    if (benchmark->is_baseline) {                                              \
      printf("\033[32mThis is a baseline run!\033[0m\n");                      \
    }                                                                          \
                                                                               \
    printf("\033[32mRunning %lu warmup iterations, followed by %lu timed "     \
           "iterations...\033[0m\n",                                           \
           warmup_iterations, timed_iterations);                               \
                                                                               \
    disable_cpu_scaling(core);                                                 \
    printf("\033[33mDisabled CPU scaling for core %d!\033[0m\n", core);        \
                                                                               \
    system_wait();                                                             \
                                                                               \
    /* save old cpu set */                                                     \
    cpu_set_t old_set;                                                         \
    CPU_ZERO(&old_set);                                                        \
    sched_getaffinity(0, sizeof(cpu_set_t), &old_set);                         \
                                                                               \
    /* pin process to a core */                                                \
    cpu_set_t cpuset;                                                          \
    CPU_ZERO(&cpuset);                                                         \
    CPU_SET(core, &cpuset);                                                    \
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) != 0) {               \
      perror("Failed to set affinity!");                                       \
    }                                                                          \
                                                                               \
    printf("\033[33mPinned process to core %d!\033[0m\n", core);               \
                                                                               \
    /* save old schduling policy */                                            \
    int old_policy = sched_getscheduler(0);                                    \
    struct sched_param old_sp;                                                 \
    sched_getparam(0, &old_sp);                                                \
                                                                               \
    /* set priority to highest */                                              \
    struct sched_param sp = {.sched_priority = 99};                            \
    sched_setscheduler(0, SCHED_FIFO, &sp);                                    \
                                                                               \
    printf("\033[33mSet scheduling settings!\033[0m\n");                       \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mBlocking signals in current thread!\033[0m\n");            \
                                                                               \
    uint64_t cycle_count_overhead = get_cycle_count_overhead();                \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
    get_system_status();                                                       \
                                                                               \
    /* Warmup */                                                               \
    for (size_t i = 0; i < warmup_iterations; i++) {                           \
      func_call;                                                               \
    }                                                                          \
                                                                               \
    /* Measure */                                                              \
    for (size_t i = 0; i < (timed_iterations); i++) {                          \
      COMPILER_BARRIER();                                                      \
      uint64_t start = get_cycles();                                           \
      func_call;                                                               \
      uint64_t end = get_cycles();                                             \
      COMPILER_BARRIER();                                                      \
      samples[i] = (end - start) - cycle_count_overhead;                       \
    }                                                                          \
                                                                               \
    printf("\033[32mCollected %lu samples!\033[0m\n", timed_iterations);       \
                                                                               \
    get_system_status();                                                       \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
                                                                               \
    benchmark->results->samples = samples;                                     \
    benchmark->results->is_cycles = true;                                      \
                                                                               \
    enable_cpu_scaling(core);                                                  \
    printf("\033[33mRe-enabled CPU scaling for core %d!\033[0m\n", core);      \
                                                                               \
    sched_setaffinity(0, sizeof(cpu_set_t), &old_set);                         \
    printf("\033[33mRestored CPU affinity!\033[0m\n");                         \
                                                                               \
    sched_setscheduler(0, old_policy, &old_sp);                                \
                                                                               \
    printf("\033[33mRestored scheduling settings!\033[0m\n");                  \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mUnblocking signals in current thread!\033[0m\n");          \
  } while (0)

/**
 * @brief Macro for running a benchmark without CPU core pinning.
 *
 * This macro provides a benchmark execution framework that:
 * - Blocks signals to prevent interruption
 * - Performs warmup and timed iterations
 * - Measures cycle counts with overhead compensation
 * - Does NOT pin to specific CPU cores (allows for OS or custom scheduling)
 *
 * @param func_call The function call to benchmark (e.g., my_function())
 * @param benchmark Pointer to benchmark_t structure with configuration
 *
 * @note Should be used for multi-core benchmarks
 * @note Automatically handles signal blocking/unblocking
 * @note Uses cycle counting for high-precision timing
 * @note Includes thermal monitoring and system status reporting
 */
#define BENCHMARK_FUNC_CYCLES(func_call, benchmark)                            \
  do {                                                                         \
                                                                               \
    size_t warmup_iterations = benchmark->warmup_iterations;                   \
    size_t timed_iterations = benchmark->timed_iterations;                     \
                                                                               \
    uint64_t *samples = malloc((timed_iterations) * sizeof(uint64_t));         \
                                                                               \
    printf("\033[32mRunning benchmark: %s\033[0m\n", benchmark->name);         \
                                                                               \
    if (benchmark->is_baseline) {                                              \
      printf("\033[32mThis is a baseline run!\033[0m\n");                      \
    }                                                                          \
                                                                               \
    printf("\033[32mRunning %lu warmup iterations, followed by %lu timed "     \
           "iterations...\033[0m\n",                                           \
           warmup_iterations, timed_iterations);                               \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mBlocking signals in current thread!\033[0m\n");            \
                                                                               \
    uint64_t cycle_count_overhead = get_cycle_count_overhead();                \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
    get_system_status();                                                       \
                                                                               \
    /* Warmup */                                                               \
    for (size_t i = 0; i < warmup_iterations; i++) {                           \
      func_call;                                                               \
    }                                                                          \
                                                                               \
    /* Measure */                                                              \
    for (size_t i = 0; i < (timed_iterations); i++) {                          \
      COMPILER_BARRIER();                                                      \
      uint64_t start = get_cycles();                                           \
      func_call;                                                               \
      uint64_t end = get_cycles();                                             \
      COMPILER_BARRIER();                                                      \
      samples[i] = (end - start) - cycle_count_overhead;                       \
    }                                                                          \
                                                                               \
    printf("\033[32mCollected %lu samples!\033[0m\n", timed_iterations);       \
                                                                               \
    get_system_status();                                                       \
                                                                               \
    throttle_warning(MAX_TEMP);                                                \
                                                                               \
    benchmark->results->samples = samples;                                     \
    benchmark->results->is_cycles = true;                                      \
                                                                               \
    block_all_signals_in_this_thread();                                        \
    printf("\033[33mUnblocking signals in current thread!\033[0m\n");          \
                                                                               \
  } while (0)

/**
 * @brief Disables CPU frequency scaling by setting the governor to performance
 * mode for a specific core.
 *
 * Sets the CPU frequency governor to "performance" mode for the specified CPU
 * core, which keeps that CPU at its maximum frequency. This ensures consistent
 * timing measurements by preventing frequency changes during benchmark
 * execution on the specified core.
 *
 * @param core The CPU core number to disable scaling for (0-based indexing)
 *
 * @note Requires root privileges or appropriate permissions
 * @note Only affects the specified CPU core
 * @note Uses the system() function to execute cpufreq governor command
 */
static inline void disable_cpu_scaling(int core) {
  char command[256];
  snprintf(command, sizeof(command),
           "echo performance > "
           "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor",
           core);
  system(command);
}

/**
 * @brief Re-enables CPU frequency scaling by restoring the default governor for
 * a specific core.
 *
 * Restores the CPU frequency governor to "ondemand" mode for the specified CPU
 * core, which allows the CPU to scale its frequency based on load. This
 * reverses the effect of disable_cpu_scaling() and restores normal power
 * management behavior.
 *
 * @param core The CPU core number to re-enable scaling for (0-based indexing)
 *
 * @note Requires root privileges or appropriate permissions
 * @note Only affects the specified CPU core
 * @note Uses the system() function to execute cpufreq governor command
 * @note Restores normal power management and frequency scaling behavior
 */
static inline void enable_cpu_scaling(int core) {
  char command[256];
  snprintf(
      command, sizeof(command),
      "echo ondemand > /sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor",
      core);
  system(command);
}

/**
 * @brief Enables user-space access to performance monitoring units (PMU).
 *
 * Opens /dev/cpu_dma_latency to prevent the system from entering deep sleep
 * states during benchmark execution. This helps maintain consistent timing
 * measurements by preventing CPU power management from affecting performance.
 *
 * @return File descriptor for /dev/cpu_dma_latency, or -1 on error
 *
 * @note Typically requires root privileges or kernel module support
 * @note The returned file descriptor should remain open during benchmark
 * execution
 * @note Helps prevent CPU frequency scaling and sleep state transitions
 * @note Caller is responsible for closing the returned file descriptor
 */
static inline int enable_pmu_user_access(void) {
  // This typically requires a kernel module or running as root
  // Write to /dev/cpu_dma_latency to prevent deep sleep states
  int fd = open("/dev/cpu_dma_latency", O_WRONLY);
  if (fd >= 0) {
    uint32_t latency = 0;
    write(fd, &latency, sizeof(latency));
    // Keep fd open during benchmarks
  }
  return fd;
}

/**
 * @brief Disables user-space access to performance monitoring units (PMU).
 *
 * Closes the /dev/cpu_dma_latency file descriptor to restore normal CPU power
 * management behavior. This reverses the effect of enable_pmu_user_access()
 * and allows the system to enter deep sleep states again.
 *
 * @param fd File descriptor returned by enable_pmu_user_access()
 *
 * @note Should be called after benchmark execution to restore normal behavior
 * @note Restores normal CPU power management and sleep state transitions
 * @note Counterpart to enable_pmu_user_access()
 */
static inline void disable_pmu_user_access(int fd) {
  if (fd >= 0) {
    close(fd);
  }
}

/**
 * @brief Measures the overhead of the cycle counting mechanism itself.
 *
 * Calculates the time required to read the cycle counter twice in succession,
 * which represents the measurement overhead. This value is subtracted from
 * benchmark measurements to get more accurate timing results.
 *
 * @return The cycle count overhead in CPU cycles
 */
[[nodiscard]] static inline uint64_t get_cycle_count_overhead(void) {
  uint64_t start = get_cycles();
  uint64_t end = get_cycles();
  return end - start;
}

/**
 * @brief Configures pthread attributes for high-priority, CPU-pinned threads.
 *
 * Sets up thread attributes for optimal benchmark execution by:
 * - Pinning the thread to a specific CPU core
 * - Setting real-time scheduling policy (SCHED_FIFO)
 * - Setting maximum priority (99)
 *
 * @param cpu_core The CPU core to pin the thread to (0-based indexing)
 * @param attr Pointer to pthread_attr_t structure to configure
 *
 * @note Requires appropriate privileges for real-time scheduling
 * @note Thread will be bound to the specified CPU core
 * @note Uses highest available priority for minimal scheduling latency
 */
static inline void setup_thread_attributes(int cpu_core, pthread_attr_t *attr) {

  pthread_attr_init(attr);

  // bind to CPU
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_core, &cpuset);
  pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpuset);

  // set scheduling policy to real-time
  pthread_attr_setschedpolicy(attr, SCHED_FIFO);

  // set priority to highest
  struct sched_param sp = {.sched_priority = 99};
  pthread_attr_setschedparam(attr, &sp);
}

/**
 * @brief Blocks all signals in the current thread to prevent benchmark
 * interruption.
 *
 * Prevents signal delivery to the current thread, which could cause
 * timing variations or benchmark interruption. This ensures consistent
 * benchmark execution by eliminating signal-related timing artifacts.
 *
 * @note Blocks all signals using SIG_BLOCK with a full signal set
 * @note Should be paired with unblock_all_signals_in_this_thread()
 */
static void block_all_signals_in_this_thread(void) {
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
}

/**
 * @brief Unblocks all signals in the current thread.
 *
 * Restores normal signal handling in the current thread after benchmark
 * execution. This is the counterpart to block_all_signals_in_this_thread()
 * and should be called to restore normal system behavior.
 *
 * @note Unblocks all signals using SIG_UNBLOCK with an empty signal set
 */
static void unblock_all_signals_in_this_thread(void) {
  sigset_t set;
  sigemptyset(&set);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

/**
 * @brief Helper function to encode hardware cache perf event configuration.
 *
 * @param cache_id  Cache level/type ID (e.g., PERF_COUNT_HW_CACHE_L1D).
 * @param op_id     Operation type ID (e.g., OP_READ).
 * @param result_id Event result type ID (e.g., RESULT_ACCESS, RESULT_MISS).
 * @return Encoded perf event config value.
 */
static inline uint64_t perf_hw_cache_config(int cache_id, int op_id,
                                            int result_id) {
  return (cache_id) | (op_id << 8) | (result_id << 16);
}

/**
 * @brief Starts L1 data cache miss and reference counters.
 *
 * Call this function before entering your timed code section. It initializes
 * and enables performance counters for L1 data cache accesses and misses, and
 * returns a @ref cache_counter_t structure containing their associated file
 * descriptors.
 *
 * @return Initialized @ref cache_counter_t with valid file descriptors on
 * success; -1 on error.
 */
cache_counter_t start_l1_cache_miss_counter() {
  cache_counter_t counter = {-1, -1};
  struct perf_event_attr pe;

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HW_CACHE;
  pe.size = sizeof(struct perf_event_attr);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;

  // L1 data cache read accesses (PERF_COUNT_HW_CACHE_L1D | OP_READ |
  // RESULT_ACCESS)
  pe.config = perf_hw_cache_config(0x0, 0x0, 0x0);
  counter.refs_fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
  if (counter.refs_fd == -1) {
    perror("perf_event_open (L1D refs)");
    return counter;
  }

  // L1 data cache read misses (PERF_COUNT_HW_CACHE_L1D | OP_READ | RESULT_MISS)
  pe.config = perf_hw_cache_config(0x0, 0x0, 0x1);
  counter.miss_fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
  if (counter.miss_fd == -1) {
    perror("perf_event_open (L1D misses)");
    close(counter.refs_fd);
    counter.refs_fd = -1;
    return counter;
  }

  // Reset and enable both counters
  ioctl(counter.refs_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(counter.refs_fd, PERF_EVENT_IOC_ENABLE, 0);
  ioctl(counter.miss_fd, PERF_EVENT_IOC_RESET, 0);
  ioctl(counter.miss_fd, PERF_EVENT_IOC_ENABLE, 0);

  return counter;
}

/**
 * @brief Stops L1 cache counters and returns the L1 cache miss rate as a
 * percentage.
 *
 * Call this function after your timed code section has finished. This function
 * disables and reads the reference and miss counters contained in the @ref
 * cache_counter_t structure, closes them, and calculates the percentage of
 * accesses that resulted in L1 cache misses.
 *
 * @param counter Pointer to @ref cache_counter_t object whose file descriptors
 * will be used and closed.
 * @return L1 cache miss rate as a percentage (returns 0.0 if no accesses were
 * counted).
 */
double stop_l1_cache_miss_counter(cache_counter_t *counter) {
  long long misses = 0, refs = 0;

  if (counter->refs_fd != -1) {
    ioctl(counter->refs_fd, PERF_EVENT_IOC_DISABLE, 0);
    read(counter->refs_fd, &refs, sizeof(long long));
    close(counter->refs_fd);
    counter->refs_fd = -1;
  }

  if (counter->miss_fd != -1) {
    ioctl(counter->miss_fd, PERF_EVENT_IOC_DISABLE, 0);
    read(counter->miss_fd, &misses, sizeof(long long));
    close(counter->miss_fd);
    counter->miss_fd = -1;
  }

  if (refs == 0)
    return 0.0;
  return 100.0 * misses / (double)refs;
}

#endif // BENCH_H
