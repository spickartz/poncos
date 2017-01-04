#include "poncos/scheduler_multi_app.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>

#include "poncos/cgroup_controller.hpp"
#include "poncos/job.hpp"
#include "poncos/poncos.hpp"

// called after a command was completed
void multi_app_sched::command_done(const size_t config) {}

void multi_app_sched::schedule(const job_queueT &job_queue, fast::MQTT_communicator &comm, controllerT &controller,
							   std::chrono::seconds wait_time) {
	// for all commands
	for (auto job : job_queue.jobs) {
// wait until a job is finished <- controller TODO add next job size
// check if enough ressources are available for new job <- here
// -> check the size of the free lists
// no -> wait again
// yes -> continue

// select ressources
// -> pick one VM per machine ie use only ressources from one free list
// -> which one is not important
// map <host-id, std::array<2, std::pair<guest-name, free?>> <- controller
// -> return types: - std::vector<guest-name> to start-job
//                  - std::vector<std::pair<host-id, slot>> to find opossing VM

// start job on this VMs
// controller.execute()

// wait

// stop the opposing VM
// controller.stop_opposing(std::vector<std::pair<host-id, slot>>)

// start distgen
// -> store with job
// -> map <host, jobs>

// for all host-id of new job
// 	membw ok?
// 		yes -> next
// 		no -> mark it
// for all marked
// 	check if there is another host available
//		yes: save pair for swap
//		no: job must be suspended
// if yes for all: swap
// if no:
// 	wait for a job to finish
//	check again if membw is ok / swap

#if 0
		assert(job.nprocs == controller.machines.size() * SLOT_SIZE);

		controller.wait_for_ressource(job.nprocs);

		// search for a free slot and assign it to a new job
		size_t new_slot = 0;
		size_t job_id = 0;
		for (; new_slot < SLOTS; ++new_slot) {
			if (!co_config_in_use[new_slot]) {
				co_config_in_use[new_slot] = true;

				cgroup_controller::execute_config config;

				for (size_t j = 0; j < controller.machines.size(); ++j) {
					config.emplace_back(j, new_slot);
				}

				job_id = controller.execute(job, config, [this](const size_t config) { command_done(config); });

				std::cout << ">> \t starting '" << job << "' at configuration " << new_slot << std::endl;

				break;
			}
		}
		assert(new_slot < SLOTS);

		// for the initialization phase of the application to be completed
		std::this_thread::sleep_for(wait_time);

		// check if two are running
		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: freezing old" << std::endl;
			controller.freeze_opposing(job_id);
		}

		// measure distgen result
		std::cout << ">> \t Running distgend" << std::endl;
		co_config_distgend[new_slot] =
			run_distgen(comm, controller.machines, controller.generate_opposing_config(job_id));

		std::cout << ">> \t Result for command '" << job << "' is: " << 1 - co_config_distgend[new_slot] << std::endl;

		if (co_config_in_use[0] && co_config_in_use[1]) {
			// std::cout << "0: thaw old" << std::endl;
			controller.thaw_opposing(job_id);

			std::cout << ">> \t Estimating total usage of "
					  << (1 - co_config_distgend[0]) + (1 - co_config_distgend[1]);

			if ((1 - co_config_distgend[0]) + (1 - co_config_distgend[1]) > 0.9) {
				std::cout << " -> we will run one" << std::endl;
				// std::cout << "0: freezing new" << std::endl;
				controller.freeze(job_id);

				controller.wait_for_ressource(job.nprocs);

				// std::cout << "0: thaw new" << std::endl;
				controller.thaw(job_id);
			} else {
				std::cout << " -> we will run both applications" << std::endl;
			}

		} else {
			std::cout << ">> \t Just one config in use ATM" << std::endl;
		}
#endif
	}
	controller.done();
}
