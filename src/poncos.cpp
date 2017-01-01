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
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "poncos/cgroup_controller.hpp"
#include "poncos/poncos.hpp"
#include "poncos/time_measure.hpp"

#include <fast-lib/message/agent/mmbwmon/ack.hpp>
#include <fast-lib/message/agent/mmbwmon/reply.hpp>
#include <fast-lib/message/agent/mmbwmon/request.hpp>
#include <fast-lib/mqtt_communicator.hpp>

// COMMAND LINE PARAMETERS
static std::string server;
static size_t port = 1883;
static std::string queue_filename;
static std::string machine_filename;

// marker if a slot is in use
static bool co_config_in_use[SLOTS] = {false, false};

// distgen results of a slot
static double co_config_distgend[SLOTS];

// id of the job currently running at SLOT
static size_t co_config_id[SLOTS] = {42, 42};

[[noreturn]] static void print_help(const char *argv) {
	std::cout << argv << " supports the following flags:\n";
	std::cout << "\t --server \t URI of the MQTT broker. \t\t\t Required!\n";
	std::cout << "\t --port \t Port of the MQTT broker. \t\t\t Default: 1883\n";
	std::cout << "\t --queue \t\t Filename for the job queue. \t\t\t Required!\n";
	std::cout << "\t --machine \t\t Filename containing node names. \t\t\t Required!\n";
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

// called after a command was completed
void command_done(const size_t config) {
	co_config_in_use[config] = false;
	co_config_distgend[config] = 0;
}

static double run_distgen(fast::MQTT_communicator &comm, size_t slot, const std::vector<std::string> &machines) {

	// ask for measurements
	{
		fast::msg::agent::mmbwmon::request m;

		const auto &conf = co_configs[slot];

		// TODO check if we can use the same type
		m.cores.resize(conf.cpus.size());
		for (int i = 0; i < conf.cpus.size(); ++i) {
			m.cores[i] = static_cast<size_t>(conf.cpus[i]);
		}

		for (std::string mach : machines) {
			const std::string topic = "fast/agent/" + mach + "/mmbwmon/request";
			// std::cout << "sending message \n topic: " << topic << "\n message:\n" << m.to_string() << std::endl;
			comm.send_message(m.to_string(), topic);
		}
	}

	double ret = 0.0;

	// wait for results
	{
		for (std::string mach : machines) {
			fast::msg::agent::mmbwmon::reply m;
			const std::string topic = "fast/agent/" + mach + "/mmbwmon/response";
			// std::cout << "waiting on topic: " << topic << " ... " << std::flush;
			m.from_string(comm.get_message(topic));
			// std::cout << "done\n";

			if (m.result > ret) ret = m.result;
		}
	}

	return ret;
}

static void coschedule_queue(const std::vector<std::string> &command_queue, fast::MQTT_communicator &comm,
							 cgroup_controller &controller) {
	// for all commands
	for (auto command : command_queue) {
		controller.wait_for_ressource();

		// search for a free slot and assign it to a new job
		size_t new_slot = 0;
		for (; new_slot < SLOTS; ++new_slot) {
			if (!co_config_in_use[new_slot]) {
				co_config_in_use[new_slot] = true;
				// co_config_cgroup_name[i] = cg_name;

				cgroup_controller::execute_config config;

				for (size_t j = 0; j < controller.machines.size(); ++j) {
					config.emplace_back(j, new_slot);
				}

				co_config_id[new_slot] = controller.execute(command, config, command_done);

				std::cout << ">> \t starting '" << command << "' at configuration " << new_slot << std::endl;

				break;
			}
		}
		assert(new_slot < SLOTS);

		// for the initialization phase of the application to be completed
		std::this_thread::sleep_for(std::chrono::seconds(20));

		// old config is used to run distgen
		const size_t old_slot = (new_slot + 1) % SLOTS;

		// check if two are running
		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: freezing old" << std::endl;
			controller.freeze(co_config_id[old_slot]);
		}

		// measure distgen result
		std::cout << ">> \t Running distgend at " << old_slot << std::endl;
		co_config_distgend[new_slot] = run_distgen(comm, old_slot, controller.machines);

		std::cout << ">> \t Result for command '" << command << "' is: " << 1 - co_config_distgend[new_slot]
				  << std::endl;

		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: thaw old" << std::endl;
			controller.thaw(co_config_id[old_slot]);

			std::cout << ">> \t Estimating total usage of "
					  << (1 - co_config_distgend[0]) + (1 - co_config_distgend[1]);

			if ((1 - co_config_distgend[0]) + (1 - co_config_distgend[1]) > 0.9) {
				std::cout << " -> we will run one" << std::endl;
				// std::cout << "0: freezing new" << std::endl;
				controller.freeze(co_config_id[new_slot]);

				controller.wait_for_completion_of(co_config_id[old_slot]);

				// std::cout << "0: thaw new" << std::endl;
				controller.thaw(co_config_id[new_slot]);
			} else {
				std::cout << " -> we will run both applications" << std::endl;
			}

		} else {
			std::cout << ">> \t Just one config in use ATM" << std::endl;
		}
	}
	controller.done();
}

int main(int argc, char const *argv[]) {
	parse_options(static_cast<size_t>(argc), argv);

	// fill the command qeue
	std::cout << "Reading command queue " << queue_filename << " ...";
	std::cout.flush();
	std::vector<std::string> command_queue;
	read_file(queue_filename, command_queue);
	std::cout << " done!" << std::endl;

	std::cout << "Command queue:\n";
	std::cout << "==============\n";
	for (std::string c : command_queue) {
		std::cout << c << "\n";
	}
	std::cout << "==============\n";

	auto comm = std::make_shared<fast::MQTT_communicator>("fast/poncos", "fast/poncos", "fast/poncos", server,
														  static_cast<int>(port), 60);

	cgroup_controller controller(comm, machine_filename);

	// subscribe to the various topics
	for (std::string mach : controller.machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/response";
		comm->add_subscription(topic);
	}

	std::cout << "MQTT ready!\n\n";

	const auto runtime = time_measure<>::execute(coschedule_queue, command_queue, *comm, controller);
	std::cout << "total runtime: " << runtime << " ms" << std::endl;
}
