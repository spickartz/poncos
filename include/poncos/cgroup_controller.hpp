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

#include <memory>
#include <string>
#include <utility>

#include "poncos/controller.hpp"
#include "poncos/job.hpp"
#include "poncos/poncos.hpp"

#include <fast-lib/mqtt_communicator.hpp>

class cgroup_controller : public controllerT {
  public:
	cgroup_controller(const std::shared_ptr<fast::MQTT_communicator> &_comm, const std::string &machine_filename);
	~cgroup_controller();

	void init();
	void dismantle();

	// freezes all cgroups with supplied id
	void freeze(const size_t id);
	// thaws all cgroups with the supplied id
	void thaw(const size_t id);

  private:
	std::string generate_command(const jobT &command, size_t counter, const execute_config &config) const;
	void command_done(const size_t config);

  private:
};

#endif /* end of include guard: poncos_cgroup_controller */
