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

#include "fdisplay.h"
#include "hashtext_fmt.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

namespace {

	std::string get_basedir(const char *fname) {
		const auto p = std::strrchr(fname, '/');
		if(!p)
			return "";
		return std::string(fname, p+1);
	}

	class fimpl : public vbrush::iface {
		const std::string	fname_,
		      			basedir_;
		std::string		tmpfile_;
		int			tmpfh_;

		void write_attr(const uint32_t attr_mask) {
			wchar_t	buf[2];
			buf[0] = L'#';
			buf[1] = (wchar_t)attr_mask;
			if(8 != write(tmpfh_, buf, 8))
					throw std::runtime_error("Can't write display file");
		}
	public:
		fimpl(const char* fname) : fname_(fname), basedir_(get_basedir(fname)), tmpfh_(-1) {
			if(fname_.empty())
				throw std::runtime_error("Empty file display name provided");
		}

		~fimpl() {
			// remove the target file and tmp, don't care about
			// results if we can remove of not
			std::remove(fname_.c_str());
			if(-1 != tmpfh_) {
				close(tmpfh_);
				std::remove(tmpfile_.c_str());
			}
		}

		virtual bool init(void) {
			// create a tmp file
			char	buf[256];
			std::strncpy(buf, (basedir_ + "XXXXXX").c_str(), 255);
			if(-1 == mkstemp(buf))
				return false;
			tmpfile_ = buf;
			tmpfh_ = open(tmpfile_.c_str(), O_CREAT|O_WRONLY, S_IRWXU|S_IRGRP|S_IROTH);
			if(-1 == tmpfh_)
				return false;
			return true;
		}

		virtual void draw_text(const char* t, const ssize_t len) {
			const size_t	sz = std::strlen(t);
			wchar_t		wbuf[sz+1];
			for(size_t i = 0; i < sz; ++i)
				wbuf[i] = t[i];
			wbuf[sz] = L'\0';
			draw_text(wbuf, len);
		}

		virtual void draw_text(const wchar_t* t, const ssize_t len) {
			size_t		total_written = 0;
			const wchar_t	*esc = std::wcschr(t, L'#');
			while(esc) {
				const size_t	sz = esc - t;
				const ssize_t	rv = write(tmpfh_, t, sz*sizeof(wchar_t));
				if(rv != (ssize_t)(sz*sizeof(wchar_t)))
					throw std::runtime_error("Can't write display file");
				// then write escaped '#'
				if(8 != write(tmpfh_, L"##", 8))
					throw std::runtime_error("Can't write display file");
				t = esc + 1;
				esc = std::wcschr(t, L'#');
				total_written += sz+1;
			}
			const size_t	sz = std::wcslen(t);
			const ssize_t	rv = write(tmpfh_, t, sz*sizeof(wchar_t));
			if(rv != (ssize_t)(sz*sizeof(wchar_t)))
				throw std::runtime_error("Can't write display file");
			total_written += sz;
			const ssize_t	leftover = (len > 0) ? len - (ssize_t)total_written : 0;
			if(leftover > 0) {
				// we have some leftover empty chars
				const static wchar_t	*W_SPACE = L" ";
				for(ssize_t i = 0; i < leftover; ++i)
					if(sizeof(wchar_t) != write(tmpfh_, W_SPACE, sizeof(wchar_t)))
						throw std::runtime_error("Can't write display file");
			}
		}

		virtual void next_row(const size_t n_rows) {
			const static wchar_t	*W_NEWLINE = L"\n";
			for(size_t i = 0; i < n_rows; ++i)
				if(sizeof(wchar_t) != write(tmpfh_, W_NEWLINE, sizeof(wchar_t)))
					throw std::runtime_error("Can't write display file");
		}

		virtual void set_attr_on(const vbrush::iface::attr a) {
			switch(a) {
			case BOLD: {
				write_attr(ht_fmt::BOLD_ON);
			} break;
			case REVERSE: {
				write_attr(ht_fmt::REVERSE_ON);
			} break;
			case DIM: {
				write_attr(ht_fmt::DIM_ON);
			} break;
			case C_BLUE: {
				write_attr(ht_fmt::BLUE_ON);
			} break;
			case C_MAGENTA: {
				write_attr(ht_fmt::MAGENTA_ON);
			} break;
			case C_YELLOW: {
				write_attr(ht_fmt::YELLOW_ON);
			} break;
			case C_GREEN: {
				write_attr(ht_fmt::GREEN_ON);
			} break;
			default:
				break;
			}
		}

		virtual void set_attr_off(const vbrush::iface::attr a) {
			switch(a) {
			case BOLD: {
				write_attr(ht_fmt::BOLD_OFF);
			} break;
			case REVERSE: {
				write_attr(ht_fmt::REVERSE_OFF);
			} break;
			case DIM: {
				write_attr(ht_fmt::DIM_OFF);
			} break;
			case C_BLUE: {
				write_attr(ht_fmt::BLUE_OFF);
			} break;
			case C_MAGENTA: {
				write_attr(ht_fmt::MAGENTA_OFF);
			} break;
			case C_YELLOW: {
				write_attr(ht_fmt::YELLOW_OFF);
			} break;
			case C_GREEN: {
				write_attr(ht_fmt::GREEN_OFF);
			} break;

			default:
				break;
			}
		}

		virtual void display(void) {
			if(-1 == tmpfh_)
				throw std::runtime_error("Can't display unintialized file");
			close(tmpfh_);
			tmpfh_ = -1;
			if(std::rename(tmpfile_.c_str(), fname_.c_str()))
				throw std::runtime_error("Can't swap display file");
			if(chmod(fname_.c_str(), S_IRWXU|S_IRGRP|S_IROTH))
				throw std::runtime_error("Can't change permissions of display file");
		}
	};
}

vbrush::iface* fdisplay::get(const char *fname) {
	return new fimpl(fname);
}

