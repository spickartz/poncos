/**
 * Poor mans scheduler
 *
 * Copyright 2016 by LRR-TUM
 * Jens Breitbart     <j.breitbart@tum.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#ifndef pons_hpp
#define pons_hpp

#include <cassert>
#include <fstream>
#include <string>
#include <vector>

struct sched_configT {
	std::vector<size_t> cpus;
	std::vector<size_t> mems;
};

inline void read_command_queue(std::string filename, std::vector<std::string> &command_queue) {
	std::ifstream file(filename);
	assert(file.good());
	std::string command;
	while (std::getline(file, command)) {
		assert(file.good());
		command_queue.push_back(command);
	}
}

inline std::string cgroup_name_from_id(size_t id) {
	std::string cg_name("pons_");
	cg_name += std::to_string(id);
	return cg_name;
}

#endif /* end of include guard: pons_hpp */
