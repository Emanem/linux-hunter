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

#ifndef _OFFSETS_H_
#define _OFFSETS_H_

/*
 * Taken from https://github.com/sir-wilhelm/SmartHunter/blob/master/SmartHunter/Game/Helpers/MhwHelper.cs
 */

namespace offsets {
	namespace PlayerNameCollection {
		const static uint32_t	IDLength = 12,
		      			PlayerNameLength = 32,
					FirstPlayerName = 0x53305,
					SessionID = FirstPlayerName + 0xF43,
					SessionHostPlayerName = SessionID + 0x3F,
					LobbyID = FirstPlayerName + 0x463,
					LobbyHostPlayerName = LobbyID + 0x29,
					NextLobbyHostName = 0x2F;
	}

	namespace PlayerDamageCollection {
		const static uint32_t	MaxPlayerCount = 4,
		      			FirstPlayerPtr = 0x48,
					NextPlayerPtr = 0x58,
					Damage = 0x48;
	}

	namespace Monster {
		const static uint32_t	PreviousMonsterOffset = 0x10,
					NextMonsterOffset = 0x18,
					MonsterStartOfStructOffset = 0x40,
					MonsterHealthComponentOffset = 0x7670,
					MonsterNumIDOffset = 0x12280,
					MonsterSizeScale = 0x188,
					MonsterScaleModifier = 0x7730;
	}

	namespace MonsterModel {
		const static uint32_t	IdLength = 32, // 64?
					IdOffset = 0x179;
	}

	namespace MonsterHealthComponent {
		const static uint32_t	MaxHealth = 0x60,
					CurrentHealth = 0x64;
	}
}

#endif //_OFFSETS_H_

