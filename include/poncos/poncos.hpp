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
#include <condition_variable>
#include <string>
#include <vector>

#include "distgen/distgen.h"

struct sched_configT {
	std::vector<unsigned char> cpus;
	std::vector<unsigned char> mems;

	operator distgend_configT() const {
		assert(cpus.size() < DISTGEN_MAXTHREADS);
		distgend_configT ret;
		ret.number_of_threads = cpus.size();
		for (size_t i = 0; i < ret.number_of_threads; ++i) ret.threads_to_use[i] = cpus[i];

		return ret;
	}
};

constexpr size_t SLOTS = 2;
extern const sched_configT co_configs[SLOTS];
extern const distgend_initT distgen_init;

void print_distgen_results();
void read_command_queue(std::string filename, std::vector<std::string> &command_queue);
std::string cgroup_name_from_id(size_t id);

#endif /* end of include guard: pons_hpp */
