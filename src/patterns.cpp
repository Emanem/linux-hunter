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

#include "patterns.h"

/*
 * Patterns are taken from:
 * https://github.com/sir-wilhelm/SmartHunter/blob/master/SmartHunter/Game/Config/MemoryConfig.cs
 * Please note that some patterns have to be adapted on Linux,
 * because with wine some structures have a different layout
 * For example, 'PlayerName' can't be found, but 'PlayerNameLinux' 
 * can be found.
 */

patterns::pattern patterns::PlayerName {
	"PlayerName",
	"48 8B 0D ?? ?? ?? ?? 48 8D 54 24 38 C6 44 24 20 00 E8 ?? ?? ?? ?? 48 8B 5C 24 70 48 8B 7C 24 60 48 83 C4 68 C3"
};

patterns::pattern patterns::CurrentPlayerName {
	"CurrentPlayerName",
	"48 8B 0D ?? ?? ?? ?? 48 8D 55 ?? 45 31 C9 41 89 C0 E8"
};

patterns::pattern patterns::PlayerDamage {
	"PlayerDamage",
	"48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 75 04 33 C9"
};

patterns::pattern patterns::Monster {
	"Monster",
	"48 8B 0D ?? ?? ?? ?? B2 01 E8 ?? ?? ?? ?? C6 83 ?? ?? ?? ?? ?? 48 8B 0D"
};

patterns::pattern patterns::PlayerBuff {
	"PlayerBuff",
	"48 8B 05 ?? ?? ?? ?? 41 8B 94 00 ?? ?? ?? ?? 89 57"
};

patterns::pattern patterns::LobbyStatus {
	"LobbyStatus",
	"48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 4E ?? F3 0F 10 86 ?? ?? ?? ?? F3 0F 58 86 ?? ?? ?? ?? F3 0F 11 86 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 4E"
};

patterns::pattern patterns::Emetta {
	"Emetta",
	"45 6D 65 74 74 61"
};

patterns::pattern patterns::PlayerNameLinux {
	"PlayerNameLinux",
	"48 8B 0D ?? ?? ?? ?? 48 8D 54 24 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 8B 5C 24 60 48 83 C4 50 5F C3"
};

