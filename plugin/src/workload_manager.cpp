#include "workload_manager.h"

void WORKLOAD_MANAGER::setWorkload(CGSim::JobQueue& jobs)
{
    long max_jobs = std::stol(CGSim::GlobalManagers::get_resource_manager()->get_custom_parameter("Num_of_Jobs"));
    std::string jobs_file = CGSim::GlobalManagers::get_resource_manager()->get_custom_parameter("jobs_file");
    std::ifstream file(jobs_file);
    nlohmann::json job_data = nlohmann::json::parse(file);
    int num_of_jobs = 0;

    for (const auto& j : job_data)
    {
        if (max_jobs > 0 && num_of_jobs >= max_jobs) break;
        auto* job = new CGSim::Job();

        std::string id = j["id"].get<std::string>();
        job->set_id(id);
        job->set_creation_time(j["creation_time"].get<double>());
        job->set_cores(j["cores"].get<int>());
        job->set_flops(j["flops"].get<double>());
        for (const auto& input : j["input_files"]) job->add_input_file(input.get<std::string>());
        for (const auto& output : j["output_files"]) job->add_output_file(output["name"].get<std::string>(), std::to_string(output["size"].get<unsigned long long>()));

        jobs.push(job);
        ++num_of_jobs;
    }
}
