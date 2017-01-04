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
	virtual void command_done(const size_t config) = 0;
	static std::vector<double> run_distgen(fast::MQTT_communicator &comm, const std::vector<std::string> &machines,
										   const controllerT::execute_config &config);
};

#endif /* end of include guard: poncos_scheduler */
