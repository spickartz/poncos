#include "distgen/distgen.h"
#include "poncos/poncos.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

void print_distgen_results() {
	assert(distgen_init.number_of_threads / distgen_init.SMT_factor - 1 < DISTGEN_MAXTHREADS);
	distgend_configT config;
	for (unsigned char i = 0; i < distgen_init.number_of_threads / distgen_init.SMT_factor; ++i) {
		config.number_of_threads = i + 1;
		config.threads_to_use[i] = i;
		std::cout << "Using " << i + 1 << " threads:" << std::endl;
		std::cout << "\tMaximum: " << distgend_get_max_bandwidth(config) << " GByte/s" << std::endl;
		std::cout << std::endl;
	}
}

void read_command_queue(std::string filename, std::vector<std::string> &command_queue) {
	std::ifstream file(filename);
	assert(file.good());
	std::string command;
	while (std::getline(file, command)) {
		assert(file.good());
		command_queue.push_back(command);
	}
}

std::string cgroup_name_from_id(size_t id) {
	std::string cg_name("pons_");
	cg_name += std::to_string(id);
	return cg_name;
}
