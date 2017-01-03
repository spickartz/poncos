#ifndef poncos_controller
#define poncos_controller

#include <condition_variable>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fast-lib/mqtt_communicator.hpp>

#include "poncos/job.hpp"

class controllerT {
  public:
	// entries in the vector are read as: (machine index in machinefiles, #slot)
	using execute_config = std::vector<std::pair<size_t, size_t>>;

  public:
	controllerT(const std::shared_ptr<fast::MQTT_communicator> &_comm, const std::string &machine_filename);
	virtual ~controllerT();

	virtual void init() = 0;
	virtual void dismantle() = 0;

	virtual void freeze(const size_t id) = 0;
	virtual void thaw(const size_t id) = 0;

	virtual size_t execute(const jobT &job, const execute_config &config, std::function<void(size_t)> callback);

	virtual void wait_for_ressource(const size_t);
	virtual void wait_for_completion_of(const size_t);
	virtual void done();

	const std::vector<std::string> &machines;
	const size_t &total_available_slots;

  protected:
	// executed by a new thread, calls system to start the application
	void execute_command_internal(std::string command, size_t cmd_counter, const execute_config config,
								  std::function<void(size_t)> callback);
	virtual std::string generate_command(const jobT &command, size_t counter, const execute_config &config) const = 0;
	std::string cmd_name_from_id(const size_t id) const;

  protected:
	// numbers of slots free
	size_t free_slots;

	// a counter that is increased with every new cgroup created
	size_t cmd_counter;

	// lock/cond variable used to wait for a job to be completed
	std::mutex worker_counter_mutex;
	std::condition_variable worker_counter_cv;
	std::unique_lock<std::mutex> work_counter_lock;

	// threads used to run the applications
	std::vector<std::thread> thread_pool;

	// maps ids to the thread_pool
	std::vector<size_t> id_to_tpool;
	// maps ids to the slots
	std::vector<size_t> id_to_slot;

	// reference to a mqtt communictor
	std::shared_ptr<fast::MQTT_communicator> comm;

  private:
	// numbers of total slots available
	size_t _total_available_slots;
	// a list of all machines
	std::vector<std::string> _machines;
};

#endif /* end of include guard: poncos_controller */
