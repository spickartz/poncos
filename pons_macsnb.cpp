/**
 * Simple scheduler for our mac-snb nodes.
 *
 * Copyright 2016 by LRR-TUM
 * Jens Breitbart     <j.breitbart@tum.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#include "pons.hpp"
#include "time_measure.hpp"
#include "vendor/distgend/distgend.h"
#include "vendor/ponci/ponci.hpp"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <thread>
#include <vector>

// CONSTANTS
static constexpr size_t MAX_WORKERS = 2;

// TODO merge these two. requires changes to the libraries
static const sched_configT co_configs[MAX_WORKERS] = {{{0, 1}, {0}}, {{6, 7}, {0}}};
static const distgend_configT co_configs_dist[MAX_WORKERS] = {{2, {0, 1}}, {2, {6, 7}}};

// VARIABLES
static size_t cgroups_counter = 0;
static bool co_config_in_use[MAX_WORKERS] = {false, false};
static std::string co_config_cgroup_name[MAX_WORKERS];
static double co_config_distgend[MAX_WORKERS];

static std::vector<std::thread> threads;
static size_t co_config_thread_index[MAX_WORKERS];

static size_t workers_active = 0;
static std::mutex worker_counter_mutex;
static std::condition_variable worker_counter_cv;

static void execute_command_internal(std::string command, std::string cg_name, size_t config_used) {
	cgroup_add_me(cg_name);

	command += " 2>&1 ";
	command += "| tee ";
	command += cg_name + ".log";

	assert(system(command.c_str()) != -1);

	// we are done
	// call the boss
	std::lock_guard<std::mutex> work_counter_lock(worker_counter_mutex);
	--workers_active;
	co_config_in_use[config_used] = false;
	worker_counter_cv.notify_one();
}

static size_t execute_command(std::string command, const std::unique_lock<std::mutex> &work_counter_lock) {
	assert(work_counter_lock.owns_lock());
	++workers_active;

	std::string cg_name = cgroup_name_from_id(cgroups_counter);
	cgroup_create(cg_name);
	++cgroups_counter;

	for (size_t i = 0; i < MAX_WORKERS; ++i) {
		if (!co_config_in_use[i]) {
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
		// wait until workers_active < MAX_WORKERS
		std::unique_lock<std::mutex> work_counter_lock(worker_counter_mutex);
		worker_counter_cv.wait(work_counter_lock, [] { return workers_active < MAX_WORKERS; });

		const size_t new_config = execute_command(command, work_counter_lock);
		const size_t old_config = (new_config + 1) % MAX_WORKERS;

		// check if two are running
		if (co_config_in_use[0] && co_config_in_use[1]) {
			std::cout << "0: freezing old" << std::endl;
			cgroup_freeze(co_config_cgroup_name[old_config]);
		}

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(5s);

		// TODO wait until cgroup frozen

		// measure distgen result
		co_config_distgend[new_config] = distgend_is_membound(co_configs_dist[old_config]);
		std::cout << "distgend: " << co_config_distgend[new_config] << std::endl;

		if (co_config_in_use[0] && co_config_in_use[1]) {
			std::cout << "0: thaw old" << std::endl;
			cgroup_thaw(co_config_cgroup_name[old_config]);

			if (co_config_distgend[0] + co_config_distgend[1] < 1.0) {
				std::cout << "0: freezing new" << std::endl;
				cgroup_freeze(co_config_cgroup_name[new_config]);
				work_counter_lock.unlock();

				std::cout << "0: wait for old" << std::endl;
				threads[co_config_thread_index[old_config]].join();

				std::cout << "0: thaw new" << std::endl;
				cgroup_thaw(co_config_cgroup_name[new_config]);
			}
		}
	}
}

int main(int argc, char const *argv[]) {

	if (argc != 2) {
		std::cout << "Usage: pons_macsnb <config file>" << std::endl;
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

	distgend_initT distgen_init;
	distgen_init.SMT_factor = 2;
	distgen_init.NUMA_domains = 2;
	distgen_init.number_of_threads = 32;

	std::cout << "Starting distgen initialization ...";
	std::cout.flush();
	distgend_init(distgen_init);
	std::cout << " done!" << std::endl << std::endl;

	// TODO measure time
	std::cout << "total runtime: " << time_measure<>::execute(coschedule_queue, command_queue) << " ms" << std::endl;
	// TODO add consecutive execution

	// wait until all workers are finished before deleting the cgroup
	std::unique_lock<std::mutex> work_counter_lock(worker_counter_mutex);
	worker_counter_cv.wait(work_counter_lock, [] { return workers_active == 0; });

	for (auto &t : threads) {
		if (t.joinable()) t.join();
	}

	for (size_t i = 0; i < cgroups_counter; ++i) {
		cgroup_delete(cgroup_name_from_id(i));
	}
}
