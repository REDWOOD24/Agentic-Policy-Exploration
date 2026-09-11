#include "dispatcher.h"
#include "units_parser.h"
#include <algorithm>

double DISPATCHER::storage_needed(const std::unordered_map<std::string,std::string>& files)
{
    long long sum = 0;
    for (const auto& [_,v] : files) sum += CGSim::Utilities::parse_units_size(v);
    return sum;
}

std::string DISPATCHER::most_data_located(CGSim::Job* j)
{
    auto files = j->get_input_files();
    auto needed = storage_needed(j->get_output_files());

    std::unordered_map<std::string,std::size_t> counts;
    for (const auto& filename : files)
      for (const auto& site : fm->request_file(filename)->get_locations()) ++counts[site];

    while (!counts.empty()) {
        auto best = std::max_element(counts.begin(),counts.end(),
            [](const auto& a,const auto& b){ return a.second < b.second; });

        if (rm->get_site(best->first)->get_available_storage() >= needed) {
            return best->first;
        }
        counts.erase(best);
    }
    throw std::runtime_error("Could not find most data located for given job");
}

void DISPATCHER::findBestSite(CGSim::Job* j)
{
    socket.sendJson({
        {"request_type","assign_job"},
        {"job_id",j->get_id()},
        {"expect_reply",true}
    });

    while (true) {
        SocketClient::json message;
        socket.receiveJson(message);

        if (message.contains("site_decision") && message["site_decision"].is_string()) {
            j->set_site(message["site_decision"].get<std::string>());
            break;
        }

        if (message["request_type"] == "tool") {
            if (message["tool_type"] == "most_data_located")
                socket.sendJson({
                    {"response_type","tool"},
                    {"tool_result",most_data_located(j)},
                    {"expect_reply",true}
                });
            else throw std::runtime_error("Currently suppported tools are {'most_data_located'}");
        }
    }
}

void DISPATCHER::findAvailableCPU(CGSim::Job* j)
{
    if (j->get_site().empty()) return;

    auto* site = CGSim::GlobalManagers::get_resource_manager()->get_site(j->get_site());

    for (auto* cpu : site->get_cpus()) {
        if (cpu->get_cores_available() < j->get_cores()) continue;
        j->set_cpu(cpu->get_name());
        j->set_disk(cpu->get_disks()[0]->get_name());
        return;
    }
}

void DISPATCHER::assignJob(CGSim::Job* job)
{
    findBestSite(job);
    findAvailableCPU(job);
}
