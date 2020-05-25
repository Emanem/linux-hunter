#ifndef _UI_H_
#define _UI_H_

#include <string>
#include <curses.h>

namespace ui {
	struct data {
		struct player_info {
			bool		used = false;
			std::string	name;
			int32_t		damage = 0;

		};

		std::string	session_id,
				host_name;
		player_info	players[4];
	};

	class window {
		WINDOW 	*w_;
	public:
		window();

		~window();

		void draw(const char *version, const ui::data& d);
	};
}

#endif // _UI_H_

