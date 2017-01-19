#include "poncos/controller.hpp"

#include <iostream>
#include <limits>
#include <utility>

#include "poncos/poncos.hpp"

// inititalize fast-lib log
FASTLIB_LOG_INIT(controller_log, "controller")
FASTLIB_LOG_SET_LEVEL_GLOBAL(controller_log, trace);

controllerT::controllerT(std::shared_ptr<fast::MQTT_communicator> _comm, const std::string &machine_filename)
	: machines(_machines), available_slots(_available_slots), machine_usage(_machine_usage),
	  id_to_config(_id_to_config), cmd_counter(0), work_counter_lock(worker_counter_mutex), comm(std::move(_comm)) {

	// fill the machine file
	FASTLIB_LOG(controller_log, info) << "Reading machine file " << machine_filename << " ...";
	read_file(machine_filename, _machines);

	FASTLIB_LOG(controller_log, info) << "Machine file:";
	FASTLIB_LOG(controller_log, info) << "==============";
	for (const std::string &c : _machines) {
		FASTLIB_LOG(controller_log, info) << c;
	}
	FASTLIB_LOG(controller_log, info) << "==============";

	_machine_usage.assign(_machines.size(), std::array<size_t, 2>{{std::numeric_limits<size_t>::max(),
																   std::numeric_limits<size_t>::max()}});
	FASTLIB_LOG(controller_log, trace) << "machine_usage = " << machine_usage;

	_available_slots = _machines.size();
}

controllerT::~controllerT() {
	done();
	for (auto &t : thread_pool) {
		if (t.joinable()) t.join();
	}
	thread_pool.resize(0);
}

void controllerT::done() {
	// wait until all workers are finished
	if (!work_counter_lock.owns_lock()) work_counter_lock.lock();
	worker_counter_cv.wait(work_counter_lock, [&] {
		for (auto i : machine_usage) {
			for (size_t s = 0; s < SLOTS; ++s) {
				if (i[s] != std::numeric_limits<size_t>::max()) return false;
			}
		}

		return true;
	});
}

void controllerT::wait_for_ressource(const size_t requested) {
	if (!work_counter_lock.owns_lock()) work_counter_lock.lock();

	worker_counter_cv.wait(work_counter_lock, [&] {
		// TODO maybe we should not do this lazy but keep it updated all the time
		size_t counter = 0;

		for (auto i : machine_usage) {
			for (size_t s = 0; s < SLOTS; ++s) {
				if (i[s] == std::numeric_limits<size_t>::max()) {
					counter += SLOT_SIZE;
					break;
				}
			}
		}

		return counter >= requested;
	});
}

void controllerT::wait_for_change() {
	if (!work_counter_lock.owns_lock()) work_counter_lock.lock();

	worker_counter_cv.wait(work_counter_lock);
}

void controllerT::wait_for_completion_of(const size_t id) {
	assert(id < id_to_tpool.size());
	work_counter_lock.unlock();

	thread_pool[id_to_tpool[id]].join();
}

controllerT::execute_config controllerT::generate_opposing_config(const size_t id) const {
	assert(id < id_to_config.size());
	static_assert(SLOTS == 2, "");

	execute_config opposing_config;
	const execute_config &config = id_to_config[id];

	for (auto const &config_elem : config) {
		// TODO: what abour more than two slots per host?
		opposing_config.emplace_back(config_elem.first, (config_elem.second + 1) % SLOTS);
	}

	return opposing_config;
}

void controllerT::update_config(const size_t id, const execute_config &new_config) {
	execute_config &old_config = _id_to_config[id];
	assert(new_config.size() == old_config.size());
	FASTLIB_LOG(controller_log, trace) << "job #" << id << ": machine_usage = " << machine_usage;
	FASTLIB_LOG(controller_log, trace) << "job #" << id << ": config        = " << old_config;
	// update id_to_config for all affected jobs and machine_usage.
	for (size_t i = 0; i < new_config.size(); ++i) {
		auto &oc = old_config[i];
		const auto &nc = new_config[i];

		// get the config of the opposing job
		const size_t op_job_id = machine_usage[nc.first][nc.second];
		std::swap(_machine_usage[oc.first][oc.second], _machine_usage[nc.first][nc.second]);

		// is there any "oposite job"?
		if (op_job_id == std::numeric_limits<size_t>::max()) {
			// no? We only need to update our config
			oc = nc;
			continue;
		}

		// yes? get the oposing config
		execute_config &op_config = _id_to_config[op_job_id];

		// find entry that matches our current entry in the new config
		bool success = false;
		for (auto &opc : op_config) {
			if (opc.first == nc.first && opc.second == nc.second) {
				// swap both configs
				std::swap(opc, oc);
				success = true;
				break;
			}
		}
		assert(success);
	}
	FASTLIB_LOG(controller_log, trace) << "job #" << id << ": machine_usage = " << machine_usage;
	FASTLIB_LOG(controller_log, trace) << "job #" << id << ": config        = " << old_config;

	// "old" config should now be updated to the new config
	assert(old_config == new_config);
}

size_t controllerT::execute(const jobT &job, const execute_config &config, std::function<void(size_t)> callback) {
	assert(work_counter_lock.owns_lock());
	assert(!config.empty());

	id_to_tpool.push_back(thread_pool.size());
	_id_to_config.push_back(config);
	for (const auto &i : config) {
		assert(machine_usage[i.first][i.second] == std::numeric_limits<size_t>::max());
		_machine_usage[i.first][i.second] = cmd_counter;
	}
	FASTLIB_LOG(controller_log, trace) << "machine_usage = " << machine_usage;

	const std::string command = generate_command(job, cmd_counter, config);
	thread_pool.emplace_back(&controllerT::execute_command_internal, this, command, cmd_counter, config, callback);

	return cmd_counter++;
}

void controllerT::execute_command_internal(std::string command, size_t counter, const execute_config &config,
										   const std::function<void(size_t)> &callback) {
	const std::string cmd_name = cmd_name_from_id(counter);

	command += " 2>&1 ";
	// command += "| tee ";
	command += "> ";
	command += cmd_name + ".log";

	auto temp = system(command.c_str());
	assert(temp != -1);

	// we are done
	const execute_config &cur_config = id_to_config[counter];
	FASTLIB_LOG(controller_log, info) << ">> \t '" << command << "' completed at configuration "
									  << cur_config[0].second;

	std::lock_guard<std::mutex> work_counter_lock(worker_counter_mutex);

	FASTLIB_LOG(controller_log, trace) << "job #" << counter << ": machine_usage = " << machine_usage;
	FASTLIB_LOG(controller_log, trace) << "job #" << counter << ": config        = " << id_to_config[counter];
	for (const auto &i : cur_config) {
		assert(machine_usage[i.first][i.second] != std::numeric_limits<size_t>::max());
		_machine_usage[i.first][i.second] = std::numeric_limits<size_t>::max();
	}
	FASTLIB_LOG(controller_log, trace) << "machine_usage = " << machine_usage;

	callback(cur_config[0].second);
	worker_counter_cv.notify_one();
}

std::string controllerT::cmd_name_from_id(size_t id) const { return std::string("poncos_") + std::to_string(id); }
