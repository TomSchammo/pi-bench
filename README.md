
# Pi-Bench

A small benchmarking framework for the Raspberry Pi to collect accurate function timings.


- Uses macros for low overhead timing.
- Can automatically pin processes to CPUs and set priorities for single threaded applications.
- Timing data can be collected in cycles or microseconds.
- Collects L1 cache miss rates
- Allows for tracking the CPU temperature to avoid thermal throttling.
- Allows to set baselines to calculate and print relative performance.
- Supports automatic result validation
- Can save timing data & cache performance for each run to CSV


## Installation

1. Clone the repository

Run `git clone https://github.com/TomSchammo/pi-bench`

2. Install the library

Run `cd pi-bench && make install`

## Usage

1. Define your benchmarking parameters and declare optinal ground-truth buffer

```
#define WARMUP_RUNS 0
#define TIMED_RUNS 1

static void *gt = NULL;
```

2. Define your benchmarks

The library uses the X-Macro pattern to automatically discover and collect
benchmarks.
So the `BENCHMARKS` macro needs to be defined *before* including the header

```
#define _GNU_SOURCE

#define BENCHMARKS                                                             \
  BENCHMARK_TIME_PINNED(                                                       \
      "Benchmark Name", true, false, output_buffer, BYTES_OUTPUT_BUFFER, 1,    \
      function(input, output))                                                 \
```

3. Include the utils header

```
#include <pi-bench/utils.h>
```

4. Run the benchmarks

```
RUN_BENCHMARKS();
```

5. Get the results

```
PRINT_RESULTS_INDIVIDUAL();

PRINT_RESULTS_GROUP();

SAVE("./results/");
```

6. Clean the environment

```
CLEANUP();
```

# TODO

- Add example `main.c`
