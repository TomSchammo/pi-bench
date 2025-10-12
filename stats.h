
#ifndef STATS_H
#define STATS_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Calculate the arithmetic mean of an array of unsigned 64-bit integers.
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The arithmetic mean of the data, or 0 if size is 0
 */
[[nodiscard]]
static uint64_t mean(uint64_t *data, size_t size) {

  if (size == 0)
    return 0;

  uint64_t sum = 0;
  for (size_t i = 0; i < size; i++)
    sum += data[i];

  return sum / size;
}

/**
 * @brief Sort an array of unsigned 64-bit integers using selection sort
 * algorithm.
 *
 * This function sorts the array in-place in ascending order using the selection
 * sort algorithm, which repeatedly finds the minimum element and places it at
 * the beginning.
 *
 * @param data Pointer to the array to be sorted
 * @param size Number of elements in the array
 */
void insertion_sort(uint64_t *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    size_t smallest_idx = i;
    for (size_t j = i + 1; j < size; j++) {
      if (data[j] < data[smallest_idx]) {
        smallest_idx = j;
      }
    }

    uint64_t tmp = data[i];
    data[i] = data[smallest_idx];
    data[smallest_idx] = tmp;
  }
}

/**
 * @brief Calculate the median value of an array of unsigned 64-bit integers.
 *
 * The median is the middle value when the array is sorted. For even-sized
 * arrays, the median is the average of the two middle values.
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @param sort Function pointer to the sorting algorithm to use
 * @return The median value, or 0 if size is 0
 */
[[nodiscard]]
static uint64_t median(uint64_t *data, size_t size,
                       void (*sort)(uint64_t *, size_t)) {

  if (size == 0)
    return 0;

  sort(data, size);

  const size_t median_idx = size / 2;

  if (size % 2 == 0) {

    const uint64_t lower = data[median_idx];
    const uint64_t upper = data[median_idx + 1];

    return (upper + lower) / 2;
  } else {
    return data[median_idx];
  }
}

/**
 * @brief Calculate the population standard deviation of an array of unsigned
 * 64-bit integers.
 *
 * Standard deviation measures the amount of variation or dispersion in the
 * data. This function calculates the population standard deviation (using N as
 * the divisor).
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The population standard deviation, or 0.0 if size is 0
 */
[[nodiscard]]
static double stddev(uint64_t *data, size_t size) {
  if (size == 0)
    return 0.0;

  const uint64_t mean_val = mean(data, size);
  double sum_squared_diff = 0.0;

  for (size_t i = 0; i < size; i++) {
    const double diff = (double)data[i] - (double)mean_val;
    sum_squared_diff += diff * diff;
  }

  const double variance = sum_squared_diff / size;
  return sqrt(variance);
}

/**
 * @brief Calculate the population variance of an array of unsigned 64-bit
 * integers.
 *
 * Variance measures the average of the squared differences from the mean.
 * This function calculates the population variance (using N as the divisor).
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The population variance, or 0.0 if size is 0
 */
[[nodiscard]]
static double var(uint64_t *data, size_t size) {
  if (size == 0)
    return 0.0;

  const uint64_t mean_val = mean(data, size);
  double sum_squared_diff = 0.0;

  for (size_t i = 0; i < size; i++) {
    const double diff = (double)data[i] - (double)mean_val;
    sum_squared_diff += diff * diff;
  }

  return sum_squared_diff / size;
}

#endif // STATS_H
