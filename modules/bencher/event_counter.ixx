// event_counter.ixx
module;
#if !defined(_MSC_VER)
#include <dirent.h>
#endif

#include "bencher/config.hpp"
export module bencher.event_counter;

import std;

#ifdef BENCH_WIN
import bencher.counters.windows;
#elif defined(BENCH_LINUX)
import bencher.counters.linux_perf_events;
#elif defined(BENCH_MAC)
import bencher.counters.apple;
#endif

namespace bencher
{
   export struct event_count
   {
      double elapsed_ns() const noexcept { return std::chrono::duration<double, std::nano>(elapsed).count(); }

      std::optional<std::uint64_t> missed_branches{};
      std::uint64_t bytes_processed{};
      std::optional<std::uint64_t> instructions{};
      std::chrono::duration<double> elapsed{};
      std::optional<std::uint64_t> branches{};
      std::optional<std::uint64_t> cycles{};
   };

   export using event_collector = event_collector_type<event_count>;
}
