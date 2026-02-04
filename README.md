# bencher

[![CI](https://github.com/stephenberry/bencher/actions/workflows/ci.yml/badge.svg)](https://github.com/stephenberry/bencher/actions/workflows/ci.yml)

A modern, header-only C++23 benchmarking library with hardware performance counter support and automatic statistical analysis.

## Features

- **Header-only**: Single include, no linking required
- **Hardware Performance Counters**: CPU cycles, instructions, branches, and branch misses on supported platforms
- **Automatic Statistical Analysis**: Confidence interval-based iteration count with median-based metrics
- **Cache-Aware Benchmarking**: L1 cache eviction between runs for consistent cold-cache measurements
- **Multiple Output Formats**: Console output, Markdown reports, SVG bar charts, and JSON (optional)
- **Cross-Platform**: Windows, Linux, and macOS (including Apple Silicon)
- **Compiler Support**: GCC, Clang, and MSVC

## Requirements

- C++23 compatible compiler
- CMake 3.24+ (for building)

## Installation

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    bencher
    GIT_REPOSITORY https://github.com/stephenberry/bencher.git
    GIT_TAG main
)
FetchContent_MakeAvailable(bencher)

target_link_libraries(your_target PRIVATE bencher::bencher)
```

### Manual Integration

Copy the `include/bencher` directory to your project and add it to your include path.

## Quick Start

```cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"

int main()
{
   bencher::stage stage{"My Benchmarks"};

   stage.run("Algorithm A", [] {
      // Your code to benchmark
      // ...
      return bytes_processed; // Return the number of bytes processed
   });

   stage.run("Algorithm B", [] {
      // Another implementation
      // ...
      return bytes_processed;
   });

   bencher::print_results(stage);

   return 0;
}
```

## API Reference

### `bencher::stage`

The main benchmarking container that holds configuration and results.

```cpp
struct stage {
   std::string name{};                           // Stage name for reports
   uint64_t min_execution_count = 30;            // Minimum iterations before checking CI
   uint64_t max_execution_count = 1000;          // Maximum iterations
   double confidence_interval_threshold = 2.0;   // Target CI threshold (±%)
   std::string baseline{};                       // Baseline for comparison (empty = slowest)
   double throughput_units_divisor = 1024 * 1024; // Units divisor (default MB/s)
   std::string throughput_units_label = "MB/s";  // Label for throughput unit
   std::string processed_units_label = "Bytes";  // Label for processed units

   // Run a benchmark (function can return uint64_t bytes or void)
   template <class Function, class... Args>
   const performance_metrics& run(std::string_view subject_name,
                                  Function&& function,
                                  Args&&... args);

   // Parameterized benchmarks
   template <class Function, class T>
   void run_with(std::string_view base_name,
                 Function&& function,
                 std::initializer_list<T> params);

   // Benchmark with per-iteration setup (setup is not timed)
   template <class Setup, class Function>
   const performance_metrics& run_with_setup(std::string_view subject_name,
                                             Setup&& setup,
                                             Function&& function);
};
```

### Return Value Convention

Benchmark functions can return the number of bytes processed to enable throughput calculations:

```cpp
stage.run("Parse JSON", [&] {
   parse(buffer);
   return buffer.size(); // Return bytes processed for throughput calculation
});
```

**Void functions** are also supported for latency-focused benchmarks where throughput is not meaningful:

```cpp
stage.run("Compute", [] {
   expensive_calculation();
   // No return needed - throughput will be 0, but timing metrics are still collected
});
```

This is useful when:
- You only care about execution time, not throughput
- The concept of "bytes processed" doesn't apply to your workload
- You're measuring latency rather than data processing speed

### Custom Units (ops/s, items/s, etc.)

You can change the throughput units and labels for non-byte workloads:

```cpp
bencher::stage stage{"Zero-Copy Read"};
stage.throughput_units_divisor = 1.0;
stage.throughput_units_label = "ops/s";
stage.processed_units_label = "Ops";

stage.run("My Format", [&] {
   // ...
   return ops_processed; // return number of ops
});
```

### Parameterized Benchmarks

Test the same algorithm with different input sizes using `run_with`:

```cpp
stage.run_with("sort", [](size_t n) {
   std::vector<int> data(n);
   std::iota(data.begin(), data.end(), 0);
   std::sort(data.begin(), data.end());
   bencher::do_not_optimize(data);
   return n * sizeof(int);  // Return bytes processed
}, {1000, 10000, 100000, 1000000});
```

Results are stored with names like `"sort/1000"`, `"sort/10000"`, etc.

You can also use any range type:

```cpp
std::vector<size_t> sizes = {1000, 10000, 100000};
stage.run_with("algorithm", my_func, sizes);
```

### Benchmarks with Per-Iteration Setup

Use `run_with_setup` when your benchmark mutates its input and you need fresh state for each iteration:

```cpp
stage.run_with_setup("sort",
   [] {
      // Setup function (called before each iteration, NOT timed)
      return generate_random_data(10000);
   },
   [](auto& data) {
      // Benchmark function (timed)
      std::sort(data.begin(), data.end());
      return data.size() * sizeof(int);
   }
);
```

This is essential for accurate measurements when:
- **Sorting algorithms**: The data must be unsorted for each iteration
- **In-place algorithms**: The input gets modified and can't be reused
- **Consuming data structures**: Queues, stacks, or other structures that get emptied

Without `run_with_setup`, if you sort data in the first iteration, subsequent iterations would measure sorting already-sorted data—a completely different performance profile.

For benchmarks where the input is not modified (read-only operations like searching or hashing), regular `run()` with setup before the call is sufficient:

```cpp
auto data = load_data();  // Setup once
stage.run("search", [&] {
   auto result = binary_search(data, target);  // Data not modified
   bencher::do_not_optimize(result);
   return data.size() * sizeof(int);
});
```

### `bencher::do_not_optimize`

Prevents the compiler from optimizing away benchmark code:

```cpp
stage.run("Compute", [] {
   double result = expensive_computation();
   bencher::do_not_optimize(result); // Prevent optimization
   return sizeof(result);
});
```

Works with:
- Values: `bencher::do_not_optimize(value)`
- Functions returning void: `bencher::do_not_optimize(func, args...)`
- Functions returning values: `bencher::do_not_optimize(func, args...)`

### Output Functions

```cpp
// Print results to console with comparison
bencher::print_results(stage);
bencher::print_results(stage, false); // Without comparison

// Generate Markdown report
std::string markdown = bencher::to_markdown(stage);

// Generate SVG bar chart
std::string svg = bencher::bar_chart(stage);

// Generate JSON (requires BENCHER_ENABLE_JSON, see below)
#include "bencher/json.hpp"
std::string json = bencher::to_json(stage);
std::string json_pretty = bencher::to_json_pretty(stage);

// Save to file
bencher::save_file(markdown, "results.md");
bencher::save_file(svg, "results.svg");
```

## Performance Metrics

The library collects the following metrics (platform-dependent):

| Metric | Description |
|--------|-------------|
| Throughput (MB/s) | Processing speed in megabytes per second |
| Throughput MAD (±%) | Median Absolute Deviation of throughput |
| Instructions per Execution | Total CPU instructions per benchmark run |
| Instructions per Cycle (IPC) | CPU efficiency metric |
| Instructions per Byte | Instructions executed per byte processed |
| Cycles per Execution | CPU cycles per benchmark run |
| Cycles per Byte | CPU cycles per byte processed |
| Branches per Execution | Branch instructions per run |
| Branch Misses per Execution | Mispredicted branches per run |
| Frequency (GHz) | Estimated CPU frequency |
| Total Iterations | Number of iterations performed |

### Statistical Approach

- Uses **median** values for all metrics (robust against outliers)
- Computes **95% confidence interval** on throughput
- Automatically stops when CI reaches target threshold
- Reports **Median Absolute Deviation (MAD)** for variability

### Understanding the Metrics

These guidelines help interpret benchmark results. Values are approximate and vary by CPU architecture and workload.

| Metric | Excellent | Typical | Poor | What It Means |
|--------|-----------|---------|------|---------------|
| **IPC** | 3-4+ | 1.5-3 | < 1 | Low IPC suggests memory stalls, cache misses, or branch mispredictions |
| **Branch Misses** | < 1% | 1-3% | > 5% | High miss rate indicates unpredictable branches (consider branchless code) |
| **Throughput MAD** | < 1% | 1-3% | > 5% | High variance suggests inconsistent performance or system noise |
| **Cycles per Byte** | < 1 | 1-10 | > 50 | Workload-dependent; lower is better for throughput-oriented code |

**Important caveats:**

- **Compare relative, not absolute**: Use these metrics to compare implementations against each other, not against universal standards
- **Workload matters**: Compute-bound code behaves differently than memory-bound code
- **CPU architecture varies**: ARM and x86 have different characteristics; even generations within a family differ
- **Context is key**: A "poor" IPC might be unavoidable for memory-intensive workloads

**When to investigate:**

- IPC < 1 with high cycles → likely memory-bound; check cache efficiency
- High branch misses → consider lookup tables or branchless algorithms
- High throughput MAD → check for background processes or thermal throttling

## Platform Support

### Hardware Performance Counters

| Platform | Architecture | Counters Available |
|----------|--------------|-------------------|
| macOS | ARM64 (Apple Silicon) | Cycles, Instructions, Branches, Branch Misses |
| macOS | x86_64 | Cycles, Instructions, Branches, Branch Misses |
| Linux | x86_64 | Cycles, Instructions, Branches, Branch Misses |
| Linux | ARM64 | Cycles, Instructions, Branches, Branch Misses |
| Windows | x86_64 | Cycles (via RDTSC) |

> **Note**: On macOS, full performance counter access requires root privileges. Without root, timing measurements are still available.

> **Note**: On Windows, only cycle counting (via RDTSC) is available. Unlike Linux and macOS, Windows lacks a user-space API for accessing CPU performance counters without kernel drivers. Throughput and timing measurements work correctly. For full metrics (instructions, branches, branch misses), use external profilers like [Intel VTune](https://www.intel.com/content/www/us/en/developer/tools/oneapi/vtune-profiler.html) or [AMD uProf](https://www.amd.com/en/developer/uprof.html) alongside this library.

### Cache Management

The library automatically:
- Evicts L1 data cache between runs for cold-cache benchmarks
- Performs a 1-second warmup to stabilize CPU frequency
- Detects L1 cache size on each platform

### Thread Affinity (Linux)

To reduce variance from scheduler migrations, pin your benchmark to a single CPU core using `taskset`:

```bash
taskset -c 0 ./my_benchmark
```

This is optional—the library's statistical approach (confidence intervals, multiple iterations) already handles most scheduler-induced variance.

## SVG Chart Configuration

Customize bar chart output with `chart_config`:

```cpp
chart_config cfg;
cfg.chart_width = 1000;
cfg.chart_height = 600;
cfg.y_axis_label = "MB/s";
cfg.title = "Performance Comparison";
cfg.colors = themes::bright; // or themes::dark

std::string svg = generate_bar_chart_svg(names, values, cfg);
```

Available themes:
- `themes::bright` - Vibrant colors for light backgrounds
- `themes::dark` - Deep colors for dark backgrounds

## JSON Output (Optional)

JSON output is useful for CI integration and programmatic analysis. It requires [Glaze](https://github.com/stephenberry/glaze), which can be enabled via CMake:

```cmake
# Enable JSON support when fetching bencher
FetchContent_Declare(
    bencher
    GIT_REPOSITORY https://github.com/stephenberry/bencher.git
    GIT_TAG main
)
set(BENCHER_ENABLE_JSON ON)
FetchContent_MakeAvailable(bencher)
```

Then use the JSON functions:

```cpp
#include "bencher/json.hpp"

std::string json = bencher::to_json(stage);
std::string json_pretty = bencher::to_json_pretty(stage);

bencher::save_file(json, "results.json");
```

Example output:

```json
{
  "name": "JSON benchmarks",
  "results": [
    {
      "name": "JSON Read",
      "throughput_mb_per_sec": 1847.3,
      "time_in_ns": 856420,
      "bytes_processed": 722,
      "instructions_per_execution": 45023,
      "instructions_per_cycle": 2.31,
      "total_iteration_count": 47
    }
  ]
}
```

> **Note**: Fields with no value (e.g., hardware counters unavailable on the platform) are automatically excluded from the JSON output.

## Example Output

### Console Output

```
Performance Metrics for: JSON benchmarks
----------------------------------------------------
 - JSON Read -
Bytes Processed                         :        722
Throughput (MB/s)                        :       1847
Throughput MAD (±%)                      :       1.2
Instructions per Execution              :      45023
Instructions per Cycle                  :       2.31
Cycles per Execution                    :      19487
Frequency (GHz)                         :       3.21
Total Iterations                        :         47
====================================================
```

### Comparison Output

```
JSON Write is 23.5% faster than JSON Read
====================================================

████████████████████████████████████████│ JSON Write (2284)
████████████████████████████████▍       │ JSON Read (1847)
```

## Complete Example

```cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"
#include "bencher/bar_chart.hpp"
#include "bencher/file.hpp"

#include <vector>
#include <algorithm>

int main()
{
   std::vector<int> data(1'000'000);
   std::iota(data.begin(), data.end(), 0);

   bencher::stage stage{"Sorting Algorithms"};

   stage.run("std::sort", [&] {
      auto copy = data;
      std::sort(copy.begin(), copy.end());
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   stage.run("std::stable_sort", [&] {
      auto copy = data;
      std::stable_sort(copy.begin(), copy.end());
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   // Console output
   bencher::print_results(stage);

   // Save reports
   bencher::save_file(bencher::to_markdown(stage), "sorting_results.md");
   bencher::save_file(bencher::bar_chart(stage), "sorting_results.svg");

   return 0;
}
```

## Real-World Examples

### Comparing Sorting Algorithms

Compare multiple sorting implementations against a baseline:

```cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"

#include <algorithm>
#include <random>
#include <vector>

int main()
{
   // Generate random data
   std::vector<int> data(100'000);
   std::mt19937 rng(42);
   std::ranges::generate(data, rng);

   bencher::stage stage{"Sorting Algorithms"};
   stage.baseline = "std::sort";  // Compare all results to std::sort

   stage.run("std::sort", [&] {
      auto copy = data;
      std::sort(copy.begin(), copy.end());
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   stage.run("std::stable_sort", [&] {
      auto copy = data;
      std::stable_sort(copy.begin(), copy.end());
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   stage.run("std::ranges::sort", [&] {
      auto copy = data;
      std::ranges::sort(copy);
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   stage.run("partial_sort (10%)", [&] {
      auto copy = data;
      std::partial_sort(copy.begin(), copy.begin() + copy.size() / 10, copy.end());
      bencher::do_not_optimize(copy);
      return copy.size() * sizeof(int);
   });

   bencher::print_results(stage);
   return 0;
}
```

### Testing Serialization Performance

Benchmark different serialization approaches:

```cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"

#include <sstream>
#include <vector>

struct Record {
   int id;
   double value;
   std::string name;
};

int main()
{
   // Create test data
   std::vector<Record> records(10'000);
   for (int i = 0; i < 10'000; ++i) {
      records[i] = {i, i * 1.5, "item_" + std::to_string(i)};
   }

   bencher::stage stage{"Serialization"};

   stage.run("Binary Write", [&] {
      std::vector<char> buffer;
      buffer.reserve(records.size() * sizeof(Record));
      for (const auto& r : records) {
         auto* ptr = reinterpret_cast<const char*>(&r.id);
         buffer.insert(buffer.end(), ptr, ptr + sizeof(r.id));
         ptr = reinterpret_cast<const char*>(&r.value);
         buffer.insert(buffer.end(), ptr, ptr + sizeof(r.value));
      }
      bencher::do_not_optimize(buffer);
      return buffer.size();
   });

   stage.run("Stringstream", [&] {
      std::ostringstream oss;
      for (const auto& r : records) {
         oss << r.id << ',' << r.value << ',' << r.name << '\n';
      }
      auto str = oss.str();
      bencher::do_not_optimize(str);
      return str.size();
   });

   stage.run("String Append", [&] {
      std::string result;
      result.reserve(records.size() * 32);
      for (const auto& r : records) {
         result += std::to_string(r.id) + ',';
         result += std::to_string(r.value) + ',';
         result += r.name + '\n';
      }
      bencher::do_not_optimize(result);
      return result.size();
   });

   bencher::print_results(stage);
   return 0;
}
```

### Memory-Bound vs Compute-Bound

Demonstrate different performance characteristics:

```cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"

#include <cmath>
#include <numeric>
#include <vector>

int main()
{
   bencher::stage stage{"Memory vs Compute"};

   // Memory-bound: sequential access (high throughput, low IPC expected)
   stage.run("Sequential Read", [] {
      std::vector<int> data(1'000'000);
      std::iota(data.begin(), data.end(), 0);
      long sum = 0;
      for (auto x : data) sum += x;
      bencher::do_not_optimize(sum);
      return data.size() * sizeof(int);
   });

   // Memory-bound: random access (lower throughput due to cache misses)
   stage.run("Random Access", [] {
      std::vector<int> data(1'000'000);
      std::vector<size_t> indices(1'000'000);
      std::iota(data.begin(), data.end(), 0);
      std::iota(indices.begin(), indices.end(), 0);

      // Shuffle for random access pattern
      std::mt19937 rng(42);
      std::ranges::shuffle(indices, rng);

      long sum = 0;
      for (auto i : indices) sum += data[i];
      bencher::do_not_optimize(sum);
      return data.size() * sizeof(int);
   });

   // Compute-bound: heavy math (high IPC expected)
   stage.run("Compute Heavy", [] {
      double result = 0.0;
      for (int i = 1; i <= 100'000; ++i) {
         result += std::sin(i) * std::cos(i) * std::sqrt(i);
      }
      bencher::do_not_optimize(result);
      return 100'000 * sizeof(double);
   });

   // Branch-heavy: unpredictable branches (watch branch misses)
   stage.run("Branch Heavy", [] {
      std::vector<int> data(100'000);
      std::mt19937 rng(42);
      std::ranges::generate(data, rng);

      long sum = 0;
      for (auto x : data) {
         if (x % 2 == 0) sum += x;
         else if (x % 3 == 0) sum -= x;
         else if (x % 5 == 0) sum ^= x;
         else sum += 1;
      }
      bencher::do_not_optimize(sum);
      return data.size() * sizeof(int);
   });

   bencher::print_results(stage);
   return 0;
}
```

### CI Integration

Use JSON output for automated performance tracking in CI pipelines:

```cpp
// benchmark_ci.cpp
#include "bencher/bencher.hpp"
#include "bencher/diagnostics.hpp"
#include "bencher/json.hpp"  // Requires BENCHER_ENABLE_JSON

int main()
{
   bencher::stage stage{"CI Benchmarks"};

   stage.run("critical_path", [] {
      // Your performance-critical code
      std::vector<int> data(100'000);
      std::sort(data.begin(), data.end());
      bencher::do_not_optimize(data);
      return data.size() * sizeof(int);
   });

   // Output JSON for CI parsing
   std::cout << bencher::to_json(stage) << std::endl;

   // Optionally save artifacts
   bencher::save_file(bencher::to_json_pretty(stage), "benchmark_results.json");
   bencher::save_file(bencher::bar_chart(stage), "benchmark_results.svg");

   return 0;
}
```

Example GitHub Actions workflow:

```yaml
# .github/workflows/benchmark.yml
name: Benchmark

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Build benchmarks
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Release -DBENCHER_ENABLE_JSON=ON
          cmake --build build --target benchmark_ci

      - name: Run benchmarks
        run: |
          # Pin to CPU 0 for consistent results
          taskset -c 0 ./build/benchmark_ci > results.json

      - name: Upload results
        uses: actions/upload-artifact@v4
        with:
          name: benchmark-results
          path: |
            results.json
            benchmark_results.svg
```

For performance regression detection, compare against a baseline file:

```bash
#!/bin/bash
# scripts/check_regression.sh

THRESHOLD=10  # Alert if performance drops more than 10%

current=$(jq '.results[0].throughput_mb_per_sec' results.json)
baseline=$(jq '.results[0].throughput_mb_per_sec' baseline.json)

change=$(echo "scale=2; (($current - $baseline) / $baseline) * 100" | bc)

if (( $(echo "$change < -$THRESHOLD" | bc -l) )); then
  echo "Performance regression detected: ${change}%"
  exit 1
fi

echo "Performance change: ${change}%"
```

## Building and Testing

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --config Release
```

### Run Tests

```bash
cd build
ctest -C Release --output-on-failure
```

### Compiler Support

The library is tested on CI with:
- **GCC**: 13, 14
- **Clang**: 17, 18, 19
- **MSVC**: Latest (Windows)
- **Apple Clang**: Latest (macOS)

## License

See LICENSE file for details.
