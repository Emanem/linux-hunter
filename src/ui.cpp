#include "ui.h"
#include <cstring>
#include <cctype>

namespace {
	int PLAYER_COLORS[] = { 1, 2, 3, 4 };
}

// colors list at https://stackoverflow.com/questions/47686906/ncurses-init-color-has-no-effect

ui::window::window() : w_(initscr()) {
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

ui::window::~window() {
	endwin();
}

void ui::window::draw(const size_t flags, const app_data& ad, const mhw_data& d) {
	clear();
	char		buf[256]; // local buffer for strings
	int 		row = 0, // number of terminal rows
        		col = 0; // number of terminal columns
        getmaxyx(stdscr, row, col);      /* find the boundaries of the screeen */
	// TODO check we have enough space to display
	if(col < 64 || row < 15) {
		mvprintw(0, 0, "Need at least a screen of 64x15 (%d/%d)", col, row);
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
		std::snprintf(buf, 256, "linux-hunter %s              (%4ld/%4ld/%4ld w/u/s)", ad.version, ad.tm.wall, ad.tm.user, ad.tm.system);
		mvprintw(base_row++, xoffset, "%-64s", buf);
	}
	// print main stats
	{
		int	xoffset = 0;
		mvprintw(base_row, xoffset, "SessionId:[");
		xoffset += 11;
		std::snprintf(buf, 13, "%ls", d.session_id.c_str());
		attron(A_BOLD);
		mvprintw(base_row, xoffset, "%s", buf);
		attroff(A_BOLD);
		xoffset += 12;
		mvprintw(base_row, xoffset, "] Host:[", buf);
		xoffset += 8;
		attron(A_BOLD);
		mvaddwstr(base_row, xoffset, d.host_name.c_str());
		attroff(A_BOLD);
		// use wcslen because the string may have
		// many unused \0 at the end
		xoffset += std::wcslen(d.host_name.c_str());
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
		total_damage += (d.players[i].used) ? d.players[i].damage : 0;
	// print players data
	for(size_t i = 0; i < sizeof(d.players)/sizeof(d.players[0]); ++i, ++base_row) {
		if(!d.players[i].used) {
			attron(A_DIM);
			mvprintw(base_row, 0, "%-32s%-4d                  ", "<N/A>", i);
			attroff(A_DIM);
			continue;
		}
		int		xoffset = 0;
		const auto	name_attr = (d.players[i].left_session) ? A_DIM : COLOR_PAIR(PLAYER_COLORS[i]);
		attron(name_attr);
		mvaddwstr(base_row, xoffset, (d.players[i].left_session) ? L"Left the session" : d.players[i].name.c_str());
		xoffset += 32;
		if(!d.players[i].left_session)
			attroff(name_attr);
		mvprintw(base_row, xoffset, "%-4d", i);
		xoffset += 4;
		mvprintw(base_row, xoffset, "%10d", d.players[i].damage);
		xoffset += 10;
		mvprintw(base_row, xoffset, "%8.2f", (total_damage > 0) ? 100.0*d.players[i].damage/total_damage : 0);
		xoffset += 6;
		if(d.players[i].left_session)
			attroff(name_attr);
	}
	// now just the total
	{
		attron(A_BOLD);
		mvprintw(base_row++, 0, "%-32s%-4s%10d%8s", "Total", "", total_damage, (total_damage > 0) ? "100.00" : "0.0");
		attroff(A_BOLD);
	}
	base_row++;
	// flasg to check monster data
	if(flags & draw_flags::SHOW_MONSTER_DATA) {
		// then Monsters - first header
		attron(A_REVERSE);
		mvprintw(base_row++, 0, "%-32s%-14s%-8s", "Monster Name", "HP", "%");
		attroff(A_REVERSE);
		// print the monster data
		const int	max_monsters = sizeof(d.monsters)/sizeof(d.monsters[0]);
		int		cur_monster = 0;
		while((base_row < row) && (cur_monster < max_monsters)) {
			const mhw_data::monster_info&	mi = d.monsters[cur_monster];
			if(!mi.used) {
				++cur_monster;
				continue;
			}
			if(mi.hp_current <= 0.001) attron(A_DIM);
			mvprintw(base_row++, 0, "%-32s %6d/%6d%8.2f", mi.name, (int)mi.hp_current, (int)mi.hp_total, 100.0*mi.hp_current/mi.hp_total);
			if(mi.hp_current <= 0.001) attroff(A_DIM);
			++cur_monster;
		}
	}
	refresh();
}

