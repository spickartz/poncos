/**
 * Simple tool to classify applications.
 *
 * Copyright 2016 by LRR-TUM
 * Jens Breitbart     <j.breitbart@tum.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#include "distgen/distgen.h"
#include "ponci/ponci.hpp"
#include "poncos/poncos.hpp"
#include "poncos/time_measure.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <signal.h>

// VARIABLES
static size_t cgroups_counter = 0;

static std::vector<std::thread> threads;

static std::vector<double> dg_res;

// catch this signal, because the execute_command_internal thread will recieve SIG_TERM
static void kill_handler(int) {}

static void execute_command_internal(std::string command, std::string cg_name) {
	cgroup_add_me(cg_name);

	command += " 2>&1 ";
	// command += "| tee ";
	command += "> ";
	command += cg_name + ".log";

	system(command.c_str());

	// we are done
	std::cout << ">> \t '" << command << "' completed at configuration " << 0 << std::endl;
}

static void execute_command(std::string command) {
	std::string cg_name = cgroup_name_from_id(cgroups_counter);
	cgroup_create(cg_name);
	++cgroups_counter;

	std::cout << ">> \t starting '" << command << "' at configuration " << 0 << std::endl;
	cgroup_set_cpus(cg_name, co_configs[0].cpus);
	cgroup_set_mems(cg_name, co_configs[0].mems);

	threads.emplace_back(execute_command_internal, command, cg_name);
}

static void coschedule_queue(const std::vector<std::string> &command_queue) {
	for (auto command : command_queue) {
		if (command == "") continue;

		execute_command(command);

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(10s);

		// measure distgen result
		std::cout << ">> \t Running distgend at " << 1 << std::endl;
		const double temp = distgend_is_membound(co_configs[1]);
		std::cout << ">> \t Result " << temp << std::endl;

		dg_res.push_back(temp);

		auto name = cgroup_name_from_id(cgroups_counter - 1);

		std::cout << ">> \t Killing cgroup " << name << std::endl;
		cgroup_kill(name.c_str());
	}
}

static void cleanup() {
	for (auto &t : threads) {
		if (t.joinable()) t.join();
	}
	threads.resize(0);
}

int main(int argc, char const *argv[]) {
	assert(signal(SIGTERM, kill_handler) != SIG_ERR);
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <config file>" << std::endl;
		return 0;
	}

	assert(argc == 2);
	std::string queue_filename(argv[1]);

	// fill the command qeue
	std::cout << "Reading command queue " << queue_filename << " ...";
	std::cout.flush();
	std::vector<std::string> command_queue;
	read_command_queue(queue_filename, command_queue);
	std::cout << " done!" << std::endl;

	std::cout << "Starting distgen initialization ...";
	std::cout.flush();
	distgend_init(distgen_init);
	std::cout << " done!" << std::endl << std::endl;

	print_distgen_results();

	coschedule_queue(command_queue);

	assert(dg_res.size() == command_queue.size());

	std::cout << std::endl << std::endl;

	std::cout << "measurments,";
	for (size_t i = 0; i < command_queue.size(); ++i) {
		const std::size_t found = command_queue[i].find_last_of("/");
		std::cout << command_queue[i].substr(found + 1) << ",";
	}
	std::cout << std::endl;
	std::cout << ",";
	for (double d : dg_res) {
		std::cout << d << ",";
	}

	std::cout << std::endl << std::endl;

	std::cout << "predict,";
	for (size_t i = 0; i < command_queue.size(); ++i) {
		const std::size_t found = command_queue[i].find_last_of("/");
		std::cout << command_queue[i].substr(found + 1) << ",";
	}
	std::cout << std::endl;

	for (size_t i = 0; i < command_queue.size(); ++i) {
		const std::size_t found = command_queue[i].find_last_of("/");
		std::cout << command_queue[i].substr(found + 1) << ",";
		for (size_t j = 0; j < command_queue.size(); ++j) {
			std::cout << (1 - dg_res[i]) + (1 - dg_res[j]) << ",";
		}
		std::cout << std::endl;
	}

	cleanup();
}
