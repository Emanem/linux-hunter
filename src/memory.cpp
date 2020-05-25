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
#include <algorithm>
#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/uio.h>

memory::pattern::pattern() {
}

memory::pattern::pattern(const patterns::pattern& p) {
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

void memory::browser::snap_pid(const pid_t pid) {
	all_regions_.clear();
	/* example :
	* 5662e000-56a21000 r-xp 00000000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a21000-56a36000 r--p 003f3000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a36000-56a43000 rw-p 00408000 08:16 29098197                           /home/ema/.steam/ubuntu12_32/steam
	* 56a43000-56a66000 rw-p 00000000 00:00 0 
	* 57524000-58428000 rw-p 00000000 00:00 0                                  [heap]
	* cbbfd000-cbbfe000 ---p 00000000 00:00 0 
	* 
	* */
	const std::string	maps_name = std::string("/proc/") + std::to_string(pid) + "/maps";
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
			all_regions_.push_back(mem_region(beg, end, line));
	}

	for(auto& v : all_regions_) {
		const ssize_t		sz = v.end - v.beg;
		const struct iovec	local = { (void*)v.data, (size_t)sz },
					remote = { (void*)v.beg, (size_t)sz };
		const auto		rv = process_vm_readv(pid, &local, 1, &remote, 1, 0);
		if(-1 >= rv) {
			std::cerr << "Region: " << v.debug_info << " Error with process_vm_readv (" << std::to_string(errno) << " " << strerror(errno) << ")" << std::endl;
			v.data_sz = -1;
			continue;
		}
		if(sz != rv) {
			std::cerr << "Region: " << v.debug_info << "coudln't be fully read: " << sz << " vs " << rv << std::endl;
			v.data_sz = rv;
		}
	}
}

ssize_t memory::browser::find_once(const pattern& p, const uint8_t* buf, const size_t sz, pbyte& hint) const {
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
			}
		} else {
			// simple comparisons here
			if(std::memcmp(p_buf + i.tgt_offset, &p.bytes[i.src_offset], i.length)) {
				hint = p_buf + p.matches.begin()->length;
				return -1;
			}
		}
	}
	// print out the buffer for reference
	/*const int	len = p.matches.rbegin()->tgt_offset + p.matches.rbegin()->length;
	for(int j = 0; j < len; ++j) {
		std::printf("%02X ", p_buf[j]);
	}
	std::printf("\n");*/
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

memory::browser::browser() {
}

memory::browser::~browser() {
}

void memory::browser::snap(const pid_t pid) {
	snap_pid(pid);
	verify_regions();
}

void memory::browser::store(const char* dir_name) {
	auto	*d = opendir(dir_name);
	if(!d) throw std::runtime_error("Can't store data, invalid directory");
	else closedir(d);
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
		all_regions_.push_back(mem_region(i.beg, i.end, i.file));
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

ssize_t memory::browser::find_first(const pattern& p, const size_t start_addr) {
	for(const auto& v : all_regions_) {
		if(v.end <= start_addr)
			continue;
		const uint8_t	*p_buf = v.data,
				*p_hint = 0;
		size_t		p_size = v.data_sz;
		while(true) {
			const auto	rs = find_once(p, p_buf, p_size, p_hint);
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

std::string memory::browser::read_utf8(const size_t addr, const size_t len) {
	// pre-condition: all_regions_ is
	// actually correctly formatted
	// addr between boundaries is _not_
	// supported
	for(const auto& v : all_regions_) {
		if(v.beg > addr || v.end <= addr)
			continue;
		if(addr + len > (v.data_sz + v.beg))
			throw std::runtime_error("Can't interpret memory, T size too large");
		const char*	utf8_ptr = (const char*)&v.data[addr - v.beg];
		return std::string((const char*)utf8_ptr, utf8_ptr + len);
	}
	throw std::runtime_error("Coudln't find specified address");
}

size_t memory::browser::load_effective_addr_rel(const size_t addr) {
	const int	opcodeLength = 3,
			paramLength = 4,
			instructionLength = opcodeLength + paramLength;

	uint32_t operand = read_mem<uint32_t>(addr + opcodeLength);
	uint64_t operand64 = operand;

	// 64 bit relative addressing 
	if (operand64 > std::numeric_limits<int32_t>::max()) {
		operand64 = 0xffffffff00000000 | operand64;
	}
	return addr + operand64 + instructionLength;
}

size_t memory::browser::load_multilevel_addr_rel(const size_t addr, const uint32_t* off_b, const uint32_t* off_e) {
	size_t	rv = addr;
	for(auto i = off_b; i != off_e; ++i) {
		const auto	cur_rv = read_mem<size_t>(rv);
		rv = (uint32_t)(cur_rv) + *i;

	}
	return rv;
}

