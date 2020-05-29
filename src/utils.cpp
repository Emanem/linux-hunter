#include "utils.h"
#include <dirent.h>
#include <unistd.h>
#include <memory>
#include <cctype>
#include <fstream>
#include <cstring>

pid_t utils::find_mhw_pid(void) {
	std::unique_ptr<DIR, void(*)(DIR*)>	d(opendir("/proc"), [](DIR *d){ if(d) closedir(d);});
	if(!d)
		throw std::runtime_error("Can't find MH:W pid - '/proc' doesn't seem to exist");
	struct dirent	*de = 0;
	while((de = readdir(d.get())) != 0) {
		if(de->d_type != DT_DIR)
			continue;
		if(de->d_name[0] == '\0' || !std::isdigit(de->d_name[0]))
			continue;
		std::ifstream	istr((std::string("/proc/") + de->d_name + "/cmdline").c_str());
		std::string	line;
		if(!std::getline(istr, line))
			continue;
		// "Z:\\disk5\\SteamLibrary\\steamapps\\common\\Monster Hunter World\\MonsterHunterWorld.exe"
		const static char	MHW_EXE[] = "\\MonsterHunterWorld.exe";
		const char		*ptr_mhw = std::strstr(line.c_str(), MHW_EXE);
		if(ptr_mhw && (ptr_mhw[23] == '\0'))
			return std::atoi(de->d_name);
	}
	throw std::runtime_error("Can't find MH:W pid");
}

