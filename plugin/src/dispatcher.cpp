#include "dispatcher.h"

double DISPATCHER::storage_needed(std::unordered_map<std::string, long long>& files) 
{
    long long sum = 0;
    for (const auto& [_, value] : files)
        sum += value;
    return sum;
}

std::string DISPATCHER::most_data_located(Job* j)
{
  const auto& files = j->input_files_sizes_locations;
  const auto needed = storage_needed(j->output_files);

  std::unordered_map<std::string, std::size_t> counts;
  for (const auto& [name, file] : files) for (const auto& site : file.second) ++counts[site];

  while (!counts.empty()) 
  {
    const auto best = std::max_element(counts.begin(), counts.end(),[](const auto& a, const auto& b) {return a.second < b.second;});
    if (CGSim::get_file_manager()->request_remaining_site_storage(best->first) >= needed)
    {std::cout << "best is: " + best->first << std::endl; return best->first;}
    counts.erase(best);
  }

  throw std::runtime_error("Could not find most data located for given job");

}

void DISPATCHER::findBestSite(Job* j)
{
  socket.sendJson
  ({
    {"request_type","assign_job"},
    {"job_id",j->jobid},
    {"expect_reply",true}
  });
  
  while(true)
  {
    SocketClient::json message;
    socket.receiveJson(message);
    std::string decision;
    if (message.contains("site_decision") && message["site_decision"].is_string())
    {j->comp_site = message["site_decision"].get<std::string>(); break;}

    if(message["request_type"] == "tool")
    {
      if(message["tool_type"] == "most_data_located")
      {
        auto result = most_data_located(j);
        //std::cout << result << std::endl;
        socket.sendJson
        ({
          {"response_type","tool"},
          {"tool_result",result},
          {"expect_reply",true}
        });
      }
      else throw std::runtime_error("Currently suppported tools are {'most_data_located'}");
    }  
  }
   
}


void DISPATCHER::findAvailableCPU(Job* j)
{
    if(j->comp_site == "") return;
    auto site = sg4::Engine::get_instance()->netzone_by_name_or_null(j->comp_site);
    auto cpus = site->get_all_hosts();

    for(const auto& cpu: cpus)
    {
        if(cpu->get_name().find("JOB-SERVER_cpu") != std::string::npos) continue;
        if(cpu->get_name().find("_communication_server") != std::string::npos) continue;
        if(cpu->extension<HostExtensions>()->get_cores_available() < j->cores) continue;

        auto d = cpu->get_disks()[0]; //Change later

        j->disk           =  d->get_name();
        j->disk_read_bw   =  d->get_read_bandwidth();
        j->disk_write_bw  =  d->get_write_bandwidth();

        j->comp_host          =  cpu->get_name();
        j->comp_host_speed    =  cpu->get_speed();

        return;
    }
}

Job* DISPATCHER::assignJob(Job* job)
{
  findBestSite(job);
  findAvailableCPU(job);
  return job;
}

