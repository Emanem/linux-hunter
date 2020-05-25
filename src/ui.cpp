#include "ui.h"

namespace {
	int PLAYER_COLORS[] = { 1, 2, 3, 4 };
}

// colors list at https://stackoverflow.com/questions/47686906/ncurses-init-color-has-no-effect

ui::window::window() : w_(initscr()) {
	if(has_colors()) {
		use_default_colors();
		start_color();
		init_pair(PLAYER_COLORS[0], COLOR_BLUE, 16);
		init_pair(PLAYER_COLORS[1], COLOR_MAGENTA, 16);
		init_pair(PLAYER_COLORS[2], COLOR_YELLOW, 16);
		init_pair(PLAYER_COLORS[3], COLOR_GREEN, 16);
	}
}

ui::window::~window() {
	endwin();
}

void ui::window::draw(const char* version, const ui::data& d) {
	clear();
	char		buf[256]; // local buffer for strings
	int 		row = 0, // number of terminal rows
        		col = 0; // number of terminal columns
        getmaxyx(stdscr, row, col);      /* find the boundaries of the screeen */
	// TODO check we have enough space to display
	if(col < 64 || row < 10) {
		mvprintw(0, 0, "Need at least a screen of 64x10 (%d/%d)", col, row);
		refresh();
		return;
	}
	/*
	24                      
	XXXXXXXXXXXXXXXXXXXXXXXX
	linux-hunter 0.0.6      

	24                      7      32                                      1 = 64
	OOOOOOOOOOOOOOOOOOOOOOOOHHHHHHHEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEH
	SessionId:[Camw+P?+dNLP] Host:[EmettaXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX]

	Player Name 32                          Id 4Damage 10 % 6
	EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEIIIIDDDDDDDDDDPPPPPP
	 */ 
	// print title
	int	base_row = 0;
	{
		int	xoffset = 0;
		char	buf[256];
		std::snprintf(buf, 24, "linux-hunter %s", version);
		mvprintw(base_row++, xoffset, "%-24s", buf);
	}
	// print main stats
	{
		int	xoffset = 0;
		mvprintw(base_row, xoffset, "SessionId:[");
		xoffset += 11;
		std::snprintf(buf, 13, "%s", d.session_id.c_str());
		attron(A_BOLD);
		mvprintw(base_row, xoffset, "%s", buf);
		attroff(A_BOLD);
		xoffset += 12;
		mvprintw(base_row, xoffset, "] Host:[", buf);
		xoffset += 8;
		const auto host_len = std::snprintf(buf, 33, "%s", d.host_name.c_str());
		attron(A_BOLD);
		mvprintw(base_row, xoffset, "%s", buf);
		attroff(A_BOLD);
		xoffset += host_len;
		mvprintw(base_row, xoffset, "]");
		base_row += 2;
	}
	// print header
	{
		attron(A_REVERSE);
		mvprintw(base_row++, 0, "%-32s%-4s%-10s%-8s", "Player Name", "Id", "Damage", "%");
		attroff(A_REVERSE);
	}
	// compute total damage
	int	total_damage = 0;
	for(size_t i = 0; i < sizeof(d.players)/sizeof(d.players[0]); ++i)
		total_damage += d.players[i].damage;
	// print players data
	for(size_t i = 0; i < sizeof(d.players)/sizeof(d.players[0]); ++i, ++base_row) {
		int	xoffset = 0;
		attron(COLOR_PAIR(PLAYER_COLORS[i]));
		mvprintw(base_row, xoffset, "%-32s", d.players[i].name.c_str());
		xoffset += 32;
		attroff(COLOR_PAIR(PLAYER_COLORS[i]));
		mvprintw(base_row, xoffset, "%-4d", i);
		xoffset += 4;
		mvprintw(base_row, xoffset, "%10d", d.players[i].damage);
		xoffset += 10;
		mvprintw(base_row, xoffset, "%8.2f", 100.0*d.players[i].damage/total_damage);
		xoffset += 6;
	}
	// now just the total
	{
		attron(A_BOLD);
		mvprintw(base_row++, 0, "%-32s%-4s%10d%8s", "Total", "", total_damage, "100.00");
		attroff(A_BOLD);
	}
	refresh();
}

