#include "mhw_lookup.h"
#include "mhw_lookup_monster.h"
#include "offsets.h"
#include <regex>
#include <algorithm>
#include <cwchar>
#include <cmath> 

namespace {
	bool sort_MONSTERS(void) {
		// apparently this structure can be modified
		// at runtime, is not written in the R/O
		// location of the executable
		std::sort(&mhw_lookup::MONSTERS[0], &mhw_lookup::MONSTERS[sizeof(mhw_lookup::MONSTERS)/sizeof(mhw_lookup::monster_data)],
				[](const mhw_lookup::monster_data& lhs, const mhw_lookup::monster_data& rhs) -> bool {
					return lhs.num_id < rhs.num_id;
				});
		return true;
	}

	// this is to ensure that monster data is properly sorted
	const bool	sorted_MONSTERS = sort_MONSTERS();

	const mhw_lookup::crown_preset_data* get_crown_preset(const uint8_t crown_preset_id) {
		// use binary search via lower_bound
		const auto	*first = &mhw_lookup::CROWN_PRESETS[0],
				*last = &mhw_lookup::CROWN_PRESETS[sizeof(mhw_lookup::CROWN_PRESETS)/sizeof(mhw_lookup::crown_preset_data)];
		const auto	it = std::lower_bound(first, last, crown_preset_id,
				[](const mhw_lookup::crown_preset_data& lhs, const uint32_t rhs) -> bool {
					return lhs.crown_preset_id < rhs;
				});

		if(it != last && it->crown_preset_id == crown_preset_id)
			return it;

		return NULL;
	}

	const mhw_lookup::monster_data* get_monster_stored_data(const uint32_t id) {
		// use binary search via lower_bound
		const auto	*first = &mhw_lookup::MONSTERS[0],
				*last = &mhw_lookup::MONSTERS[sizeof(mhw_lookup::MONSTERS)/sizeof(mhw_lookup::monster_data)];
		const auto	it = std::lower_bound(first, last, id,
				[](const mhw_lookup::monster_data& lhs, const uint32_t rhs) -> bool {
					return lhs.num_id < rhs;
				});

		if(it != last && it->num_id == id)
			return it;

		return NULL;
	}
	
	// get session info 
	bool get_data_session(const memory::pattern* player, memory::browser& mb, ui::mhw_data& d) {
		const auto	pnameptr = mb.load_effective_addr_rel(player->mem_location, true);
		const auto	pnameaddr = mb.read_mem<uint32_t>(pnameptr, true);
		// get session name (this should be UTF-8)...
		d.session_id = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionID, offsets::PlayerNameCollection::IDLength, true);
		d.host_name = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::SessionHostPlayerName, offsets::PlayerNameCollection::PlayerNameLength, true);
		return true;
	}

	// try to understand if the player is in hunt
	bool get_data_ishunt(const memory::pattern* lobby, memory::browser& mb) {
		// in case we can't resolve lobby, return true
		if(!lobby || (lobby->mem_location == -1))
			return true;
		const auto	plobbyptr = mb.load_effective_addr_rel(lobby->mem_location, true);
		const auto	lobbyaddr = mb.read_mem<size_t>(plobbyptr, true);
		const bool	is_mission = mb.read_mem<uint32_t>(lobbyaddr + 0x54, true) != 0,
				is_expedition = mb.read_mem<uint32_t>(lobbyaddr + 0x38, true) != 1;
		return is_mission || is_expedition;
	}

	// try get players' damage (need name too)
	bool get_data_damage(const mhw_lookup::pattern_data& pd, memory::browser& mb, ui::mhw_data& d) {
		const auto	pnameptr = mb.load_effective_addr_rel(pd.player->mem_location, true);
		const auto	pnameaddr = mb.read_mem<uint32_t>(pnameptr, true);
		const auto	pdmgroot = mb.load_effective_addr_rel(pd.damage->mem_location, true);
		const uint32_t	pdmgml[] = { offsets::PlayerDamageCollection::FirstPlayerPtr + (offsets::PlayerDamageCollection::MaxPlayerCount * sizeof(size_t) * offsets::PlayerDamageCollection::NextPlayerPtr ) };
		const auto	pdmglistaddr = mb.load_multilevel_addr_rel(pdmgroot, &pdmgml[0], &pdmgml[1], true);
		if(!pdmglistaddr)
			return false;
		// for each player...
		for(uint32_t i = 0; i < offsets::PlayerDamageCollection::MaxPlayerCount; ++i) {
			// not sure why, but on Linux the offset has 1 more byte for each entry...
			const auto	pnameoffset = offsets::PlayerNameCollection::PlayerNameLength * i + i*1;
			d.players[i].name = mb.read_utf8(pnameaddr + offsets::PlayerNameCollection::FirstPlayerName + pnameoffset, offsets::PlayerNameCollection::PlayerNameLength, true);
			// a player slot is used if the string is non empty and
			// it is not made up all of '\0's...
			d.players[i].used = (!d.players[i].name.empty()) && (d.players[i].name.find_first_not_of(L'\0') != std::wstring::npos); 
			if(!d.players[i].used)
				continue;
			const auto	pfirstplayer = pdmglistaddr + offsets::PlayerDamageCollection::FirstPlayerPtr;
			const auto	pcurplayer = pfirstplayer + offsets::PlayerDamageCollection::NextPlayerPtr * i;
			const auto	curplayeraddr = mb.read_mem<size_t>(pcurplayer, true);
			d.players[i].damage = mb.read_mem<int32_t>(curplayeraddr + offsets::PlayerDamageCollection::Damage, true);
			// usually MH:W IB just overwrites the first wchar_t of
			// the utf8 string with '\0' when a player leaves, which
			// gives us the chance to ascertain if a player has left
			// or not, because the 'name' wouldn't be empty but first
			// char would be '\0'
			// Also damage needs to be greated than 0
			d.players[i].left_session = (L'\0' == d.players[i].name[0]) && (d.players[i].damage > 0.0);
		}
		return true;
	}

	// try get a single monster's data
	bool get_data_single_monster(const size_t maddr, memory::browser& mb, ui::mhw_data::monster_info& m) {
		const auto	realmaddr = maddr + offsets::Monster::MonsterStartOfStructOffset + offsets::Monster::MonsterHealthComponentOffset;
		size_t		hcompaddr = 0;
		if(!mb.safe_read_mem<size_t>(maddr + offsets::Monster::MonsterHealthComponentOffset, hcompaddr, true))
			return false;
		const auto	id = mb.read_utf8(realmaddr + offsets::MonsterModel::IdOffset + 0x0c, offsets::MonsterModel::IdLength, true);
		const auto	numid = mb.read_mem<uint32_t>(maddr + offsets::Monster::MonsterNumIDOffset, true);
		// according to SmartHunter/HunterPie, we need to split the id string
		// by '\' and the last sub-string the the real monster Id
		const auto	slash_p = id.find_last_of(L"\\");
		const auto	realid = (slash_p == std::wstring::npos) ? id : id.substr(slash_p+1);
		// according to Smarthunter/HunterPie, only if this realid
		// matches "em[0-9]" then the moster is included
		// in current hunt
		const std::wregex IncludeMonsterIdRegex(L"em[0-9].*");
		if(!std::regex_match(realid, IncludeMonsterIdRegex))
			return false;
		m.used = true;
		m.hp_total = mb.read_mem<float>(hcompaddr + offsets::MonsterHealthComponent::MaxHealth, true);
		m.hp_current = mb.read_mem<float>(hcompaddr + offsets::MonsterHealthComponent::CurrentHealth, true);
		const auto size_scale = mb.read_mem<float>(maddr + offsets::Monster::MonsterSizeScale, true);
		auto scale_modifier = mb.read_mem<float>(maddr + offsets::Monster::MonsterScaleModifier, true);
		if(scale_modifier <= 0 || scale_modifier >= 2 ) scale_modifier = 1;
		// if with SmartHunter we should do the lookup based on the
		// string id, with info gotten from HunterPie, it's better
		// to use the numerical id
		const mhw_lookup::monster_data* m_stored_data = get_monster_stored_data(numid);
		if(m_stored_data) {
			m.name = m_stored_data->name;
			const auto modified_size_scale = round(size_scale/scale_modifier*100)/100;
			m.body_size = m_stored_data->base_size * modified_size_scale;
			const mhw_lookup::crown_preset_data* m_crown_preset = get_crown_preset(m_stored_data->crown_preset);
			if (m_crown_preset) {
				if (modified_size_scale <= m_crown_preset->mini)
					m.crown = "Mini";
				else if (modified_size_scale >= m_crown_preset->gold)
					m.crown = "Gold";				
				else if (modified_size_scale >= m_crown_preset->silver)
					m.crown = "Silver";
			}
		}	
		
		return true;
	}

	// try to get all monsters' data
	// Follow the logic at https://github.com/Haato3o/HunterPie/blob/db774b871e39629dc1d4bd58754def4c556701a2/HunterPie/Core/Monsters/Monster.cs
	// Seems to rely less on initial offset, which is harder to
	// maintain on Linux - rely more on jumping through pointers
	// which should be easier to maintain on Linux
	bool get_data_monster(const memory::pattern* monster, memory::browser& mb, ui::mhw_data& d) {
		const auto	mrootptr = mb.load_effective_addr_rel(monster->mem_location, true);
		const uint32_t	mlistlookup[] = { 0x698, 0x0, 0x138, 0x0 };
		size_t		monsters[3] = { 0 };
		if(!mb.safe_load_multilevel_addr_rel(mrootptr, &mlistlookup[0], &mlistlookup[4], monsters[2], true))
			return false;
		// second monster
		if(!mb.safe_read_mem<size_t>(monsters[2] - 0x30, monsters[1], true)) {
			monsters[1] = 0;
		} else {
			monsters[1] += offsets::Monster::MonsterStartOfStructOffset;
		}
		// first monster
		if(!mb.safe_read_mem<size_t>(monsters[1] - offsets::Monster::MonsterStartOfStructOffset + offsets::Monster::PreviousMonsterOffset, monsters[0], true)) {
			monsters[0] = 0;
		} else {
			monsters[0] += offsets::Monster::MonsterStartOfStructOffset;
		}
		static_assert( sizeof(d.monsters)/sizeof(d.monsters[0]) == sizeof(monsters)/sizeof(monsters[0]), "Monsters can only be 3 at any time!");
		uint32_t	cur_monster = 0;
		for(size_t i = 0; i < sizeof(d.monsters)/sizeof(d.monsters[0]); ++i) {
			// Ensure the monster pointer is within a valid
			// memory location - this caters for 0 addresses too
			if(monsters[i] < 0xffffff)
				continue;
			if(!get_data_single_monster(monsters[i], mb, d.monsters[cur_monster])) {
				d.monsters[cur_monster].used = false;
			 } else {
				 ++cur_monster;
			 }
		}
		return true;
	}
}

void mhw_lookup::get_data(const mhw_lookup::pattern_data& pd, memory::browser& mb, ui::mhw_data& d) {
	d = ui::mhw_data();
	if(!get_data_session(pd.player, mb, d))
		return;
	if(get_data_ishunt(pd.lobby, mb) && pd.damage)
		get_data_damage(pd, mb, d);
	if(pd.monster)
		get_data_monster(pd.monster, mb, d);
}

