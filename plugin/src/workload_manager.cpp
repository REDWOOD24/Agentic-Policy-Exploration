#include "workload_manager.h"
#include <random>

long long WORKLOAD_MANAGER::random_number(long long min, long long max)
{
    static std::mt19937_64 gen(std::random_device{}());
    return std::uniform_int_distribution<long long>(min, max)(gen);
}

void WORKLOAD_MANAGER::setWorkload(CGSim::JobQueue& jobs)
{
    long max_jobs = std::stol(CGSim::GlobalManagers::get_resource_manager()->get_custom_parameter("Num_of_Jobs"));

    for (long i = 1; i <= max_jobs; ++i)
    {
        auto* job = new CGSim::Job();
        std::string id = std::to_string(i);

        job->set_id(id);
        job->set_creation_time(random_number(0, 30));
        job->set_cores(random_number(1, 8));
        job->set_flops(random_number(1000000, 2000000));

        auto number_of_input_files = random_number(1, 5);
        for (int j = 0; j < number_of_input_files; ++j) job->add_input_file(std::to_string(random_number(0, 29999)));

        std::string output = "output_" + id + "_0.root";
        job->add_output_file(output, std::to_string(random_number(200000000000, 300000000000)));

        jobs.push(job);
    }
}
