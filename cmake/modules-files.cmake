target_sources(bencher_bencher
  PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS modules
    FILES
      modules/bencher/bar_chart.ixx
      modules/bencher/bencher.ixx
      modules/bencher/cache_clearer.ixx
      modules/bencher/diagnostics.ixx
      modules/bencher/do_not_optimize.ixx
      modules/bencher/event_counter.ixx
      modules/bencher/file.ixx
      modules/bencher/json.ixx
      modules/bencher/line_chart.ixx
      modules/bencher/radar_chart.ixx
)

# counters
target_sources(bencher_bencher
  PUBLIC
    FILE_SET CXX_MODULES
    FILES
      modules/bencher/counters/apple_arm_perf_events.ixx
      modules/bencher/counters/linux_perf_events.ixx
      modules/bencher/counters/windows_perf_events.ixx
)
