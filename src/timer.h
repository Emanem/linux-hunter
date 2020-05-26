#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>
#include <stdexcept>
#include <sys/time.h>
#include <sys/resource.h>

namespace timer {

	class wall_tmr {
		std::chrono::time_point<std::chrono::high_resolution_clock>	beg_;
		size_t								*out_;

		wall_tmr(const wall_tmr&) = delete;
		wall_tmr& operator=(const wall_tmr&) = delete;
	public:
		wall_tmr(size_t* o = 0) : beg_(std::chrono::high_resolution_clock::now()), out_(o) {
		}

		size_t get(void) const {
			const auto	end = std::chrono::high_resolution_clock::now();
			return std::chrono::duration_cast<std::chrono::milliseconds>(end - beg_).count();
		}

		~wall_tmr() {
			if(out_) {
				*out_ = get();
			}
		}
	};

	struct cpu_ms {
		size_t	wall = 0,
			user = 0,
			system = 0;
	};

	class thread_tmr {
		std::chrono::time_point<std::chrono::high_resolution_clock>	beg_;
		struct rusage							ru_;
		cpu_ms								*out_;

		thread_tmr(const thread_tmr&) = delete;
		thread_tmr& operator=(const thread_tmr&) = delete;
	public:
		thread_tmr(cpu_ms *o = 0) : beg_(std::chrono::high_resolution_clock::now()), ru_({}), out_(o) {
			if(getrusage(RUSAGE_THREAD, &ru_))
				throw std::runtime_error("getrusage failed");
		}

		cpu_ms get(void) const {
			cpu_ms		rv;
			const auto	end = std::chrono::high_resolution_clock::now();
			struct rusage	re = {0};
			if(getrusage(RUSAGE_THREAD, &re))
				throw std::runtime_error("getrusage failed");

			auto fn_tv_diff = [](const struct timeval& b, const struct timeval& e) -> size_t {
				const double	btm = 1.0*b.tv_sec + 1.0e-6*b.tv_usec,
						etm = 1.0*e.tv_sec + 1.0e-6*e.tv_usec,
						dtm = etm - btm;
				return 1000*dtm;
			};

			rv.wall = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg_).count();
			rv.user = fn_tv_diff(ru_.ru_utime, re.ru_utime);
			rv.system = fn_tv_diff(ru_.ru_stime, re.ru_stime);

			return rv;
		}

		~thread_tmr() {
			if(out_) {
				*out_ = get();
			}
		}
	};
}

#endif //_TIMER_H_

