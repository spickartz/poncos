#ifndef two_app_scheduler
#define two_app_scheduler

#include "poncos/scheduler.hpp"

struct two_app_sched : public schedulerT {
	virtual void schedule(const job_queueT &job_queue, fast::MQTT_communicator &comm, controllerT &controller,
						  std::chrono::seconds wait_time);
	virtual void command_done(const size_t config);

	// marker if a slot is in use
	bool co_config_in_use[SLOTS] = {false, false};
	// distgen results of a slot
	double co_config_distgend[SLOTS] = {0.0, 0.0};
};

#endif /* end of include guard: two_app_scheduler */
