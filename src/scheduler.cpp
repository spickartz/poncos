#include "poncos/scheduler.hpp"

#include <fast-lib/message/agent/mmbwmon/reply.hpp>
#include <fast-lib/message/agent/mmbwmon/request.hpp>

schedulerT::~schedulerT() {}

double schedulerT::run_distgen(fast::MQTT_communicator &comm, const std::vector<std::string> &machines,
							   const controllerT::execute_config &config) {

	// ask for measurements
	{
		for (const auto &c : config) {
			fast::msg::agent::mmbwmon::request m;

			const auto &slot_conf = co_configs[c.second];

			// TODO check if we can use the same type
			m.cores.resize(slot_conf.cpus.size());
			for (size_t i = 0; i < slot_conf.cpus.size(); ++i) {
				m.cores[i] = static_cast<size_t>(slot_conf.cpus[i]);
			}

			const std::string topic = "fast/agent/" + machines[c.first] + "/mmbwmon/request";
			// std::cout << "sending message \n topic: " << topic << "\n message:\n" << m.to_string() << std::endl;
			comm.send_message(m.to_string(), topic);
		}
	}

	double ret = 0.0;

	// wait for results
	{
		for (const auto &c : config) {
			fast::msg::agent::mmbwmon::reply m;
			const std::string topic = "fast/agent/" + machines[c.first] + "/mmbwmon/response";
			// std::cout << "waiting on topic: " << topic << " ... " << std::flush;
			m.from_string(comm.get_message(topic));
			// std::cout << "done\n";

			if (m.result > ret) ret = m.result;
		}
	}

	return ret;
}
