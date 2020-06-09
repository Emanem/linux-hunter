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

#include "wdisplay.h"
#include <ncurses.h>
#include <cstring>
#include <cwchar>
#include <clocale>
#include <stdexcept>

namespace {
	int PLAYER_COLORS[] = { 1, 2, 3, 4 };

	class wimpl : public vbrush::iface {
		WINDOW 	*w_;
		int	cur_row_,
			cur_col_;

		int to_ncurses(const vbrush::iface::attr a) {
			using vbrush::iface;

			switch(a) {
			case attr::BOLD:
				return A_BOLD;
			case attr::REVERSE:
				return A_REVERSE;
			case attr::DIM:
				return A_DIM;
			case attr::C_BLUE:
				return COLOR_PAIR(PLAYER_COLORS[0]);
			case attr::C_MAGENTA:
				return COLOR_PAIR(PLAYER_COLORS[1]);
			case attr::C_YELLOW:
				return COLOR_PAIR(PLAYER_COLORS[2]);
			case attr::C_GREEN:
				return COLOR_PAIR(PLAYER_COLORS[3]);
			default:
				break;
			};
			throw std::runtime_error("Invalid vbrush::iface::attr!");
		}
	public:
		wimpl() : w_(initscr()), cur_row_(0), cur_col_(0) {
			// this is needed for ncursesw to print out
			// wchar_t ...
			setlocale(LC_CTYPE, "");
			if(has_colors()) {
				use_default_colors();
				start_color();
				init_pair(PLAYER_COLORS[0], COLOR_BLUE, 16);
				init_pair(PLAYER_COLORS[1], COLOR_MAGENTA, 16);
				init_pair(PLAYER_COLORS[2], COLOR_YELLOW, 16);
				init_pair(PLAYER_COLORS[3], COLOR_GREEN, 16);
			}
		}

		~wimpl() {
			endwin();
		}

		virtual bool init(void) {
			clear();
			cur_row_ = cur_col_ = 0;
			int 	row = 0, // number of terminal rows
        			col = 0; // number of terminal columns
			getmaxyx(stdscr, row, col);      /* find the boundaries of the screeen */
			// TODO check we have enough space to display
			if(col < 64 || row < 15) {
				mvprintw(0, 0, "Need at least a screen of 64x15 (%d/%d)", col, row);
				refresh();
				return false;
			}
			return true;
		}

		virtual void draw_text(const char* t, const ssize_t len) {
			mvprintw(cur_row_, cur_col_, "%s", t);
			if(len >= 0) cur_col_ += len;
			else cur_col_ += std::strlen(t);
		}

		virtual void draw_text(const wchar_t* t, const ssize_t len) {
			mvaddwstr(cur_row_, cur_col_, t);
			if(len >= 0) cur_col_ += len;
			else cur_col_ += std::wcslen(t);
		}

		virtual void next_row(const size_t n_rows) {
			cur_row_ += n_rows;
			cur_col_ = 0;
		}

		virtual void set_attr_on(const vbrush::iface::attr a) {
			attron(to_ncurses(a));
		}

		virtual void set_attr_off(const vbrush::iface::attr a) {
			attroff(to_ncurses(a));
		}

		virtual void display(void) {
			refresh();
		}
	};
}

vbrush::iface* wdisplay::get(void) {
	return new wimpl;
}

