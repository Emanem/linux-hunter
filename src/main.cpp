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
#include <thread>
#include <getopt.h>
#include <cstring>
#include "patterns.h"
#include "offsets.h"
#include "memory.h"
#include "ui.h"

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
	const char*	VERSION = "0.0.1";

	// settings/options management
	pid_t		mhw_pid = -1;
	std::string	save_dir,
			load_dir;
	bool		debug_ptrs = false;

	void print_help(const char *prog, const char *version) {
		std::cerr <<	"Usage: " << prog << " [options]\nExecutes linux-hunter " << version << "\n\n"
				"-p, --mhw-pid p   Specifies which pid to scan memory for (usually main MH:W)\n"
				"-s, --save dir    Captures the specified pid into directory 'dir' and quits\n"
				"-l, --load dir    Loads the specified capture directory 'dir' and displays info (static - useful for debugging)\n"
				"    --debug-ptrs  Prints the main AoB (Array of Bytes) pointers (useful for debugging)\n"
				"    --help        prints this help and exit\n\n"
				"Press 'q' or 'ESC' inside linux-hunter to quit\n"
		<< std::flush;
	}

	int parse_args(int argc, char *argv[], const char *prog, const char *version) {
		int			c;
		static struct option	long_options[] = {
			{"help",		no_argument,	   0,	0},
			{"mhw-pid",		required_argument, 0,   'p'},
			{"save",		required_argument, 0,	's'},
			{"load",		required_argument, 0,	'l'},
			{"debug-ptrs",		no_argument,	   0,	0},
			{0, 0, 0, 0}
		};

		while (1) {
			// getopt_long stores the option index here
			int		option_index = 0;

			if(-1 == (c = getopt_long(argc, argv, "p:s:l:", long_options, &option_index)))
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
				}
			} break;

			case 'p': {
				mhw_pid = std::atoi(optarg);
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
	// try get player name
	bool get_data_session(const memory::pattern& mp_Player, memory::browser& mb, ui::data& d) {
		const auto	pnameptr = mb.load_effective_addr_rel(mp_Player.mem_location, true);
		const auto	pnameaddr = mb.read_mem<uint32_t>(pnameptr, true);
		// get session name (this should be UTF-8)...
		d.session_id = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionID, offsets::PlayerNameCollection::IDLength, true);
		d.host_name = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionHostPlayerName, offsets::PlayerNameCollection::PlayerNameLength, true);
		return true;
	}
	// try get player damage (need name too)
	bool get_data_damage(const memory::pattern& mp_Player, const memory::pattern& mp_Damage, memory::browser& mb, ui::data& d) {
		const auto	pnameptr = mb.load_effective_addr_rel(mp_Player.mem_location, true);
		const auto	pnameaddr = mb.read_mem<uint32_t>(pnameptr, true);
		const auto	pdmgroot = mb.load_effective_addr_rel(mp_Damage.mem_location, true);
		const uint32_t	pdmgml[] = { offsets::PlayerDamageCollection::FirstPlayerPtr + (offsets::PlayerDamageCollection::MaxPlayerCount * sizeof(size_t) * offsets::PlayerDamageCollection::NextPlayerPtr ) };
		const auto	pdmglistaddr = mb.load_multilevel_addr_rel(pdmgroot, &pdmgml[0], &pdmgml[1], true);
		// for each player...
		for(uint32_t i = 0; i < offsets::PlayerDamageCollection::MaxPlayerCount; ++i) {
			const auto	pnameoffset = offsets::PlayerNameCollection::PlayerNameLength * i;
			d.players[i].name = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::FirstPlayerName + pnameoffset, offsets::PlayerNameCollection::PlayerNameLength, true);
			d.players[i].used = !d.players[i].name.empty();
			if(!d.players[i].used)
				continue;
			const auto	pfirstplayer = pdmglistaddr + offsets::PlayerDamageCollection::FirstPlayerPtr;
			const auto	pcurplayer = pfirstplayer + offsets::PlayerDamageCollection::NextPlayerPtr * i;
			const auto	curplayeraddr = mb.read_mem<size_t>(pcurplayer, true);
			d.players[i].damage = mb.read_mem<int32_t>(curplayeraddr + offsets::PlayerDamageCollection::Damage, true);
		}
		return true;
	}

	void get_data(const memory::pattern& mp_Player, const memory::pattern& mp_Damage, memory::browser& mb, ui::data& d) {
		d = ui::data();
		if(!get_data_session(mp_Player, mb, d))
			return;
		if(!get_data_damage(mp_Player, mp_Damage, mb, d))
			return;
	}
}

int main(int argc, char *argv[]) {
	try {
		/*ui::window	w;
		ui::data	d;
		d.session_id = "Camw+P?+dNLP";
		d.host_name = "Emetta";
		d.players[0].used = true;
		d.players[0].name = "Jomgor";
		d.players[0].damage = 5909;
		d.players[1].used = true;
		d.players[1].name = "Shenanagins";
		d.players[1].damage = 8736;
		d.players[2].used = true;
		d.players[2].name = "Emetta";
		d.players[2].damage = 18266;
		d.players[3].used = true;
		d.players[3].name = "xyz";
		d.players[3].damage = 6426;
		w.draw(VERSION, d);
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		return 0;*/
		memory::pattern	p0(patterns::PlayerName),
				p1(patterns::CurrentPlayerName),
				p2(patterns::PlayerDamage),
				p3(patterns::Monster),
				p4(patterns::PlayerBuff),
				p5(patterns::Emetta),
				p6(patterns::PlayerNameLinux);
		memory::pattern	*p_vec[] = { &p0, &p1, &p2, &p3, &p4 , &p5, &p6 };
		// parse args first
		const auto optind = parse_args(argc, argv, argv[0], VERSION);
		// check come consistency
		if(!load_dir.empty() && !save_dir.empty())
			throw std::runtime_error("Can't specify both 'load' and 'save' options");
		// start here...
		memory::browser	mb(mhw_pid);
		// if we're in load mode fill b
		// with content from the disk
		if(!load_dir.empty()) {
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
		for(auto p : p_vec) {
			p->mem_location = mb.find_first(*p);
			if(debug_ptrs) {
				/*
				 * This code is used to ensure the read_mem was
				 * actually working... seems to be :-)
				 */
				std::ostringstream	ostr;
				if(p->mem_location > 0) {
					const uint64_t	u64 = mb.read_mem<uint64_t>(p->mem_location);
					print_bin(u64, ostr);
				}
				std::fprintf(stderr, "%-16s\t%16li\t%s\n", p->name.c_str(), p->mem_location, ostr.str().c_str());
			}
		}
		std::cerr << "Done" << std::endl;
		if(-1 == p6.mem_location || -1 == p2.mem_location)
			throw std::runtime_error("Can't find AoB for patterns::PlayerNameLinux and/or patterns::PlayerDamage");
		// main loop
		// TODO manage keyboards input
		ui::window	w;
		ui::data	d;
		while(true) {
			get_data(p6, p2, mb, d);
			w.draw(VERSION, d);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}

