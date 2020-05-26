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
			bool		used = false;
			std::wstring	name;
			int32_t		damage = 0;

		};

		std::wstring	session_id,
				host_name;
		player_info	players[4];
	};

	class window {
		WINDOW 	*w_;
	public:
		window();

		~window();

		void draw(const app_data& ad, const mhw_data& d);
	};
}

#endif // _UI_H_

