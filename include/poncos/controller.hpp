#ifndef poncos_controller
#define poncos_controller

#include <string>
#include <vector>

#include "poncos/job.hpp"

// disable the weak vtable warning as this is a pure virtual function, which has no implementation
// so its vtable must be emitted in every compilation unit
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

struct controllerT {
	// entries in the vector are read as: (machine index in machinefiles, #slot)
	using execute_config = std::vector<std::pair<size_t, size_t>>;

	virtual ~controllerT() = 0;
	virtual void init() = 0;
	virtual void dismantle() = 0;

	virtual void freeze(const size_t id) = 0;
	virtual void thaw(const size_t id) = 0;

	virtual void wait_for_ressource(const size_t) = 0;
	virtual void wait_for_completion_of(const size_t) = 0;
	virtual void done() = 0;

	virtual size_t execute(const jobT &, const execute_config &, std::function<void(size_t)>) = 0;

	virtual const std::vector<std::string> &machines() = 0;
};

inline controllerT::~controllerT() {}

#pragma clang diagnostic pop

#endif /* end of include guard: poncos_controller */
