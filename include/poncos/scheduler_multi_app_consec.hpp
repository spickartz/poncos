#ifndef scheduler_multi_consec_hpp
#define scheduler_multi_consec_hpp

#include "poncos/scheduler.hpp"

struct multi_app_sched_consec : public schedulerT {
	multi_app_sched_consec(const system_configT &system_config);

	virtual void schedule(const job_queueT &job_queue, fast::MQTT_communicator &comm, controllerT &controller,
						  std::chrono::seconds wait_time);
	virtual void command_done(const size_t id, controllerT &controller);
};

#endif /* end of include guard: scheduler_multi_consec_hpp */
