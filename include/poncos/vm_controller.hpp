/**
 * Poor mans scheduler
 *
 * Copyright 2017 by Jens Breitbart
 * Jens Breitbart     <jbreitbart@gmail.com>
 *
 * Licensed under GNU General Public License 2.0 or later.
 * Some rights reserved. See LICENSE
 */

#ifndef poncos_vm_controller
#define poncos_vm_controller

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "poncos/controller.hpp"
#include "poncos/job.hpp"
#include "poncos/poncos.hpp"

#include <fast-lib/message/migfra/result.hpp>
#include <fast-lib/message/migfra/task.hpp>
#include <fast-lib/mqtt_communicator.hpp>

class vm_controller : public controllerT {
  public:
	vm_controller(const std::shared_ptr<fast::MQTT_communicator> &_comm, const std::string &machine_filename,
				  const std::string &_slot_path);
	~vm_controller();

	void init();
	void dismantle();

	// freezes all VMs with supplied id
	void freeze(const size_t id);
	// thaws all VMs with the supplied id
	void thaw(const size_t id);

  private:
	std::string generate_command(const jobT &job, size_t counter, const execute_config &config) const;
	std::shared_ptr<fast::msg::migfra::Start> generate_start_task(size_t slot, vm_pool_elemT &free_vm);
	void command_done(const size_t config);

	template <typename T> void suspend_resume_virt_cluster(size_t slot);

	void start_all_VMs();
	void stop_all_VMs();

  private:
	// path to the xml slot files
	std::string slot_path;

	// maps ids to the slots
	std::unordered_map<size_t, size_t> id_to_slot;

	// stores the VMs used in the two slots
	// entries in the vector are read as: (host-name, guest-name)
	std::vector<std::pair<std::string, std::string>> virt_cluster[SLOTS];
};

#endif /* end of include guard: poncos_vm_controller */
