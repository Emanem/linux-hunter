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

#include <iostream>
#include <sstream>
#include <getopt.h>
#include <cstring>
#include <memory>
#include <csignal>
#include "memory.h"
#include "ui.h"
#include "wdisplay.h"
#include "fdisplay.h"
#include "events.h"
#include "timer.h"
#include "mhw_lookup.h"
#include "utils.h"

// Useful links with the SmartHunter sources; note that
// sir-wilhelm is the one up to date with most recent
// releases from CAPCOM
//
// https://github.com/r00telement/SmartHunter
// https://github.com/r00telement/SmartHunter/blob/a67a26357c2047790013be088819464f4e8ae596/SmartHunter/Core/ThreadedMemoryScan.cs#L73
// https://github.com/r00telement/SmartHunter/blob/573f2d11ec6816c593a967bc92f6e8fdb99b129a/SmartHunter/Core/Helpers/MemoryHelper.cs#L206
// https://github.com/sir-wilhelm/SmartHunter
// https://github.com/sir-wilhelm/SmartHunter/blob/master/SmartHunter/Game/Config/MemoryConfig.cs

namespace {
	template<typename T>
	void print_bin(const T& t, std::ostream& out) {
		const uint8_t	*p = (const uint8_t*)&t;
		for(size_t i = 0; i < sizeof(T); ++i) {
			char	buf[3];
			std::snprintf(buf, 3, "%02X", p[i]);
			buf[2] = '\0';
			out << buf << ' ';
		}
	}
}

namespace {
	const char*	VERSION = "0.1.0";

	// settings/options management
	pid_t		mhw_pid = -1;
	std::string	save_dir,
			load_dir,
			file_display;
	bool		show_monsters_data = false,
			debug_ptrs = false,
			debug_all = false,
			mem_dirty_opt = false,
			lazy_alloc = true,
			direct_mem = true;
	size_t		refresh_interval = 1000;

	void print_help(const char *prog, const char *version) {
		std::cerr <<	"Usage: " << prog << " [options]\nExecutes linux-hunter " << version << "\n\n"
				"-m, --show-monsters Shows HP monsters data (requires slightly more CPU usage)\n"
				"-s, --save dir      Captures the specified pid into directory 'dir' and quits\n"
				"-l, --load dir      Loads the specified capture directory 'dir' and displays\n"
				"                    info (static - useful for debugging)\n"
				"    --no-direct-mem Don't access MH:W memory directly and dynamically, use a local copy\n"
				"                    via buffers - increase CPU usage (both u and s) and the expenses\n"
				"                    of potentially slightly less inconsistencies\n"
				"-f, --f-display f   Writes the content of display on a file 'f', refreshing such file\n"
				"                    every same iteration. The content of the file is a 'wchar_t' similar\n"
				"                    to the UI, having special '#' as escape character to denote styles\n"
				"                    and formats (see sources for usage of '#' escape sequances)\n"
				"                    It is heavily suggested to have file 'f' under '/dev/shm' or '/tmp'\n"
				"                    memory backed filesystem\n"	
				"    --mhw-pid p     Specifies which pid to scan memory for (usually main MH:W)\n"
				"                    When not specified, linux-hunter will try to find it automatically\n"
				"                    This is default behaviour\n"
				"    --debug-ptrs    Prints the main AoB (Array of Bytes) pointers (useful for debugging)\n"
				"    --debug-all     Prints all the AoB (Array of Bytes) partial and full matches\n"
				"                    (useful for analysing AoB) and quits; implies setting debug-ptrs\n"
				"    --mem-dirty-opt Enable optimization to load memory pages just once per refresh;\n"
				"                    this should be slightly less accurate but uses less system time\n"
				"    --no-lazy-alloc Disable optimization to reduce memory usage and always allocates memory\n"
				"                    to copy MH:W process - minimize dynamic allocations at the expense of\n"
				"                    memory usage; decrease calls to alloc/free functions\n"
				"-r, --refresh i     Specifies what is the UI/stats refresh interval in ms (default 1000)\n"
				"    --help          prints this help and exit\n\n"
				"When linux-hunter is running:\n\n"
				"'q' or 'ESC'        Quits the application\n"
				"'r'                 Force a refresh\n"
		<< std::flush;
	}

	int parse_args(int argc, char *argv[], const char *prog, const char *version) {
		int			c;
		static struct option	long_options[] = {
			{"help",		no_argument,	   0,	0},
			{"mhw-pid",		required_argument, 0,   0},
			{"show-monsters",	no_argument,	   0,	'm'},
			{"save",		required_argument, 0,	's'},
			{"load",		required_argument, 0,	'l'},
			{"no-direct-mem",	no_argument,	   0,	0},
			{"f-display",		required_argument, 0,	'f'},
			{"debug-ptrs",		no_argument,	   0,	0},
			{"debug-all",		no_argument,	   0,	0},
			{"mem-dirty-opt",	no_argument,	   0,	0},
			{"no-lazy-alloc",	no_argument,	   0,	0},
			{"refresh",		required_argument, 0,   'r'},
			{0, 0, 0, 0}
		};

		while (1) {
			// getopt_long stores the option index here
			int		option_index = 0;

			if(-1 == (c = getopt_long(argc, argv, "s:l:r:mf:", long_options, &option_index)))
				break;

			switch (c) {
			case 0: {
				// If this option set a flag, do nothing else now
				if (long_options[option_index].flag != 0)
					break;
				if(!std::strcmp("help", long_options[option_index].name)) {
					print_help(prog, version);
					std::exit(0);
				} else if (!std::strcmp("debug-ptrs", long_options[option_index].name)) {
					debug_ptrs = true;
				} else if (!std::strcmp("debug-all", long_options[option_index].name)) {
					debug_all = debug_ptrs = true;
				} else if (!std::strcmp("mem-dirty-opt", long_options[option_index].name)) {
					mem_dirty_opt = true;
				} else if (!std::strcmp("mhw-pid", long_options[option_index].name)) {
					mhw_pid = std::atoi(optarg);
				} else if (!std::strcmp("no-lazy-alloc", long_options[option_index].name)) {
					lazy_alloc = false;
				} else if (!std::strcmp("no-direct-mem", long_options[option_index].name)) {
					direct_mem = false;
				}
			} break;

			case 'r': {
				refresh_interval = std::atoi(optarg);
				if(refresh_interval <= 0) refresh_interval = 1000;
			} break;

			case 's': {
				save_dir = optarg;
				if(!save_dir.empty() && (*save_dir.rbegin() == '/'))
					save_dir.resize(save_dir.size()-1);
			} break;

			case 'l': {
				load_dir = optarg;
				if(!load_dir.empty() && (*load_dir.rbegin() == '/'))
					load_dir.resize(load_dir.size()-1);
			} break;

			case 'f': {
				file_display = optarg;
			} break;

			case 'm': {
				show_monsters_data = true;
			} break;

			case '?':
			break;

			default:
				throw std::runtime_error((std::string("Invalid option '") + (char)c + "'").c_str());
			}
		}
		return optind;
	}
}

namespace {
	class keyb_proc : public events::fd_proc {
		bool&	run_;
	public:
		keyb_proc(bool& r) : events::fd_proc(STDIN_FILENO), run_(r) {
		}

		virtual bool on_data(const char* p, const size_t sz) const {
			for(size_t i = 0; i < sz; ++i) {
				switch(p[i]) {
				case 27: // ESC key
				case 'q':
					run_ = false;
					return true;
				case 'r':
					return true;
				default:
					break;
				}
			}

			return false;
		}
	};

	void 	(*prev_sigint_handler)(int) = 0;
	bool	run(true);

	void sigint_handler(int signal) {
		run = false;
		// reset to previous handler
		if(prev_sigint_handler)
			std::signal(SIGINT, prev_sigint_handler);
	}
}

int main(int argc, char *argv[]) {
	try {
		// initialize sigint
		prev_sigint_handler = std::signal(SIGINT, sigint_handler);
		// lookup patterns
		memory::pattern	p0(patterns::PlayerName),
				p1(patterns::CurrentPlayerName),
				p2(patterns::PlayerDamage),
				p3(patterns::Monster),
				p4(patterns::PlayerBuff),
				p5(patterns::Emetta),
				p6(patterns::PlayerNameLinux),
				p7(patterns::LobbyStatus);
		memory::pattern	*p_vec[] = { &p0, &p1, &p2, &p3, &p4 , &p5, &p6, &p7 };
		// parse args first
		const auto optind = parse_args(argc, argv, argv[0], VERSION);
		// check come consistency
		if(!load_dir.empty() && !save_dir.empty())
			throw std::runtime_error("Can't specify both 'load' and 'save' options");
		// if we aren't in load mode and mhw pid is -1
		// try to find it automatically
		if(-1 == mhw_pid && load_dir.empty()) {
			mhw_pid = utils::find_mhw_pid();
			std::cerr << "Found pid: " << mhw_pid << std::endl;
		}
		// start here...
		memory::browser	mb(mhw_pid, mem_dirty_opt, lazy_alloc, direct_mem);
		// if we're in load mode fill b
		// with content from the disk
		if(!load_dir.empty()) {
			if(direct_mem)
				throw std::runtime_error("The option -d,--direct-mem is incompatile with -l,--load");
			std::cerr << "Loading memory content from directory '" << load_dir << "'..." << std::endl;
			mb.load(load_dir.c_str());
			std::cerr << "done" << std::endl;
		} else {
			mb.snap();
			// if in save mode, save and exit
			if(!save_dir.empty()) {
				std::cerr << "Saving memory content to directory '" << save_dir << "'..." << std::endl;
				mb.store(save_dir.c_str());
				std::cerr << "done" << std::endl;
				return 0;
			}
		}
		// print out basic patterns
		std::cerr << "Finding main AoB entry points..." << std::endl;
		mb.find_patterns(&p_vec[0], &p_vec[sizeof(p_vec)/sizeof(p_vec[0])], debug_all);
		if(debug_ptrs) {
			/*
			 * This code is used to ensure the read_mem was
			 * actually working... seems to be :-)
			 */
			for(const auto& p : p_vec) {
				std::ostringstream	ostr;
				if(p->mem_location > 0) {
					const uint64_t	u64 = mb.read_mem<uint64_t>(p->mem_location);
					print_bin(u64, ostr);
				}
				std::fprintf(stderr, "%-16s\t%16li\t%s\n", p->name.c_str(), p->mem_location, ostr.str().c_str());
			}
		}
		std::cerr << "Done" << std::endl;
		// quit at this stage in case we have set the flag debug-all
		if(debug_all)
			return 0;
		if((-1 == p6.mem_location) || (-1 == p2.mem_location))
			throw std::runtime_error("Can't find AoB for patterns::PlayerNameLinux and/or patterns::PlayerDamage - Try to run with 'sudo' and/or specify a pid");
		if(show_monsters_data && (-1 == p3.mem_location))
			throw std::runtime_error("Can't find AoB for patterns::Monster");
		// main loop
		std::unique_ptr<vbrush::iface>	w_dpy(wdisplay::get()),
						f_dpy((file_display.empty()) ? 0 : fdisplay::get(file_display.c_str()));
		ui::app_data			ad{ VERSION, timer::cpu_ms()};
		ui::mhw_data			mhwd;
		size_t				draw_flags = 0;
		if(show_monsters_data)
			draw_flags |= ui::draw_flags::SHOW_MONSTER_DATA;
		mhw_lookup::pattern_data	mhwpd{ &p6, &p2, (show_monsters_data) ? &p3 : 0, &p7 };
		keyb_proc			kp(run);
		// if we don't perform clear, the lazy_alloc
		// option would be rendered useless because
		// the memory::browser recycles memory and if
		// we were not to clear it, we would keep it
		// even with lazy allocations
		if(lazy_alloc)
			mb.clear();
		while(run) {
			timer::thread_tmr	tt(&ad.tm);
			mb.update();
			mhw_lookup::get_data(mhwpd, mb, mhwd);
			ui::draw(w_dpy.get(), draw_flags, ad, mhwd);
			if(f_dpy) ui::draw(f_dpy.get(), draw_flags, ad, mhwd);
			size_t			cur_refresh_tm = 0;
			do {
				const auto 	tm_get = tt.get_wall();
				cur_refresh_tm = (refresh_interval > tm_get) ? (refresh_interval-tm_get) : 0;
			} while(!kp.do_io(cur_refresh_tm));
		}
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}

