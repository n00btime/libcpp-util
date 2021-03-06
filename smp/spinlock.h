//============================================================================
//                                  libcpp-util
//                   A simple odds-n-ends library for C++11
//
//         Licensed under modified BSD license. See LICENSE for details.
//============================================================================

#ifndef LIBCPP_UTIL_SPINLOCK_H
#define LIBCPP_UTIL_SPINLOCK_H

#include <atomic>
#include <chrono>

// TODO: This is usable on IA32 compatible
#if __x86_64__ && __GNUC__
#define cpu_relax() asm volatile("pause\n": : :"memory")
#else
#define cpu_relax()
#endif

namespace cpputil {

class spinlock {
	std::atomic_flag flag;

	// Uncopyable
	spinlock(const spinlock&) = delete;
	spinlock& operator=(const spinlock&) = delete;
public:
	spinlock() : flag(ATOMIC_FLAG_INIT) {}
	~spinlock() = default;

	void lock() {
		while (flag.test_and_set(std::memory_order_acquire))
			cpu_relax();
	}

	bool try_lock() {
		return !flag.test_and_set(std::memory_order_acquire);
	}
	
	template <class Rep, class Period>
	bool try_lock_for(const std::chrono::duration<Rep,Period>& duration) {
		return try_lock_until(
				std::chrono::steady_clock::now() + duration);
	}

	template <class Clock, class Duration>
	bool try_lock_until(
			const std::chrono::time_point<Clock, Duration>& when) {
		while (when > Clock::now())
			if (try_lock())
				return true;
		return false; // Time elapsed
	}

	void unlock() {
		flag.clear(std::memory_order_release);
	}
};

// Useful to have in situations where cacheline contention matters.
template <unsigned N>
class padded_spinlock {
private:
	alignas(N) spinlock s;
	char _padding[N - sizeof(s)];
public:
	void lock() {
		return s.lock();
	}

	bool try_lock() {
		return s.try_lock();
	}
	
	template <class Rep, class Period>
	bool try_lock_for(const std::chrono::duration<Rep,Period>& duration) {
		return s.try_lock_for(duration);
	}

	template <class Clock, class Duration>
	bool try_lock_until(
			const std::chrono::time_point<Clock, Duration>& when) {
		return s.try_lock_until(when);
	}

	void unlock() {
		return s.unlock();
	}
};

using cacheline_spinlock = padded_spinlock<64>;

}

#undef cpu_relax // Don't pollute
#endif
