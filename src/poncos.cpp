/**
 * Simple scheduler.
 *
 * Copyright 2016 by LRR-TUM
 * Jens Breitbart     <j.breitbart@tum.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#include "poncos/poncos.hpp"
#include "distgen/distgen.h"
#include "ponci/ponci.hpp"
#include "poncos/time_measure.hpp"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// VARIABLES
static size_t cgroups_counter = 0;
static bool co_config_in_use[SLOTS] = {false, false};
static std::string co_config_cgroup_name[SLOTS];
static double co_config_distgend[SLOTS];

static std::vector<std::thread> threads;
static size_t co_config_thread_index[SLOTS];

static size_t workers_active = 0;
static std::mutex worker_counter_mutex;
static std::condition_variable worker_counter_cv;

static void command_done(const size_t config) {
	std::lock_guard<std::mutex> work_counter_lock(worker_counter_mutex);
	--workers_active;
	co_config_in_use[config] = false;
	co_config_distgend[config] = 0;
	worker_counter_cv.notify_one();
}

void execute_command_internal(std::string command, std::string cg_name, size_t config_used) {
	cgroup_add_me(cg_name);

	command += " 2>&1 ";
	// command += "| tee ";
	command += "> ";
	command += cg_name + ".log";

	auto temp = system(command.c_str());
	assert(temp != -1);

	// we are done
	std::cout << ">> \t '" << command << "' completed at configuration " << config_used << std::endl;
	command_done(config_used);
}

static size_t execute_command(std::string command, const std::unique_lock<std::mutex> &work_counter_lock) {
	assert(work_counter_lock.owns_lock());
	++workers_active;

	std::string cg_name = cgroup_name_from_id(cgroups_counter);
	cgroup_create(cg_name);
	++cgroups_counter;

	std::cout << ">> \t starting '" << command << "' at configuration ";

	for (size_t i = 0; i < SLOTS; ++i) {
		if (!co_config_in_use[i]) {
			std::cout << i << std::endl;
			co_config_in_use[i] = true;
			co_config_cgroup_name[i] = cg_name;
			cgroup_set_cpus(cg_name, co_configs[i].cpus);
			cgroup_set_mems(cg_name, co_configs[i].mems);
			co_config_thread_index[i] = threads.size();

			threads.emplace_back(execute_command_internal, command, cg_name, i);

			return i;
		}
	}

	assert(false);
}

static void coschedule_queue(const std::vector<std::string> &command_queue) {
	for (auto command : command_queue) {
		// wait until workers_active < SLOTS
		std::unique_lock<std::mutex> work_counter_lock(worker_counter_mutex);
		worker_counter_cv.wait(work_counter_lock, [] { return workers_active < SLOTS; });

		const size_t new_config = execute_command(command, work_counter_lock);
		const size_t old_config = (new_config + 1) % SLOTS;

		// check if two are running
		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: freezing old" << std::endl;
			cgroup_freeze(co_config_cgroup_name[old_config]);
		}

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(10s);

		// TODO wait until cgroup frozen

		// measure distgen result
		std::cout << ">> \t Running distgend at " << old_config << std::endl;
		co_config_distgend[new_config] = distgend_is_membound(co_configs[old_config]);

		std::cout << ">> \t Result " << co_config_distgend[new_config] << std::endl;

		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: thaw old" << std::endl;
			cgroup_thaw(co_config_cgroup_name[old_config]);

			std::cout << ">> \t Estimating total usage of "
					  << (1 - co_config_distgend[0]) + (1 - co_config_distgend[1]);

			if ((1 - co_config_distgend[0]) + (1 - co_config_distgend[1]) > 0.9) {
				std::cout << " -> we will run one" << std::endl;
				// std::cout << "0: freezing new" << std::endl;
				cgroup_freeze(co_config_cgroup_name[new_config]);
				work_counter_lock.unlock();

				// std::cout << "0: wait for old" << std::endl;
				threads[co_config_thread_index[old_config]].join();

				// std::cout << "0: thaw new" << std::endl;
				cgroup_thaw(co_config_cgroup_name[new_config]);
			} else {
				std::cout << " -> we will run both applications" << std::endl;
			}

		} else {
			std::cout << ">> \t Just one config in use ATM" << std::endl;
		}
	}

	// wait until all workers are finished before deleting the cgroup
	std::unique_lock<std::mutex> work_counter_lock(worker_counter_mutex);
	worker_counter_cv.wait(work_counter_lock, [] { return workers_active == 0; });
}

static void cleanup() {
	for (auto &t : threads) {
		if (t.joinable()) t.join();
	}
	threads.resize(0);

	for (size_t i = 0; i < cgroups_counter; ++i) {
		cgroup_delete(cgroup_name_from_id(i));
	}
}

int main(int argc, char const *argv[]) {
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

	const auto runtime = time_measure<>::execute(coschedule_queue, command_queue);
	std::cout << "total runtime: " << runtime << " ms" << std::endl;

	cleanup();
}
