// do_not_optimize.ixx
module;
#include "bencher/config.hpp"
export module bencher.do_not_optimize;

import std;

namespace bencher
{
   template <class T, class... Args>
   concept not_invocable = not std::invocable<T, Args...>;

   template <class T, class... Args>
   concept invocable_void = std::invocable<T, Args...> && std::is_void_v<std::invoke_result_t<T, Args...>>;

   template <class T, class... Args>
   concept invocable_not_void = std::invocable<T, Args...> && !std::is_void_v<std::invoke_result_t<T, Args...>>;

#if defined(BENCH_MSVC)
   const volatile void* volatile global_force_escape_pointer;

   inline void use_char_pointer(const volatile char* const v)
   {
      global_force_escape_pointer = reinterpret_cast<const volatile void*>(v);
   }
#endif

#if defined(BENCH_MSVC)
#define BENCH_DO_NOT_OPTIMIZE(value)                                 \
   use_char_pointer(&reinterpret_cast<char const volatile&>(value)); \
   std::atomic_signal_fence(std::memory_order_seq_cst);
#elif defined(BENCH_CLANG)
#define BENCH_DO_NOT_OPTIMIZE(value) asm volatile("" : "+r,m"(value) : : "memory");
#else
#define BENCH_DO_NOT_OPTIMIZE(value) asm volatile("" : "+m,r"(value) : : "memory");
#endif

   export template <not_invocable T>
   BENCH_ALWAYS_INLINE void do_not_optimize(T&& value)
   {
      const auto* value_ptr = &value;
      BENCH_DO_NOT_OPTIMIZE(value_ptr);
   }

   export template <invocable_void Function, class... Args>
   BENCH_ALWAYS_INLINE void do_not_optimize(Function&& value, Args&&... args)
   {
      std::forward<Function>(value)(std::forward<Args>(args)...);
      BENCH_DO_NOT_OPTIMIZE(value);
   }

   export template <invocable_not_void Function, class... Args>
   BENCH_ALWAYS_INLINE void do_not_optimize(Function&& value, Args&&... args)
   {
      auto result_val = std::forward<Function>(value)(std::forward<Args>(args)...);
      BENCH_DO_NOT_OPTIMIZE(&result_val);
   }
}

#undef BENCH_DO_NOT_OPTIMIZE
