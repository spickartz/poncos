#include "poncos/poncos.hpp"

// This defines the slots used fot scheduling.
// Currently there is a fixed number of two slots.
const sched_configT co_configs[SLOTS] = {
	// cpus used in slot 0
	{{0, 1, 2, 3, 8, 9, 10, 11},
	 // numa memory domains used in slot 0
	 {0, 1}},
	// cpus used in slot 1
	{{4, 5, 6, 7, 12, 13, 14, 15},
	 // numa memory domains used in slot 1
	 {0, 1}}};
