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

#include "memory.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/uio.h>
#include <iconv.h>

memory::pattern::pattern() : mem_location(-1) {
}

memory::pattern::pattern(const patterns::pattern& p) : mem_location(-1) {
	size_t		tgt_offset = 0;
	const char	*cur_text_byte = p.bytes,
			*cur_text_end = p.bytes + std::strlen(p.bytes);
	bool		reset_chunk = true;
	while(cur_text_byte < cur_text_end) {
		// 0. check if we have empty space or any '??'
		if(*cur_text_byte == ' ') {
			++cur_text_byte;
			continue;
		} else if(cur_text_byte == std::strstr(cur_text_byte, "??")) {
			cur_text_byte += 2;
			reset_chunk = true;
			++tgt_offset;
			continue;
		}
		// 1. read a byte
		unsigned int	cur_byte = 0;
		if(1 != std::sscanf(cur_text_byte, "%02X", &cur_byte))
			throw std::runtime_error("Invalid pattern text bytes format");
		cur_text_byte += 2;
		if(cur_byte > 0xFF)
			throw std::runtime_error((std::string("Invalid patter text bytes format (larger than byte ") + cur_text_byte + ")").c_str());
		bytes.push_back((uint8_t)cur_byte);
		// 2. add data to offlen vectors
		if(reset_chunk) {
			reset_chunk = false;
			matches.push_back(offlen{ bytes.size()-1, tgt_offset, 0 });
		}
		++matches.rbegin()->length;
		++tgt_offset;
	}
	name = p.name;
}

void memory::pattern::print(std::ostream& ostr) {
	for(const auto& i : bytes) {
		char	buf[3];
		std::snprintf(buf, 3, "%02X", i);
		buf[2] = '\0';
		ostr << buf << ' ';
	}
	for(const auto& i : matches) {
		ostr << "\n(" << i.src_offset << "," << i.tgt_offset << "," << i.length << ")";
	}
}

void memory::browser::snap_mem_regions(std::vector<mem_region>& mr, const bool alloc_mem) {
	mr.clear();
	/* example :
	* 5662e000-56a21000 r-xp 00000000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a21000-56a36000 r--p 003f3000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a36000-56a43000 rw-p 00408000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a43000-56a66000 rw-p 00000000 00:00 0 
	* 57524000-58428000 rw-p 00000000 00:00 0                                  [heap]
	* cbbfd000-cbbfe000 ---p 00000000 00:00 0 
	* 
	* */
	const std::string	maps_name = std::string("/proc/") + std::to_string(pid_) + "/maps";
	std::ifstream		istr(maps_name.c_str());
	if(!istr)
		throw std::runtime_error("Can't open /proc/.../maps");
	std::string line;
	while(std::getline(istr, line)) {
		uint64_t	beg = 0,
				end = 0,
				offset = 0,
				inode = 0;
		char		permissions[32],
				device[32];
		const auto rv = std::sscanf(line.c_str(), "%lx-%lx %s %lx %s %li", &beg, &end, permissions, &offset, device, &inode);
		if(rv == 6 && !inode && permissions[0] == 'r')
			mr.push_back(mem_region(beg, end, line, alloc_mem));
	}
}

void memory::browser::snap_pid(void) {
	if(pid_ < 0)
		throw std::runtime_error((std::string("Can't snap invalid pid (" + std::to_string(pid_) + ")")).c_str());

	snap_mem_regions(all_regions_, true);
	for(auto& v : all_regions_) {
		const ssize_t		sz = v.end - v.beg;
		const struct iovec	local = { (void*)v.data, (size_t)sz },
					remote = { (void*)v.beg, (size_t)sz };
		const auto		rv = process_vm_readv(pid_, &local, 1, &remote, 1, 0);
		if(-1 >= rv) {
			std::cerr << "Region: " << v.debug_info << " Error with process_vm_readv (" << std::to_string(errno) << " " << strerror(errno) << ")" << std::endl;
			v.data_sz = -1;
			continue;
		}
		if(sz != rv) {
			std::cerr << "Region: " << v.debug_info << "coudln't be fully read: " << sz << " vs " << rv << std::endl;
			v.data_sz = rv;
		}
		v.dirty = false;
	}
}

void memory::browser::update_regions(void) {
	if(-1 == pid_)
		return;
	std::vector<mem_region>	new_regions;
	new_regions.reserve(all_regions_.size()*2);
	// get them w/o allocating memory
	snap_mem_regions(new_regions, false);
	// then merge the current into new (if possible)
	// regions are supposed to be sorted, so that
	// below algorithm should be O(N) instead of
	// O(N^2)
	size_t hint = 0;
	for(auto& r : new_regions) {
		// lookup the same in the current regions
		// using the hint
		size_t	idx = 0;
		for(idx = hint; idx < all_regions_.size(); ++idx) {
			mem_region&	cur_r = all_regions_[idx];
			// if we have match 'move' the region
			// content
			if((cur_r.beg == r.beg) && (cur_r.end == r.end)) {
				r = std::move(cur_r);
				hint = idx+1;
				break;
			}
		}
		// if we don't have 'data' member initilized
		// allocate memory - this is expensive
		// hopefully doesn't happen frequently
		if(!r.data) {
			r.data = (uint8_t*)std::malloc(r.end-r.beg);
			if(!r.data)
				throw std::runtime_error((std::string("Can't allocate mem_region (") + std::to_string(r.beg) + "," + std::to_string(r.end) + ")").c_str());
		}
	}
	// finally, swap vectors
	all_regions_.swap(new_regions);
}

ssize_t memory::browser::find_once(const pattern& p, const uint8_t* buf, const size_t sz, pbyte& hint, const bool debug_all) const {
	bool		first = true;
	const uint8_t	*p_buf = 0;
	for(const auto& i : p.matches) {
		if(first) {
			p_buf = std::search(buf, buf+sz, &p.bytes[i.src_offset], &p.bytes[i.src_offset + i.length]);
			if(p_buf == buf+sz) {
				hint = buf+sz;
				return -1;
			} else {
				hint = buf;
				first = false;
				if(debug_all) {
					const int	len = p.matches.rbegin()->tgt_offset + p.matches.rbegin()->length;
					for(int j = 0; j < len; ++j) {
						std::printf("%02X ", p_buf[j]);
					}
					std::printf("\n");
				}
			}
		} else {
			// simple comparisons here
			if(std::memcmp(p_buf + i.tgt_offset, &p.bytes[i.src_offset], i.length)) {
				hint = p_buf + p.matches.begin()->length;
				return -1;
			}
		}
	}
	return p_buf - buf;
}

void memory::browser::verify_regions(void) {
	uint64_t	c_beg = 0,
			c_end = 0;
	bool		first = true;
	for(const auto& v : all_regions_) {
		if(first) {
			c_beg = v.beg;
			c_end = v.end;
			first = false;
			continue;
		}
		// ensure those are ordered
		if(v.beg < c_beg)
			throw std::runtime_error("Invalid region sequence - order");
		if(v.beg < c_end)
			throw std::runtime_error("Invalid region sequence - overlaps");
		c_beg = v.beg;
		c_end = v.end;
	}
}

void memory::browser::refresh_region(mem_region& r) {
	if(pid_ < 0)
		return;
	if(dirty_opt_ && !r.dirty)
		return;

	const ssize_t		sz = r.end - r.beg;
	const struct iovec	local = { (void*)r.data, (size_t)sz },
				remote = { (void*)r.beg, (size_t)sz };
	const auto		rv = process_vm_readv(pid_, &local, 1, &remote, 1, 0);
	if(-1 >= rv) {
		std::cerr << "Region: " << r.debug_info << " Error with process_vm_readv (" << std::to_string(errno) << " " << strerror(errno) << ")" << std::endl;
		r.data_sz = -1;
		return;
	}
	if(sz != rv) {
		r.data_sz = rv;
	}
	r.dirty = false;
}

memory::browser::browser(const pid_t p, const bool dirty_opt) : pid_(p), dirty_opt_(dirty_opt) {
}

memory::browser::~browser() {
}

void memory::browser::snap(void) {
	snap_pid();
	verify_regions();
}

void memory::browser::update(void) {
	// need to check memory layout
	// usually shouldn't change much
	// but it _does_ sometime
	update_regions();
	// don't execute the code
	// in case we haven't enabled
	// dirty_opt_
	if(!dirty_opt_)
		return;
	for(auto& v: all_regions_)
		v.dirty = true;
}

void memory::browser::store(const char* dir_name) {
	auto	*d = opendir(dir_name);
	if(!d) {
		if(mkdir(dir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
			throw std::runtime_error((std::string("Can't find nor create directory '") + dir_name + "'").c_str());
	} else closedir(d);
	for(const auto& v : all_regions_) {
		char		f_name[256];
		std::snprintf(f_name, 255, "%s/mem.%016lx-%016lx.bin", dir_name, v.beg, v.end);
		std::ofstream	ostr(f_name, std::ios_base::binary);
		if(!ostr)
			throw std::runtime_error("Can't open file for writing (check path/permission)");
		const size_t	lcl_data_sz = (v.data_sz > 0) ? v.data_sz : 0;
		if(lcl_data_sz != (size_t)ostr.write((const char*)v.data, lcl_data_sz).tellp())
			throw std::runtime_error("can't write the whole file");
	}
}

void memory::browser::load(const char* dir_name) {
	all_regions_.clear();
	std::unique_ptr<DIR, void(*)(DIR*)>	d(opendir(dir_name), [](DIR *d){ if(d) closedir(d);});
	if(!d)
		throw std::runtime_error("opendir");
	struct mem_data {
		uint64_t	beg = 0,
				end = 0;
		std::string	file;
	};
	std::vector<mem_data>	mem_files;
	struct dirent	*de = 0;
	while((de = readdir(d.get())) != 0) {
		if(de->d_type != DT_REG)
			continue;
		mem_data	md;
		if(2 != std::sscanf(de->d_name, "mem.%016lx-%016lx.bin", &md.beg, &md.end)) {
			std::cerr << "Skipping file " << de->d_name << std::endl;
			continue;
		}
		if(!(md.beg < md.end))
			throw std::runtime_error("Invalid file (addresses are in wrong order)");
		md.file = de->d_name;
		mem_files.push_back(md);
	}
	std::sort(mem_files.begin(), mem_files.end(), [](const mem_data& lhs, const mem_data& rhs) -> bool { return lhs.file < rhs.file; } );
	all_regions_.clear();
	for(const auto& i : mem_files) {
		all_regions_.push_back(mem_region(i.beg, i.end, i.file, true));
		auto& latest_reg = *all_regions_.rbegin();
		// load data
		std::ifstream	istr((std::string(dir_name) + "/" + i.file).c_str(), std::ios_base::binary);
		istr.seekg(0, std::ios_base::end);
		const auto sz = istr.tellg();
		istr.seekg(0, std::ios_base::beg);
		if(sz > (ssize_t)(i.end-i.beg))
			throw std::runtime_error("Invalid file (larger than mapped memory)");
		if(istr.read((char*)latest_reg.data, sz).gcount() != sz)
			throw std::runtime_error("Can't read whole file");
		latest_reg.data_sz = sz;
	}
	verify_regions();
}

ssize_t memory::browser::find_first(const pattern& p, const bool debug_all, const size_t start_addr) {
	for(const auto& v : all_regions_) {
		if(v.end <= start_addr)
			continue;
		const uint8_t	*p_buf = v.data,
				*p_hint = 0;
		size_t		p_size = v.data_sz;
		while(true) {
			const auto	rs = find_once(p, p_buf, p_size, p_hint, debug_all);
			const ssize_t	hint_diff = p_hint - v.data;
			if(rs >= 0)
				return rs + hint_diff + v.beg;
			if(hint_diff >= v.data_sz)
				break;
			p_buf = p_hint;
			p_size = v.data_sz - hint_diff;
		}
	}
	return -1;
}

namespace {
	std::wstring from_utf8(const char* in, size_t sz) {
		std::wstring	rv;
		rv.resize(sz);
		auto		conv = iconv_open("WCHAR_T", "UTF-8");
		char		*pIn = (char*)in,
				*pOut = (char*)&rv[0];
		size_t		sIn = sz,
				sOut = sz*sizeof(wchar_t);
		if(((void*)-1) == conv)
			throw std::runtime_error("This system can't convert from UTF-8 to WCHAR_T");
		iconv(conv, &pIn, &sIn, &pOut, &sOut);
		iconv_close(conv);
		return rv;
	}
}

bool memory::browser::safe_read_utf8(const size_t addr, const size_t len, std::wstring& out, const bool refresh) {
	// pre-condition: all_regions_ is
	// actually correctly formatted
	// addr between boundaries is _not_
	// supported
	for(auto& v : all_regions_) {
		if(v.beg > addr || v.end <= addr)
			continue;
		if(refresh)
			refresh_region(v);
		if(addr + len > (v.data_sz + v.beg))
			throw std::runtime_error("Can't interpret memory, T size too large");
		const char*	utf8_ptr = (const char*)&v.data[addr - v.beg];
		out = from_utf8(utf8_ptr, len);
		return true;
	}
	return false;
}

bool memory::browser::safe_load_effective_addr_rel(const size_t addr, size_t& out, const bool refresh) {
	const int	opcodeLength = 3,
			paramLength = 4,
			instructionLength = opcodeLength + paramLength;

	uint32_t operand;
	if(!safe_read_mem<uint32_t>(addr + opcodeLength, operand, refresh))
		return false;
	uint64_t operand64 = operand;

	// 64 bit relative addressing 
	if (operand64 > std::numeric_limits<int32_t>::max()) {
		operand64 = 0xffffffff00000000 | operand64;
	}
	out = addr + operand64 + instructionLength;
	return true;
}

bool memory::browser::safe_load_multilevel_addr_rel(const size_t addr, const uint32_t* off_b, const uint32_t* off_e, size_t& out, const bool refresh) {
	size_t	rv = addr;
	for(auto i = off_b; i != off_e; ++i) {
		size_t	cur_rv;
		if(!safe_read_mem<size_t>(rv, cur_rv, refresh))
			return false;
		if(!cur_rv)
			return false;
		rv = (size_t)((int64_t)(cur_rv) + *i);

	}
	out = rv;
	return true;
}

std::wstring memory::browser::read_utf8(const size_t addr, const size_t len, const bool refresh) {
	std::wstring	rv;
	if(!safe_read_utf8(addr, len, rv, refresh))
		throw std::runtime_error("Couldn't find specified address");
	return rv;
}

size_t memory::browser::load_effective_addr_rel(const size_t addr, const bool refresh) {
	size_t	out;
	if(!safe_load_effective_addr_rel(addr, out, refresh))
		throw std::runtime_error("Couldn't find specified address");
	return out;
}

size_t memory::browser::load_multilevel_addr_rel(const size_t addr, const uint32_t* off_b, const uint32_t* off_e, const bool refresh) {
	size_t	rv;
	if(!safe_load_multilevel_addr_rel(addr, off_b, off_e, rv, refresh))
		throw std::runtime_error("Couldn't find specified address");
	return rv;
}

