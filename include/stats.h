
#ifndef STATS_H
#define STATS_H

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Calculate the arithmetic mean of an array.
 *
 * Generic macro that works with any arithmetic type (int, float, double, etc.)
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The arithmetic mean of the data, or 0 if size is 0
 */
#define mean(data, size)                                                       \
  ({                                                                           \
    double _sum = 0;                                                           \
    for (size_t _i = 0; _i < (size); _i++)                                     \
      _sum += (data)[_i];                                                      \
    (size) > 0 ? _sum / (size) : 0;                                            \
  })

/**
 * @brief Sort an array using selection sort algorithm.
 *
 * Generic macro that works with any comparable type.
 * This sorts the array in-place in ascending order using the selection
 * sort algorithm, which repeatedly finds the minimum element and places it at
 * the beginning.
 *
 * @param data Pointer to the array to be sorted
 * @param size Number of elements in the array
 */
#define selection_sort(data, size)                                             \
  ({                                                                           \
    for (size_t _i = 0; _i < (size); _i++) {                                   \
      size_t _smallest_idx = _i;                                               \
      for (size_t _j = _i + 1; _j < (size); _j++) {                            \
        if ((data)[_j] < (data)[_smallest_idx]) {                              \
          _smallest_idx = _j;                                                  \
        }                                                                      \
      }                                                                        \
      __typeof__(data[0]) _tmp = (data)[_i];                                   \
      (data)[_i] = (data)[_smallest_idx];                                      \
      (data)[_smallest_idx] = _tmp;                                            \
    }                                                                          \
    (void)0;                                                                   \
  })

/**
 * @brief Calculate the median value of an array.
 *
 * Generic macro that works with any arithmetic type.
 * The median is the middle value when the array is sorted. For even-sized
 * arrays, the median is the average of the two middle values.
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @param sort Macro/function to sort the array
 * @return The median value, or 0 if size is 0
 */
#define median(data, size, sort)                                               \
  ({                                                                           \
    __typeof__(data[0]) _result = 0;                                           \
    if ((size) > 0) {                                                          \
      sort((data), (size));                                                    \
      const size_t _median_idx = (size) / 2;                                   \
      if ((size) % 2 == 0) {                                                   \
        const __typeof__(data[0]) _lower = (data)[_median_idx - 1];            \
        const __typeof__(data[0]) _upper = (data)[_median_idx];                \
        _result = (_upper + _lower) / 2;                                       \
      } else {                                                                 \
        _result = (data)[_median_idx];                                         \
      }                                                                        \
    }                                                                          \
    _result;                                                                   \
  })

/**
 * @brief Calculate the population standard deviation of an array.
 *
 * Generic macro that works with any arithmetic type.
 * Standard deviation measures the amount of variation or dispersion in the
 * data. This function calculates the population standard deviation (using N as
 * the divisor).
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The population standard deviation (as double), or 0.0 if size is 0
 */
#define stddev(data, size)                                                     \
  ({                                                                           \
    double _stddev_result = 0.0;                                               \
    if ((size) > 0) {                                                          \
      const double _mean_val = mean((data), (size));                           \
      double _sum_squared_diff = 0.0;                                          \
      for (size_t _i = 0; _i < (size); _i++) {                                 \
        const double _diff = (double)(data)[_i] - (double)_mean_val;           \
        _sum_squared_diff += _diff * _diff;                                    \
      }                                                                        \
      const double _variance = _sum_squared_diff / (size);                     \
      _stddev_result = sqrt(_variance);                                        \
    }                                                                          \
    _stddev_result;                                                            \
  })

/**
 * @brief Calculate the population variance of an array.
 *
 * Generic macro that works with any arithmetic type.
 * Variance measures the average of the squared differences from the mean.
 * This function calculates the population variance (using N as the divisor).
 *
 * @param data Pointer to the array of data values
 * @param size Number of elements in the array
 * @return The population variance (as double), or 0.0 if size is 0
 */
#define var(data, size)                                                        \
  ({                                                                           \
    double _var_result = 0.0;                                                  \
    if ((size) > 0) {                                                          \
      const double _mean_val = mean((data), (size));                           \
      double _sum_squared_diff = 0.0;                                          \
      for (size_t _i = 0; _i < (size); _i++) {                                 \
        const double _diff = (double)(data)[_i] - (double)_mean_val;           \
        _sum_squared_diff += _diff * _diff;                                    \
      }                                                                        \
      _var_result = _sum_squared_diff / (size);                                \
    }                                                                          \
    _var_result;                                                               \
  })

#endif // STATS_H
