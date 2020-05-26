#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>

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
			const auto	end_ = std::chrono::high_resolution_clock::now();
			return std::chrono::duration_cast<std::chrono::milliseconds>(end_ - beg_).count();
		}

		~wall_tmr() {
			if(out_) {
				*out_ = get();
			}
		}
	};
}

#endif //_TIMER_H_

