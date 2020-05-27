#ifndef _MHW_LOOKUP_
#define _MHW_LOOKUP_

#include "memory.h"
#include "ui.h"

namespace mhw_lookup {

	struct pattern_data {
		const	memory::pattern	*player,
					*damage;
	};
	
	extern void get_data(const pattern_data& pd, memory::browser& mb, ui::mhw_data& d);
}

#endif //_MHW_LOOKUP_

