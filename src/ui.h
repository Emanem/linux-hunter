#ifndef _UI_H_
#define _UI_H_

#include <string>
#include <ncurses.h>
#include "timer.h"

namespace ui {
	struct app_data {
		const char*	version;
		timer::cpu_ms	tm;
	};

	struct mhw_data {
		struct player_info {
			bool		used = false,
					left_session = false;
			std::wstring	name;
			int32_t		damage = 0;

		};

		struct monster_info {
			bool		used = false;
			const char*	name = 0;
			float		hp_total = -1.0,
					hp_current = -1.0;
		};

		std::wstring	session_id,
				host_name;
		player_info	players[4];
		monster_info	monsters[3];
	};

	enum draw_flags {
		SHOW_MONSTER_DATA = 1
	};

	class window {
		WINDOW 	*w_;
	public:
		window();

		~window();

		void draw(const size_t flags, const app_data& ad, const mhw_data& d);
	};
}

#endif // _UI_H_

