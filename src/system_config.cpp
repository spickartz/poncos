#include "poncos/system_config.hpp"
#include "poncos/poncos.hpp"

#include <fstream>

slotT::slotT(std::vector<unsigned int> cpus, std::vector<unsigned int> mems) : cpus(cpus), mems(mems) {}

YAML::Node slotT::emit() const {
	YAML::Node node;
	node["cpus"] = cpus;
	node["mems"] = mems;
	return node;
}

void slotT::load(const YAML::Node &node) {
	fast::load(cpus, node["cpus"]);
	fast::load(mems, node["mems"]);
}

system_configT::system_configT(std::vector<slotT> slots) : slots(std::move(slots)) {}

system_configT::system_configT(const std::string &config_filename) {
	std::fstream system_config_file(config_filename, std::fstream::in);
	std::stringstream system_config_stream;
	system_config_stream << system_config_file.rdbuf();
	fast::Serializable::from_string(system_config_stream.str());
}

YAML::Node system_configT::emit() const {
	YAML::Node node;
	node["slot-list"] = slots;

	return node;
}

void system_configT::load(const YAML::Node &node) { fast::load(slots, node["slot-list"]); }

std::ostream &operator<<(std::ostream &os, const slotT &slot) {
	os << "cpus: " << slot.cpus << "; ";
	os << "mems: " << slot.mems << "; ";

	return os;
}

