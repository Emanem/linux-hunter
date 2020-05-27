#include "mhw_lookup.h"
#include "offsets.h"

namespace {
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

}

void mhw_lookup::get_data(const mhw_lookup::pattern_data& pd, memory::browser& mb, ui::mhw_data& d) {
	d = ui::mhw_data();
	if(!get_data_session(pd.player, mb, d))
		return;
	if(!get_data_damage(pd, mb, d))
		return;
}

