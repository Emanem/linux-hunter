#include "ui.h"

extern void ui::draw(vbrush::iface* b, const size_t flags, const app_data& ad, const mhw_data& d, const bool no_color) {
	char		buf[256]; // local buffer for strings
	if(!b->init())
		return;
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
	{
		std::snprintf(buf, 256, "linux-hunter %-19s(%4ld/%4ld/%4ld w/u/s)", ad.version, ad.tm.wall, ad.tm.user, ad.tm.system);
		b->draw_text(buf);
		b->next_row();
	}
	// print main stats
	{
		b->draw_text("SessionId:[");
		b->set_attr_on(vbrush::iface::attr::BOLD);
		b->draw_text(d.session_id.c_str());
		b->set_attr_off(vbrush::iface::attr::BOLD);
		b->draw_text("] Host:[");
		b->set_attr_on(vbrush::iface::attr::BOLD);
		b->draw_text(d.host_name.c_str());
		b->set_attr_off(vbrush::iface::attr::BOLD);
		b->draw_text("]");
		b->next_row(2);
	}
	// print header
	{
		std::snprintf(buf, 256, "%-39s%-4s%-10s%-8s", "Player Name", "Id", "Damage", "%");
		b->set_attr_on(vbrush::iface::attr::REVERSE);
		b->draw_text(buf);
		b->set_attr_off(vbrush::iface::attr::REVERSE);
		b->next_row();
	}
	// compute total damage
	int	total_damage = 0;
	for(size_t i = 0; i < sizeof(d.players)/sizeof(d.players[0]); ++i)
		total_damage += (d.players[i].used) ? d.players[i].damage : 0;
	// print players data
	static const vbrush::iface::attr v_colors[] = { vbrush::iface::attr::C_BLUE, vbrush::iface::attr::C_MAGENTA, vbrush::iface::attr::C_YELLOW, vbrush::iface::attr::C_GREEN };
	for(size_t i = 0; i < sizeof(d.players)/sizeof(d.players[0]); ++i, b->next_row()) {
		if(!d.players[i].used) {
			std::snprintf(buf, 256, "%-39s%-4d                  ", "<N/A>", (int)i);
			b->set_attr_on(vbrush::iface::attr::DIM);
			b->draw_text(buf);
			b->set_attr_off(vbrush::iface::attr::DIM);
			continue;
		}
		const auto	name_attr = (d.players[i].left_session) ? vbrush::iface::attr::DIM : v_colors[i];
		// set attribute when no_color is false
		// OR the player has left the session
		if(!no_color || d.players[i].left_session)
			b->set_attr_on(name_attr);
		b->draw_text((d.players[i].left_session) ? L"Left the session" : d.players[i].name.c_str(), 39);
		// only remove the attribute here
		// when no_color has not been set
		if(!no_color && !d.players[i].left_session)
			b->set_attr_off(name_attr);
		std::snprintf(buf, 256, "%-4d", (int)i);
		b->draw_text(buf);
		std::snprintf(buf, 256, "%10d", d.players[i].damage);
		b->draw_text(buf);
		std::snprintf(buf, 256, "%8.2f", (total_damage > 0) ? 100.0*d.players[i].damage/total_damage : 0);
		b->draw_text(buf);
		if(d.players[i].left_session)
			b->set_attr_off(name_attr);
	}
	// now just the total
	{
		std::snprintf(buf, 256, "%-39s%-4s%10d%8s", "Total", "", total_damage, (total_damage > 0) ? "100.00" : "0.0");
		b->set_attr_on(vbrush::iface::attr::BOLD);
		b->draw_text(buf);
		b->set_attr_off(vbrush::iface::attr::BOLD);
	}
	// flasg to check monster data
	if(flags & draw_flags::SHOW_MONSTER_DATA) {
		b->next_row(2);
		// then Monsters - first header
		if(flags & draw_flags::SHOW_CROWN_DATA)
			std::snprintf(buf, 256, "%-32s%-14s%-8s %-6s", "Monster Name", "HP", "%","Crown");
		else
			std::snprintf(buf, 256, "%-39s%-14s%-8s", "Monster Name", "HP", "%");
		b->set_attr_on(vbrush::iface::attr::REVERSE);
		b->draw_text(buf);
		b->set_attr_off(vbrush::iface::attr::REVERSE);
		// print the monster data
		const int	max_monsters = sizeof(d.monsters)/sizeof(d.monsters[0]);
		int		cur_monster = 0;
		while(cur_monster < max_monsters) {
			const mhw_data::monster_info&	mi = d.monsters[cur_monster];
			if(!mi.used) {
				++cur_monster;
				continue;
			}
			b->next_row();
			if(mi.hp_current <= 0.001) b->set_attr_on(vbrush::iface::attr::DIM);
			if(flags & draw_flags::SHOW_CROWN_DATA)
				std::snprintf(buf, 256, "%-32s %6d/%6d%8.2f %-6s", mi.name, (int)mi.hp_current, (int)mi.hp_total, 100.0*mi.hp_current/mi.hp_total, mi.crown);
			else
				std::snprintf(buf, 256, "%-39s %6d/%6d%8.2f", mi.name, (int)mi.hp_current, (int)mi.hp_total, 100.0*mi.hp_current/mi.hp_total);
			b->draw_text(buf);
			if(mi.hp_current <= 0.001) b->set_attr_off(vbrush::iface::attr::DIM);
			++cur_monster;
		}
	}
	b->display();
}

