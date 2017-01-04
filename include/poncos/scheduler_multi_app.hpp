#ifndef scheduler_multi_hpp
#define scheduler_multi_hpp

#include "poncos/scheduler.hpp"

struct multi_app_sched : public schedulerT {
	virtual void schedule(const job_queueT &job_queue, fast::MQTT_communicator &comm, controllerT &controller,
						  std::chrono::seconds wait_time);
	virtual void command_done(const size_t config, controllerT &controller);

	std::vector<size_t> check_membw(const controllerT::execute_config &config) const;

	std::vector<std::array<double, SLOTS>> membw_util;
};

#endif /* end of include guard: scheduler_multi_hpp */
