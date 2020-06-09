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

#ifndef _VBRUSH_H_
#define _VBRUSH_H_

#include <sys/types.h>

namespace vbrush {
	class iface {
	public:
		enum attr {
			BOLD = 0,
			REVERSE,
			DIM,
			C_BLUE,
			C_MAGENTA,
			C_YELLOW,
			C_GREEN
		};

		virtual bool init(void) = 0;
		virtual void draw_text(const char* t, const ssize_t len = -1) = 0;
		virtual void draw_text(const wchar_t* t, const ssize_t len = -1) = 0;
		virtual void next_row(const size_t n_rows = 1) = 0;
		virtual void set_attr_on(const attr a) = 0;
		virtual void set_attr_off(const attr a) = 0;
		virtual void display(void) = 0;
		virtual ~iface() {}
	};
}

#endif //_VBRUSH_H_

