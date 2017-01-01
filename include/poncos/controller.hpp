#ifndef poncos_controller
#define poncos_controller

#include <string>
#include <vector>

#include "poncos/job.hpp"

struct controllerT {
	// entries in the vector are read as: (machine index in machinefiles, #slot)
	using execute_config = std::vector<std::pair<size_t, size_t>>;

	virtual ~controllerT() = 0;
	virtual void init() = 0;
	virtual void dismantle() = 0;

	virtual void freeze(const size_t id) = 0;
	virtual void thaw(const size_t id) = 0;

	virtual void wait_for_ressource() = 0;
	virtual void wait_for_completion_of(const size_t id) = 0;
	virtual void done() = 0;

	virtual size_t execute(const jobT &command, const execute_config &config, std::function<void(size_t)> callback) = 0;

	virtual const std::vector<std::string> &machines() = 0;
};

#endif /* end of include guard: poncos_controller */