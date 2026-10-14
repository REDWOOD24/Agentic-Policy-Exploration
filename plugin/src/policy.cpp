#include "policy.h"

void POLICY::onSimulationEnd()
{
    socket.sendJson
    ({
        {"request_type","end_simulation"}, 
        {"expect_reply",false}
    });
}

void POLICY::onSimulationStart()
{
    socket.connectSocket();
    pm->addPolicy(agentic_decision_policy());
}

CGSim::Policy* POLICY::simple_data_replication_policy()
{
    auto* p             = new CGSim::Policy();
    p->name             = "Simple Data Replication Policy";
    p->start_time       = CGSim::Utilities::get_simulation_clock() < 0.0 ? 0.0 : CGSim::Utilities::get_simulation_clock();
    p->repeat_interval  = CGSim::Utilities::parse_units_time("1w");
    p->callback         = [this](){execute_data_replication();};
    return p;
}

//MCP TOOL
std::string POLICY::calculate_storage_imbalance()
{
    auto site_names = rm->get_list_of_sites();
    double avg = 0.0;
    for (const auto& site_name : site_names) {auto* s = rm->get_site(site_name); avg += s->get_storage_utilization();}
    avg /= site_names.size();
    double imbalance = 0.0;
    for (const auto& site_name : site_names) {auto* s = rm->get_site(site_name); imbalance += std::abs(s->get_storage_utilization() - avg);}
    double answer = imbalance / site_names.size();
    return std::to_string(answer);
}


void POLICY::execute_data_replication()
{
    for(auto& [srcName,src]:rm->get_all_sites())
    {
        for(auto& dstName:rm->get_list_of_sites())
        {
            if(dstName==srcName) continue;
            auto* dst=rm->get_site(dstName);
            size_t N = 50;
            size_t copied=0;
            for(auto& [fileName,file]:src->get_files())
            {
                if(copied>=N) break;
                if(fm->exists(fileName,dstName)) continue;
                if(dst->get_incoming_file_transfers().count(fileName)) continue;
                if(dst->get_available_storage() > file->get_size()) continue;
                fm->transfer(fileName,srcName,dstName,CGSim::FileTransferDecisionMode::COPY,"grid-replication");
                ++copied;
            }
        }
    }
        
}

CGSim::Policy* POLICY::agentic_decision_policy()
{
    auto* p             = new CGSim::Policy();
    p->name             = "Agentic Decision Policy";
    p->start_time       = CGSim::Utilities::get_simulation_clock() < 0.0 ? 0.0 : CGSim::Utilities::get_simulation_clock();
    p->repeat_interval  = CGSim::Utilities::parse_units_time("1w");
    p->callback         = [this](){call_agent();};
    return p;
}

void POLICY::call_agent()
{
    //Send communication to agent to select a data management policy
    socket.sendJson
    ({
        {"request_type","pick_data_management_policy"},
        {"policies_available","simple_data_replication"},
        {"expect_reply",true}
    });

    while (true) 
    {
        //Agent should send something back, Either a decision or tool request
        SocketClient::json message;
        socket.receiveJson(message);

        //Decision case, if decision is received it is implemented and communication with agent is exited.
        if (message.contains("decision") && message["decision"].is_array()) 
        {
            for (const auto& policy : message["decision"])
            {
                std::string policy_name = policy.get<std::string>();

                if(pm->exists(policy_name))
                {
                    //If policy is active, let it continue
                    auto* p = pm->get_policy(policy_name);
                    if(p->is_active()) continue;

                    //If it was previously deactivated it, reactivate it
                    p->start_time = CGSim::Utilities::get_simulation_clock();
                    pm->reactivate_policy(policy_name);
                }

                else
                {
                    //If policy was never added, add it
                    auto it = policy_map.find(policy_name);
                    if (it != policy_map.end()) {auto* p = it->second(); pm->addPolicy(p);}
                    else throw std::runtime_error("Agent is requesting execution of an unknown policy: " + policy_name);
                }
            }

            //Deactivate any running policies not picked by the agent
            std::unordered_set<std::string> decisions(message["decision"].begin(), message["decision"].end());
            for (const auto& [name, func] : policy_map) 
            if (!decisions.count(name) && pm->exists(name)) if(pm->get_policy(name)->is_active()) pm->deactivate_policy(name);
            
            //Decision is executed, so exit
            break;
        }

        //Tool request case, Communicate tool result back to agent and wait for further communication.
        if (message["request_type"] == "tool")
        {
            std::string tool = message["tool_type"].get<std::string>();
            auto it = tools.find(tool);
            if (it != tools.end()) 
            {
                auto tool_result = it->second();
                socket.sendJson
                ({
                {"response_type","tool"},
                {"tool_result",tool_result},
                {"expect_reply",true}
                });
            }
            else throw std::runtime_error("Agent is requesting execution of an unknown tool: " + tool);
        }

    }

}