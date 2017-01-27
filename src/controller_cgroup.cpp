#include "poncos/controller_cgroup.hpp"

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>

#include "poncos/poncos.hpp"

#include <fast-lib/message/agent/mmbwmon/restart.hpp>
#include <fast-lib/message/agent/mmbwmon/stop.hpp>

cgroup_controller::cgroup_controller(const std::shared_ptr<fast::MQTT_communicator> &_comm,
									 const std::string &machine_filename)
	: controllerT(_comm, machine_filename) {
	// subscribe to the various topics
	for (const std::string &mach : machines) {
		std::string topic = "fast/agent/" + mach + "/mmbwmon/restart/ack";
		comm->add_subscription(topic);
		topic = "fast/agent/" + mach + "/mmbwmon/stop/ack";
		comm->add_subscription(topic);
	}
}

void cgroup_controller::init() {}
void cgroup_controller::dismantle() {}

cgroup_controller::~cgroup_controller() = default;

void cgroup_controller::freeze(const size_t id) {
	assert(id < id_to_config.size());

	const execute_config &config = id_to_config[id];

	send_message<fast::msg::agent::mmbwmon::stop>(config, "/mmbwmon/stop");
}

void cgroup_controller::thaw(const size_t id) {
	assert(id < id_to_config.size());

	const execute_config &config = id_to_config[id];

	send_message<fast::msg::agent::mmbwmon::restart>(config, "/mmbwmon/restart");
}

void cgroup_controller::freeze_opposing(const size_t id) {
	// TODO probably broken if there is no opposing job!
	const execute_config opposing_config = generate_opposing_config(id);

	send_message<fast::msg::agent::mmbwmon::stop>(opposing_config, "/mmbwmon/stop");
}

void cgroup_controller::thaw_opposing(const size_t id) {
	// TODO probably broken if there is no opposing job!
	const execute_config opposing_config = generate_opposing_config(id);

	send_message<fast::msg::agent::mmbwmon::restart>(opposing_config, "/mmbwmon/restart");
}

template <typename messageT>
void cgroup_controller::send_message(const controllerT::execute_config &config, const std::string &topic_adn) const {
	for (auto config_elem : config) {
		const size_t id = machine_usage[config_elem.first][config_elem.second];
		const messageT m(cmd_name_from_id(id));
		const std::string topic = "fast/agent/" + machines[config_elem.first] + topic_adn;

		comm->send_message(m.to_string(), topic);
	}

	for (auto config_elem : config) {
		const std::string topic = "fast/agent/" + machines[config_elem.first] + topic_adn + "/ack";

		comm->get_message(topic);
	}
}

void cgroup_controller::update_config(const size_t /*id*/, const execute_config & /*new_config*/) { assert(false); }

controllerT::execute_config cgroup_controller::sort_config_by_hostname(const execute_config &config) const {
	execute_config sorted_config = config;
	std::sort(sorted_config.begin(), sorted_config.end(),
			  [](const auto &elem_a, const auto &elem_b) { return elem_a.first < elem_b.first; });
	return sorted_config;
}

std::string cgroup_controller::generate_command(const jobT &job, size_t counter, const execute_config &config) const {
	// index 0 == SLOT 0; index 1 == SLOT 1; index SLOTS == all slots on the same system are used
	std::string host_lists[SLOTS + 1];
	size_t hosts_per_slot[SLOTS + 1];
	std::string commands[SLOTS + 1];

	for (unsigned long &s : hosts_per_slot) s = 0;

	execute_config sorted_config = sort_config_by_hostname(config);

	for (size_t i = 0; i < config.size(); ++i) {
		// both slots of a system used?
		if (i + 1 < config.size() && config[i].first == config[i + 1].first) {
			++i;
			host_lists[SLOTS] += machines[config[i].first] + "," + machines[config[i].first];
			hosts_per_slot[SLOTS] += 2;
			continue;
		}
		host_lists[config[i].second] += machines[config[i].first] + ",";
		++hosts_per_slot[config[i].second];
	}

	assert(job.req_cpus() ==
		   std::accumulate(std::begin(hosts_per_slot), std::end(hosts_per_slot), size_t(0)) * SLOT_SIZE);

	for (size_t slot = 0; slot < SLOTS; ++slot) {
		if (hosts_per_slot[slot] == 0) continue;

		std::string command = " ./cgroup_wrapper.sh ";
		command += cmd_name_from_id(counter) + " ";

		// cgroup CPUs and memory is set by the bash script
		for (int i : co_configs[slot].cpus) {
			command += std::to_string(i) + ",";
		}
		// remove last ','
		command.pop_back();

		command += " ";

		for (int i : co_configs[slot].mems) {
			command += std::to_string(i) + ",";
		}
		// remove last ','
		command.pop_back();

		command += job.command;
		commands[slot] = command;
	}

	// some dedicated nodes
	if (hosts_per_slot[SLOTS] != 0) {
		std::string command = " ./cgroup_wrapper.sh ";
		command += cmd_name_from_id(counter) + " ";

		// cgroup CPUs and memory is set by the bash script
		for (size_t slot = 0; slot < SLOTS; ++slot) {
			for (int i : co_configs[slot].cpus) {
				command += std::to_string(i) + ",";
			}
		}
		// remove last ','
		command.pop_back();

		command += " ";

		for (size_t slot = 0; slot < SLOTS; ++slot) {
			for (int i : co_configs[slot].mems) {
				command += std::to_string(i) + ",";
			}
		}
		// remove last ','
		command.pop_back();

		command += job.command;
		commands[SLOTS] = command;
	}

	// TODO create hostfile
	// filename should be something like cmd_name_from_id(counter)
	// content:

	// TODO: add thread_per_procs to command string for the support of multi-threaded applications
	//       need to change -np
	std::string ret = "mpiexec ";
	ret += "-f "; // TODO hostfilename

	// a colon must be added between slots
	bool add_colon = false;
	for (size_t slot = 0; slot < SLOTS + 1; ++slot) {
		if (hosts_per_slot[slot] == 0) continue;

		if (add_colon) ret += " : ";

		ret += "-np " + std::to_string(hosts_per_slot[slot]) + " " + commands[slot];
		add_colon = true;
	}

	return ret;
}
