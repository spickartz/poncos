#ifndef poncos_system_config
#define poncos_system_config

#include <fast-lib/serializable.hpp>

struct slotT : public fast::Serializable {
	slotT() = default;
	slotT(std::vector<unsigned int> cpus, std::vector<unsigned int> mems);

	YAML::Node emit() const override;
	void load(const YAML::Node &node) override;

	std::vector<unsigned int> cpus;
	std::vector<unsigned int> mems;
};
std::ostream &operator<<(std::ostream &os, const slotT &slot);

struct system_configT : public fast::Serializable {
	system_configT() = default;
	system_configT(const std::string &config_filename);
	system_configT(std::vector<slotT> slots);

	YAML::Node emit() const override;
	void load(const YAML::Node &node) override;
	size_t slot_size(void) const { return slots[0].cpus.size(); }
	const slotT &operator[](const size_t idx) const { return slots[idx]; };

	std::vector<slotT> slots;
};

YAML_CONVERT_IMPL(slotT)
YAML_CONVERT_IMPL(system_configT)

#endif /* end of include guard: poncos_job */
