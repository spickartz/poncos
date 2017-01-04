#ifndef scheduler_multi_hpp
#define scheduler_multi_hpp

#include "poncos/scheduler.hpp"

struct multi_app_sched : public schedulerT {
	virtual void schedule(const job_queueT &job_queue, fast::MQTT_communicator &comm, controllerT &controller,
						  std::chrono::seconds wait_time);
	virtual void command_done(const size_t config);
};

#endif /* end of include guard: scheduler_multi_hpp */
