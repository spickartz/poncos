#include "poncos/poncos.hpp"

// We currently assume the slots have identical size!

// This defines the slots used fot scheduling.
// Currently there is a fixed number of two slots.
const sched_configT co_configs[SLOTS] = {
	// cpus used in slot 0
	{{0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17},
	 // numa memory domains used in slot 0
	 {0, 1}},
	// cpus used in slot 1
	{{6, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23},
	 // numa memory domains used in slot 1
	 {0, 1}}};
