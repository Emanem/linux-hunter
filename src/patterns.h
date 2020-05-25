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

#ifndef _PATTERNS_H_
#define _PATTERNS_H_

namespace patterns {
	struct pattern {
		const char	*name,
		      		*bytes;
	};

	extern pattern	PlayerName,
			CurrentPlayerName,
			PlayerDamage,
			Monster,
			PlayerBuff,
			Emetta,
			PlayerNameLinux;
}

#endif //_PATTERNS_H_

