#include "poncos/job.hpp"
#include "poncos/poncos.hpp"

#include <fstream>

jobT::jobT(size_t nprocs, size_t threads_per_proc, std::string command, bool uses_sr_protocol)
	: nprocs(nprocs), threads_per_proc(threads_per_proc), command(std::move(command)),
	  uses_sr_protocol(uses_sr_protocol) {}

YAML::Node jobT::emit() const {
	YAML::Node node;
	node["nprocs"] = nprocs;
	node["threads-per-proc"] = threads_per_proc;
	node["cmd"] = command;
	node["uses-sr-protocol"] = uses_sr_protocol;
	return node;
}

void jobT::load(const YAML::Node &node) {
	fast::load(nprocs, node["nprocs"]);
	fast::load(threads_per_proc, node["threads-per-proc"]);
	fast::load(command, node["cmd"]);
	fast::load(uses_sr_protocol, node["uses-sr-protocol"]);
}

job_queueT::job_queueT(std::vector<jobT> jobs) : jobs(std::move(jobs)) {}

job_queueT::job_queueT(const std::string &queue_filename) {
	fast::Serializable::from_string(read_file_to_string(queue_filename));
}

YAML::Node job_queueT::emit() const {
	YAML::Node node;
	node["job-list"] = jobs;

	return node;
}

void job_queueT::load(const YAML::Node &node) { fast::load(jobs, node["job-list"]); }

std::ostream &operator<<(std::ostream &os, const jobT &job) {
	os << "nprocs: " << job.nprocs << "; ";
	os << "threads-per-proc: " << job.threads_per_proc << "; ";
	os << "cmd: " << job.command << "; ";
	os << "uses-sr-protocol: " << job.uses_sr_protocol;

	return os;
}
