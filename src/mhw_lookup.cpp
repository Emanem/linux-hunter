#include "mhw_lookup.h"
#include "mhw_lookup_monster.h"
#include "offsets.h"
#include <regex>
#include <algorithm>
#include <cwchar>

namespace {
	bool sort_MONSTERS(void) {
		// apparently this structure can be modified
		// at runtime, is not written in the R/O
		// location of the executable
		std::sort(&mhw_lookup::MONSTERS[0], &mhw_lookup::MONSTERS[sizeof(mhw_lookup::MONSTERS)/sizeof(mhw_lookup::monster_data)],
				[](const mhw_lookup::monster_data& lhs, const mhw_lookup::monster_data& rhs) -> bool {
					return std::wcscmp(lhs.id, rhs.id) < 0;
				});
		return true;
	}

	// this is to ensure that monster data is properly sorted
	const bool	sorted_MONSTERS = sort_MONSTERS();

	const char* get_monster_name(const wchar_t* id) {
		// use binary search via lower_bound
		const auto	*first = &mhw_lookup::MONSTERS[0],
				*last = &mhw_lookup::MONSTERS[sizeof(mhw_lookup::MONSTERS)/sizeof(mhw_lookup::monster_data)];
		const auto	it = std::lower_bound(first, last, id,
				[](const mhw_lookup::monster_data& lhs, const wchar_t* rhs) -> bool {
					return std::wcscmp(lhs.id, rhs) < 0;
				});

		if(it != last && !std::wcscmp(it->id, id))
			return it->name;

		return "<N/A>";
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

	// try get a single monster's data - currently doesn't output anything
	bool get_data_single_monster(const size_t maddr, memory::browser& mb, ui::mhw_data::monster_info& m) {
		const auto	realmaddr = maddr + offsets::Monster::MonsterStartOfStructOffset + offsets::Monster::MonsterHealthComponentOffset;
		const auto	hcompaddr = mb.read_mem<size_t>(realmaddr, true);
		const auto	id = mb.read_utf8(realmaddr + offsets::MonsterModel::IdOffset, offsets::MonsterModel::IdLength, true);
		//std::wcout << "maddr: " << maddr << "\t[" << id << "]" << std::endl;
		// according to SmartHunter, we need ot split the id string
		// by '\' and the last sub-string the the real monster Id
		const auto	slash_p = id.find_last_of(L"\\");
		const auto	realid = (slash_p == std::wstring::npos) ? id : id.substr(slash_p+1);
		// according to Smarthunter, only if this realid
		// matches "em[0-9]" then the moster is included
		// in current hunt
		const std::wregex IncludeMonsterIdRegex(L"em[0-9].*");
		if(!std::regex_match(realid, IncludeMonsterIdRegex))
			return false;
		m.used = true;
		m.name = get_monster_name(realid.c_str());
		m.hp_total = mb.read_mem<float>(hcompaddr + offsets::MonsterHealthComponent::MaxHealth, true);
		m.hp_current = mb.read_mem<float>(hcompaddr + offsets::MonsterHealthComponent::CurrentHealth, true);
		return true;
	}

	// try to get all monsters' data
	bool get_data_monster(const memory::pattern* monster, memory::browser& mb, ui::mhw_data& d) {
		const auto	mrootptr = mb.load_effective_addr_rel(monster->mem_location, true) - 0x36CE0;
		const uint32_t	mlistlookup[] = { 0x128, 0x8, 0x0 };
		size_t		mbaselist = 0;
		if(!mb.safe_load_multilevel_addr_rel(mrootptr, &mlistlookup[0], &mlistlookup[3], mbaselist, true))
			return false;
		// this also caters for mbaselist being 0
		if(mbaselist < 0xffffff)
			return false;
		auto		firstmonster = mb.read_mem<size_t>(mbaselist + offsets::Monster::PreviousMonsterOffset, true);
		if(!firstmonster)
			firstmonster = mbaselist;
		firstmonster += offsets::Monster::MonsterStartOfStructOffset;
		// maximum of 512 monsters
		// should be more than enough
#define	MAX_MONSTERS (512)
		size_t		monstersaddr[MAX_MONSTERS] = {0};
		uint32_t	lastmonster = 0;
		while(firstmonster) {
			monstersaddr[lastmonster++] = firstmonster;
			firstmonster = mb.read_mem<size_t>(firstmonster + offsets::Monster::NextMonsterOffset, true);
			if(lastmonster >= MAX_MONSTERS)
				break;
		}
		const uint32_t	max_out_monsters = sizeof(d.monsters)/sizeof(d.monsters[0]);
		uint32_t	cur_out_monster = 0;
		for(uint32_t i = 0; i < lastmonster; ++i) {
			if(get_data_single_monster(monstersaddr[i], mb, d.monsters[cur_out_monster]))
				++cur_out_monster;
			if(cur_out_monster >= max_out_monsters)
				break;
		}
#undef	MAX_MONSTERS
		return true;
	}
}

void mhw_lookup::get_data(const mhw_lookup::pattern_data& pd, memory::browser& mb, ui::mhw_data& d) {
	d = ui::mhw_data();
	if(!get_data_session(pd.player, mb, d))
		return;
	if(pd.damage)
		get_data_damage(pd, mb, d);
	if(pd.monster)
		get_data_monster(pd.monster, mb, d);
}

