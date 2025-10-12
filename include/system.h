#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Reads the current value of the ARM64 virtual counter register.
 *
 * Returns the current value of the CNTVCT_EL0 register, which provides
 * a high-resolution timer for precise performance measurements. The counter
 * is synchronized across all CPU cores and provides nanosecond-level precision.
 *
 * @return Current virtual counter value (64-bit unsigned integer)
 *
 * @note ARM64 architecture specific (uses CNTVCT_EL0 register)
 * @note Includes memory barriers (ISB) for accurate timing
 * @note Always inlined for minimal overhead
 */
[[nodiscard]] static __attribute__((always_inline)) uint64_t get_cycles(void) {
  uint64_t val;
  __asm__("isb" ::: "memory");
  __asm__("mrs %0, cntvct_el0" : "=r"(val));
  __asm__("isb" ::: "memory");
  return val;
}

static inline void system_wait() {
  for (volatile unsigned int i = 0; i < 1 << 15; i++) {
    __asm__("nop");
  }
}

/**
 * @brief Retrieves the current CPU temperature from the thermal zone.
 *
 * Reads the CPU temperature from the Linux thermal zone interface.
 * The temperature is read from /sys/class/thermal/thermal_zone0/temp
 * and converted from millicelsius to degrees Celsius.
 *
 * @return The current CPU temperature in degrees Celsius, or -1.0f on error
 *
 * @note Requires Linux system with thermal zone support
 * @note Returns -1.0f if the thermal zone file cannot be read
 * @note Temperature is typically in the range 30-100°C for normal operation
 */
[[nodiscard]] static inline float get_cpu_temperature(void) {
  FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
  if (!f)
    return -1.0f;

  int temp_millicelsius;
  if (fscanf(f, "%d", &temp_millicelsius) != 1) {
    fclose(f);
    return -1.0f;
  }
  fclose(f);
  return temp_millicelsius / 1000.0f;
}

/**
 * @brief Retrieves the current CPU frequency of a specified core.
 *
 * Reads the current CPU frequency from the cpufreq scaling interface.
 * The frequency is read from
 * /sys/devices/system/cpu/cpu{N}/cpufreq/scaling_cur_freq and converted from
 * kHz to MHz.
 *
 * @param cpu The core number whose frequency is to be checked (0-based
 * indexing)
 * @return The CPU frequency of the specified core in MHz, or 0 on error
 *
 * @note Requires Linux system with cpufreq support
 * @note Returns 0 if the cpufreq file cannot be read
 * @note Core numbering typically starts from 0
 * @note Frequency may vary based on CPU governor and load
 */
[[nodiscard]] static inline uint64_t get_cpu_frequency(int cpu) {
  char path[256];
  snprintf(path, sizeof(path),
           "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);

  FILE *f = fopen(path, "r");
  if (!f)
    return 0;

  uint64_t freq;
  if (fscanf(f, "%llu", &freq) != 1) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return freq / 1000;
}

/**
 * @brief Retrieves the current system load average (1-minute).
 *
 * Reads the 1-minute load average from /proc/loadavg, which represents
 * the average system load over the past minute. Load average indicates
 * the number of processes that are either running or waiting for CPU time.
 *
 * @return The current 1-minute load average, or 0.0f on error
 *
 * @note Requires Linux system with /proc filesystem
 * @note Returns 0.0f if the loadavg file cannot be read
 * @note Load average of 1.0 means the system is fully utilized
 * @note Values above 1.0 indicate the system is overloaded
 */
[[nodiscard]] static inline float get_load_average(void) {
  FILE *f = fopen("/proc/loadavg", "r");
  if (!f)
    return 0.0;

  float load_1min;
  if (fscanf(f, "%f", &load_1min) != 1) {
    fclose(f);
    return 0.0;
  }
  fclose(f);
  return load_1min;
}

/**
 * @brief Retrieves current system memory usage in kilobytes.
 *
 * Reads memory information from /proc/meminfo and calculates the
 * amount of memory currently in use by subtracting available memory
 * from total memory. This represents the total memory consumption
 * by all processes and the kernel.
 *
 * @return The current system memory usage in kB, or 0 on error
 *
 * @note Requires Linux system with /proc filesystem
 * @note Returns 0 if the meminfo file cannot be read
 * @note This includes both user and kernel memory usage
 * @note Memory usage = MemTotal - MemAvailable
 */
[[nodiscard]] static inline uint64_t get_memory_usage(void) {
  FILE *f = fopen("/proc/meminfo", "r");
  if (!f)
    return 0;

  char line[256];
  uint64_t mem_total = 0, mem_available = 0;

  while (fgets(line, sizeof(line), f)) {
    if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1)
      continue;
    if (sscanf(line, "MemAvailable: %llu kB", &mem_available) == 1)
      break;
  }
  fclose(f);

  return mem_total - mem_available;
}

/**
 * @brief Retrieves the number of available CPU cores.
 *
 * Reads the number of online CPU cores from /proc/cpuinfo by counting
 * the number of processor entries. This provides the total number of
 * CPU cores available to the system.
 *
 * @return The number of available CPU cores, or 0 on error
 *
 * @note Requires Linux system with /proc filesystem
 * @note Returns 0 if cpuinfo file cannot be read
 * @note Counts only online/available cores, not offline cores
 * @note Useful for determining optimal thread count for parallel processing
 */
[[nodiscard]] static inline int get_cpu_cores(void) {
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (!f)
    return 0;

  char line[256];
  int core_count = 0;

  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "processor", 9) == 0) {
      core_count++;
    }
  }

  fclose(f);
  return core_count;
}

/**
 * @brief Checks CPU temperature against thermal throttling threshold.
 *
 * Compares the current CPU temperature with a specified maximum temperature
 * and displays a colored warning message. Green indicates safe temperature,
 * red indicates thermal throttling risk.
 *
 * @param max_temp The maximum safe temperature threshold in degrees Celsius
 *
 * @note Uses ANSI color codes for terminal output
 * @note Green (\033[32m) for safe temperatures
 * @note Red (\033[31m) for temperatures at or above threshold
 * @note Calls get_cpu_temperature() internally
 */
static inline void throttle_warning(float max_temp) {
  float current_temp = get_cpu_temperature();
  if (current_temp < max_temp) {
    printf("\033[32mCPU temperature is good (%f)!\033[0m\n", current_temp);
  } else {
    printf("\033[31mCPU has reached or ecceeded maximum temperature (%f > "
           "%f)!\033[0m",
           current_temp, max_temp);
  }
}

/**
 * @brief Displays comprehensive system status information.
 *
 * Gathers and displays various system metrics including CPU frequencies
 * for all available cores, CPU temperature with color-coded warnings, load
 * average, and memory usage. This function provides a complete system overview
 * for monitoring and debugging purposes.
 *
 * @note Dynamically displays CPU frequencies for all available cores (0 to N-1)
 * @note CPU temperature is color-coded: green (<70°C), yellow (70-80°C), red
 * (>80°C)
 * @note For a Raspberry Pi 3B+ the frequency is limited to 1.2GHz at 70 and
 * throtteling starts at 80
 * @note Shows 1-minute load average and memory usage in kB
 * @note Uses ANSI color codes for enhanced terminal output
 * @note Automatically detects the number of CPU cores using get_cpu_cores()
 */
static inline void get_system_status() {
  int cpu_cores = get_cpu_cores();

  for (int i = 0; i < cpu_cores; i++) {
    uint64_t cpu_freq = get_cpu_frequency(i);
    printf("CPU %d Frequency: %llu MHz\n", i, cpu_freq);
  }

  float cpu_temp = get_cpu_temperature();
  float load_avg = get_load_average();
  uint64_t memory_usage = get_memory_usage();

  if (cpu_temp < 70) {
    printf("\033[32mCPU Temperature: %f C\033[0m\n", cpu_temp);
  } else if (cpu_temp < 80) {
    printf("\033[33mCPU Temperature: %f C\033[0m\n", cpu_temp);
  } else {
    printf("\033[31mCPU Temperature: %f C\033[0m\n", cpu_temp);
  }

  printf("Load Average: %f\nMemory Usage: %llu kB\n", load_avg, memory_usage);
}

#endif // SYSTEM_H
