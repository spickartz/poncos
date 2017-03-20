#ifndef poncos_scheduler
#define poncos_scheduler

#include "poncos/controller.hpp"
#include "poncos/job.hpp"

#include <chrono>
#include <string>
#include <vector>

#include <fast-lib/mqtt_communicator.hpp>

struct schedulerT {
	virtual ~schedulerT();
	virtual void schedule(const job_queueT &, fast::MQTT_communicator &, controllerT &, std::chrono::seconds) = 0;
	virtual void command_done(const size_t config, controllerT &controller) = 0;
	std::vector<double> run_distgen(fast::MQTT_communicator &comm, const controllerT &controller, const size_t job_id);
};

#endif /* end of include guard: poncos_scheduler */
