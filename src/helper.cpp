#include "poncos/poncos.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
