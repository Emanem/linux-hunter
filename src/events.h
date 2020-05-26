/*
    This file is part of linux-hunter.

    linux-hunter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    linux-hunter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with linux-hunter.  If not, see <https://www.gnu.org/licenses/>.
 * */

#ifndef _EVENTS_
#define _EVENTS_

#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <chrono>

// usually you want to use this class with
// fd == STDIN_FILENO

namespace events {

	class fd_proc {
		int	efd_,
			fd_;

		fd_proc(const fd_proc&) = delete;
		fd_proc& operator=(const fd_proc&) = delete;
	public:
		fd_proc(int fd) : efd_(epoll_create1(0)), fd_(fd) {
			if(-1 == efd_)
				throw std::runtime_error((std::string("Can't created epoll fd: ") + strerror(errno)).c_str());
			// add stdin
			struct epoll_event event = {0};
    			event.events = EPOLLIN|EPOLLPRI|EPOLLERR;
    			event.data.fd = fd_;
    			if(epoll_ctl(efd_, EPOLL_CTL_ADD, fd_, &event)) {
				close(efd_);
				throw std::runtime_error((std::string("Can't add fd to epoll fd: ") + strerror(errno)).c_str());
			}
		}

		// return true when need to do a refresh
		// or when time timeout expires
		bool do_io(const size_t msec_tmout) {
			struct epoll_event	event = {0};
			const auto		b_tp = std::chrono::high_resolution_clock::now();
			const int		fds = epoll_wait(efd_, &event, 1, msec_tmout);
			const auto		e_tp = std::chrono::high_resolution_clock::now();
			const size_t		msec_diff = std::chrono::duration_cast<std::chrono::milliseconds>(e_tp - b_tp).count(),
						msec_todo = (msec_diff < msec_tmout) ? (msec_tmout - msec_diff) : 0;
			if(0 == fds) return true;
			else if (0 > fds) {
				if(EINTR == errno) {
					if(msec_todo) return do_io(msec_todo);
					else return true;
				}
				throw std::runtime_error((std::string("Error in epoll_wait: ") + strerror(errno)).c_str());
			}
			// we can only get 1 event at max...
			if (event.data.fd == fd_) {
				char		buf[128];
            			// read input line
            			const int	rb = read(fd_, &buf, 128);
				if(rb > 0) {
					 const auto rv = on_data(buf, rb);
					 if(rv || !msec_todo) return true;
					 else return do_io(msec_todo);
				} else throw std::runtime_error((std::string("Error in reading fd: ") + strerror(errno)).c_str());
			}
			// we should never hit this case
			// because it would mean we have
			// received data but not on our
			// fd_ ?
			return true;
		}

		// return true if we have to perform a refresh
		virtual bool on_data(const char* p, const size_t sz) const = 0;

		virtual ~fd_proc() {
			close(efd_);
		}
	};
}

#endif //_EVENTS_

