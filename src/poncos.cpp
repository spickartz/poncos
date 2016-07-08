/**
 * Simple scheduler.
 *
 * Copyright 2016 by LRR-TUM
 * Jens Breitbart     <j.breitbart@tum.de>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "poncos/poncos.hpp"
#include "poncos/time_measure.hpp"

#include <fast-lib/message/agent/mmbwmon/ack.hpp>
#include <fast-lib/message/agent/mmbwmon/reply.hpp>
#include <fast-lib/message/agent/mmbwmon/request.hpp>
#include <fast-lib/message/agent/mmbwmon/restart.hpp>
#include <fast-lib/message/agent/mmbwmon/stop.hpp>
#include <fast-lib/mqtt_communicator.hpp>

// MQTT stuff
const static std::string poncosID = "fast/poncos";
const static std::string baseTopic = "fast/poncos";
static std::string server;
static size_t port = 1883;

static std::string queue_filename;
static std::string machine_filename;

static std::vector<std::string> machines;

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

[[noreturn]] static void print_help(const char *argv) {
	std::cout << argv << " supports the following flags:\n";
	std::cout << "\t --server \t URI of the MQTT broker. \t\t\t Required!\n";
	std::cout << "\t --port \t Port of the MQTT broker. \t\t\t Default: 1883\n";
	std::cout << "\t --queue \t\t Filename for the job queue.\n";
	std::cout << "\t --machine \t\t Filename containing node names.\n";
	exit(0);
}

static void parse_options(size_t argc, const char **argv) {
	if (argc == 1) {
		print_help(argv[0]);
	}
	for (size_t i = 1; i < argc; ++i) {
		std::string arg(argv[i]);

		if (arg == "--server") {
			if (i + 1 >= argc) {
				print_help(argv[0]);
			}
			server = std::string(argv[i + 1]);
			++i;
			continue;
		}
		if (arg == "--port") {
			if (i + 1 >= argc) {
				print_help(argv[0]);
			}
			port = std::stoul(std::string(argv[i + 1]));
			++i;
			continue;
		}

		if (arg == "--queue") {
			if (i + 1 >= argc) {
				print_help(argv[0]);
			}
			queue_filename = std::string(argv[i + 1]);
			++i;
			continue;
		}
		if (arg == "--machine") {
			if (i + 1 >= argc) {
				print_help(argv[0]);
			}
			machine_filename = std::string(argv[i + 1]);
			++i;
			continue;
		}
	}

	if (queue_filename == "" || machine_filename == "") print_help(argv[0]);
}

static void command_done(const size_t config) {
	std::lock_guard<std::mutex> work_counter_lock(worker_counter_mutex);
	--workers_active;
	co_config_in_use[config] = false;
	co_config_distgend[config] = 0;
	worker_counter_cv.notify_one();
}

void execute_command_internal(std::string command, std::string cg_name, size_t config_used) {
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
	// cgroup is created by the bash script
	++cgroups_counter;

	// command input: mpirun -np X PONCOS command p0 p1
	// run instead  : mpirun -np X -hosts a,b cgroup_wrapper.sh 0,1,2,3,8,9,10,11 0,1 command p0 p1

	std::string replace = "-hosts ";
	for (std::string mach : machines) {
		replace += mach + ",";
	}
	// remove last ','
	replace.pop_back();

	replace += " ./cgroup_wrapper.sh ";

	for (size_t i = 0; i < SLOTS; ++i) {
		if (!co_config_in_use[i]) {
			co_config_in_use[i] = true;
			co_config_cgroup_name[i] = cg_name;
			co_config_thread_index[i] = threads.size();

			replace += cg_name + " ";

			// cgroup CPUs and memory is set by the bash script
			for (int i : co_configs[i].cpus) {
				replace += std::to_string(i) + ",";
			}
			// remove last ','
			replace.pop_back();

			replace += " ";

			for (int i : co_configs[i].mems) {
				replace += std::to_string(i) + ",";
			}
			// remove last ','
			replace.pop_back();

			command.replace(command.find("PONCOS"), std::string("PONCOS").size(), replace);

			std::cout << ">> \t starting '" << command << "' at configuration " << i << std::endl;

			threads.emplace_back(execute_command_internal, command, cg_name, i);

			return i;
		}
	}

	assert(false);
}

static void freeze_remote_cgroup(fast::MQTT_communicator &comm, std::string cgroup_name) {
	const fast::msg::agent::mmbwmon::stop m(cgroup_name);
	for (std::string mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/stop";
		// std::cout << "sending message \n topic: " << topic << "\n message:\n" << m.to_string() << std::endl;
		comm.send_message(m.to_string(), topic);
	}

	for (std::string mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/stop/ack";
		// std::cout << "waiting on topic: " << topic << " ... " << std::flush;
		comm.get_message(topic);
		// std::cout << "done\n";
	}
}

static void thaw_remote_cgroup(fast::MQTT_communicator &comm, std::string cgroup_name) {
	const fast::msg::agent::mmbwmon::restart m(cgroup_name);
	for (std::string mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/restart";
		// std::cout << "sending message \n topic: " << topic << "\n message:\n" << m.to_string() << std::endl;
		comm.send_message(m.to_string(), topic);
	}

	for (std::string mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/restart/ack";
		// std::cout << "waiting on topic: " << topic << " ... " << std::flush;
		comm.get_message(topic);
		// std::cout << "done\n";
	}
}

static double run_distgen(fast::MQTT_communicator &comm, sched_configT conf) {

	// ask for measurements
	{
		fast::msg::agent::mmbwmon::request m;

		// TODO check if we can use the same type
		m.cores.resize(conf.cpus.size());
		for (int i = 0; i < conf.cpus.size(); ++i) {
			m.cores[i] = static_cast<size_t>(conf.cpus[i]);
		}

		for (std::string mach : machines) {
			std::string topic = "fast/agent/" + mach + "/mmbwmon/request";
			// std::cout << "sending message \n topic: " << topic << "\n message:\n" << m.to_string() << std::endl;
			comm.send_message(m.to_string(), topic);
		}
	}

	double ret = 0.0;

	// wait for results
	{
		for (std::string mach : machines) {
			fast::msg::agent::mmbwmon::reply m;
			std::string topic = "fast/agent/" + mach + "/mmbwmon/response";
			// std::cout << "waiting on topic: " << topic << " ... " << std::flush;
			m.from_string(comm.get_message(topic));
			// std::cout << "done\n";

			if (m.result > ret) ret = m.result;
		}
	}

	return ret;
}

static void coschedule_queue(const std::vector<std::string> &command_queue, fast::MQTT_communicator &comm) {
	for (auto command : command_queue) {
		// wait until workers_active < SLOTS
		std::unique_lock<std::mutex> work_counter_lock(worker_counter_mutex);
		worker_counter_cv.wait(work_counter_lock, [] { return workers_active < SLOTS; });

		const size_t new_config = execute_command(command, work_counter_lock);
		const size_t old_config = (new_config + 1) % SLOTS;

		// check if two are running
		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: freezing old" << std::endl;
			freeze_remote_cgroup(comm, co_config_cgroup_name[old_config]);
		}

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(10s);

		// measure distgen result
		std::cout << ">> \t Running distgend at " << old_config << std::endl;
		co_config_distgend[new_config] = run_distgen(comm, co_configs[old_config]);

		std::cout << ">> \t Result " << co_config_distgend[new_config] << std::endl;

		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: thaw old" << std::endl;
			thaw_remote_cgroup(comm, co_config_cgroup_name[old_config]);

			std::cout << ">> \t Estimating total usage of " << co_config_distgend[0] + co_config_distgend[1];

			// if ((1 - co_config_distgend[0]) + (1 - co_config_distgend[1]) > 0.9) {
			if (co_config_distgend[0] + co_config_distgend[1] > 0.9) {
				std::cout << " -> we will run one" << std::endl;
				// std::cout << "0: freezing new" << std::endl;
				freeze_remote_cgroup(comm, co_config_cgroup_name[new_config]);
				work_counter_lock.unlock();

				// std::cout << "0: wait for old" << std::endl;
				threads[co_config_thread_index[old_config]].join();

				// std::cout << "0: thaw new" << std::endl;
				thaw_remote_cgroup(comm, co_config_cgroup_name[new_config]);
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

	// No need to delete cgroups. Should be done automatically
	// by our bash script.
}

int main(int argc, char const *argv[]) {
	parse_options(static_cast<size_t>(argc), argv);

	// fill the command qeue
	std::cout << "Reading command queue " << queue_filename << " ...";
	std::cout.flush();
	std::vector<std::string> command_queue;
	read_command_queue(queue_filename, command_queue);
	std::cout << " done!" << std::endl;

	// fill the machine file
	std::cout << "Reading command queue " << machine_filename << " ...";
	std::cout.flush();
	read_command_queue(machine_filename, machines);
	std::cout << " done!" << std::endl;

	fast::MQTT_communicator comm(poncosID, baseTopic, baseTopic, server, static_cast<int>(port), 60);

	for (std::string mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/response";
		comm.add_subscription(topic);
		topic = "fast/agent/" + mach + "/mmbwmon/restart/ack";
		comm.add_subscription(topic);
		topic = "fast/agent/" + mach + "/mmbwmon/stop/ack";
		comm.add_subscription(topic);
	}

	std::cout << "MQTT ready!\n\n";

	const auto runtime = time_measure<>::execute(coschedule_queue, command_queue, comm);
	std::cout << "total runtime: " << runtime << " ms" << std::endl;

	cleanup();
}
