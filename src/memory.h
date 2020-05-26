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

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <string>
#include <vector>
#include <ostream>
#include "patterns.h"

namespace memory {
	struct pattern {
		std::vector<uint8_t>	bytes;
		struct offlen {
			size_t	src_offset,
				tgt_offset,
				length;
		};
		std::vector<offlen>	matches;
		std::string		name;
		ssize_t			mem_location;

		pattern();
		pattern(const patterns::pattern& p);
		void print(std::ostream& ostr);
	};

	class browser {
		typedef const uint8_t*	pbyte;

		struct mem_region {
			uint64_t	beg,
					end;
			std::string	debug_info;
			uint8_t		*data;
			ssize_t		data_sz;
			bool		dirty;

			mem_region(uint64_t b, uint64_t e, const std::string& d) : beg(b), end(e), debug_info(d), data((uint8_t*)std::malloc(e-b)), data_sz(e-b), dirty(true) {
				if(!data)
					throw std::runtime_error((std::string("Can't allocate mem_region (") + std::to_string(b) + "," + std::to_string(e) + ")").c_str());
			}

			mem_region(mem_region&& rhs) : beg(std::move(rhs.beg)), end(std::move(rhs.end)), debug_info(std::move(rhs.debug_info)), data(std::move(rhs.data)), data_sz(std::move(rhs.data_sz))  {
				rhs.data = 0;
			}

			mem_region(const mem_region&) = delete;
			mem_region& operator=(const mem_region&) = delete;

			~mem_region() {
				if(data)
					std::free(data);
			}
		};

		pid_t			pid_;
		bool			dirty_opt_;
		std::vector<mem_region>	all_regions_;

		void snap_pid(void);

		ssize_t find_once(const pattern& p, const uint8_t* buf, const size_t sz, pbyte& hint, const bool debug_all) const;

		void verify_regions(void);

		void refresh_region(mem_region& r);
	public:
		browser(const pid_t p, const bool dirty_opt);

		~browser();

		void snap(void);

		void set_mem_dirty(void);

		void store(const char* dir_name);
		
		void load(const char* dir_name);

		ssize_t find_first(const pattern& p, const bool debug_all, const size_t start_addr = 0);

		template<typename T>
		T read_mem(const size_t addr, const bool refresh = false) {
			// pre-condition: all_regions_ is
			// actually correctly formatted
			// addr between boundaries is _not_
			// supported
			for(auto& v : all_regions_) {
				if(v.beg > addr || v.end <= addr)
					continue;
				if(refresh)
					refresh_region(v);
				if(addr + sizeof(T) > (v.data_sz + v.beg))
					throw std::runtime_error("Can't interpret memory, T size too large");
				return *(T*)&v.data[addr - v.beg];
			}
			throw std::runtime_error("Coudln't find specified address");
		}

		std::wstring read_utf8(const size_t addr, const size_t len, const bool refresh = false);

		size_t load_effective_addr_rel(const size_t addr, const bool refresh = false);

		size_t load_multilevel_addr_rel(const size_t addr, const uint32_t* off_b, const uint32_t* off_e, const bool refresh = false);
	};
}

#endif //_MEMORY_H_

