/**
 * Poor mans scheduler
 *
 * Copyright 2016 by Jens Breitbart
 * Jens Breitbart     <jbreitbart@gmail.com>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#ifndef poncos_cgroup_controller
#define poncos_cgroup_controller

#include <condition_variable>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "poncos/job.hpp"
#include "poncos/poncos.hpp"

#include <fast-lib/mqtt_communicator.hpp>

class cgroup_controller {
  public:
	// entries in the vector are read as: (machine index in machinefiles, slot it)
	using execute_config = std::vector<std::pair<size_t, size_t>>;

  public:
	cgroup_controller(const std::shared_ptr<fast::MQTT_communicator> &_comm, const std::string &machine_filename);
	~cgroup_controller();

	// freezes all cgroups with supplied id
	void freeze(const size_t id);
	// thaws all cgroups with the supplied id
	void thaw(const size_t id);

	// wait until ressources are free
	void wait_for_ressource();
	// waits until id has completed its run
	void wait_for_completion_of(const size_t id);
	// wait until all workers are finished
	void done();

	// executes a command
	size_t execute(const jobT &command, const execute_config &config, std::function<void(size_t)> callback);

  private:
	std::string generate_command(const jobT &command, std::string cg_name, const execute_config &config);
	void execute_command_internal(std::string command, std::string cg_name, size_t config_used,
								  std::function<void(size_t)> callback);
	void command_done(const size_t config);

	static std::string cgroup_name_from_id(size_t id);

  public:
	// getter
	const std::vector<std::string> &machines;

  private:
	// a list of all machines
	std::vector<std::string> _machines;

	// lock/cond variable used to wait for a job to be completed
	std::mutex worker_counter_mutex;
	std::condition_variable worker_counter_cv;
	std::unique_lock<std::mutex> work_counter_lock;

	// numbers of active workser
	size_t workers_active;

	// a counter that is increased with every new cgroup created
	size_t cgroups_counter;

	// threads used to run the applications
	std::vector<std::thread> thread_pool;

	// maps ids to the thread_pool
	std::unordered_map<size_t, size_t> id_to_pool;

	// reference to a mqtt communictor
	std::shared_ptr<fast::MQTT_communicator> comm;
};

#endif /* end of include guard: poncos_cgroup_controller */
