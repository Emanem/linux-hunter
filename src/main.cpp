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
#include "patterns.h"
#include "offsets.h"
#include "memory.h"

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

int main(int argc, char *argv[]) {
	try {
		memory::pattern	p0(patterns::PlayerName),
				p1(patterns::CurrentPlayerName),
				p2(patterns::PlayerDamage),
				p3(patterns::Monster),
				p4(patterns::PlayerBuff),
				p5(patterns::Emetta),
				p6(patterns::PlayerNameLinux);
		memory::pattern	*p_vec[] = { &p0, &p1, &p2, &p3, &p4 , &p5, &p6 };

		if(argc < 2)
			throw std::runtime_error("missing argument MH:W pid");

		const int	mhw_pid = std::atoi(argv[1]);

		memory::browser	b;
		//b.snap(mhw_pid);
		bool load = false;
		if(load) {
		b.load("mhw_dump");
		for(const auto p : p_vec) {
			const auto rv = b.find_first(*p);
			std::printf("%-16s\t%16li\n", p->name.c_str(), rv);
			/*
			 * This code is used to ensure the read_mem was
			 * actually working... seems to be :-)
			if(rv > 0) {
				const uint64_t	u64 = b.read_mem<uint64_t>(rv);
				print_bin(u64, std::cout);
				std::cout << std::endl;
			}*/
		}
		return 0;
		}
		b.load("mhw_dump");
		// try get player name
		{
			const auto 	rv = b.find_first(p6);
			const auto	pnameptr = b.load_effective_addr_rel(rv);
			const auto	pnameaddr = b.read_mem<uint32_t>(pnameptr);
			// get session name (this should be UTF-8)...
			const auto	sname = b.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionID, offsets::PlayerNameCollection::IDLength);
			std::cout << "sname: " << sname << std::endl;
			const auto	shostname = b.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionHostPlayerName, offsets::PlayerNameCollection::PlayerNameLength);
			std::cout << "shostname: " << shostname << std::endl;
		}
		// try get player damage (need name too)
		{
			const auto 	rv = b.find_first(p6);
			const auto	pnameptr = b.load_effective_addr_rel(rv);
			const auto	pnameaddr = b.read_mem<uint32_t>(pnameptr);
			const auto 	rvdmg = b.find_first(p2);
			const auto	pdmgroot = b.load_effective_addr_rel(rvdmg);
			const uint32_t	pdmgml[] = { offsets::PlayerDamageCollection::FirstPlayerPtr + (offsets::PlayerDamageCollection::MaxPlayerCount * sizeof(size_t) * offsets::PlayerDamageCollection::NextPlayerPtr ) };
			const auto	pdmglistaddr = b.load_multilevel_addr_rel(pdmgroot, &pdmgml[0], &pdmgml[1]);
			// for each player...
			for(uint32_t i = 0; i < offsets::PlayerDamageCollection::MaxPlayerCount; ++i) {
				const auto	pnameoffset = offsets::PlayerNameCollection::PlayerNameLength * i;
				const auto	name = b.read_utf8(pnameaddr + offsets::PlayerNameCollection::FirstPlayerName + pnameoffset, offsets::PlayerNameCollection::PlayerNameLength);
				const auto	pfirstplayer = pdmglistaddr + offsets::PlayerDamageCollection::FirstPlayerPtr;
				const auto	pcurplayer = pfirstplayer + offsets::PlayerDamageCollection::NextPlayerPtr * i;
				const auto	curplayeraddr = b.read_mem<size_t>(pcurplayer);
				const auto	cplayerdmg = b.read_mem<int>(curplayeraddr + offsets::PlayerDamageCollection::Damage);
				std::cout << "Player " << i << ":\t" << cplayerdmg << std::endl;
			}
		}
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}

