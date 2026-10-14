#ifndef POLICY_H
#define POLICY_H

#include <algorithm>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "client.h"

#include "CGSim.h"


class POLICY {
public:
    POLICY() = default;
   ~POLICY() = default;

   void onSimulationStart();
   void onSimulationEnd();

private:

    //Agent Communication
    CGSim::Policy* agentic_decision_policy();
    void call_agent();

    //Policies
    CGSim::Policy* simple_data_replication_policy();
    void execute_data_replication();

    //MCP Tool
    std::string calculate_storage_imbalance();

    SocketClient socket = SocketClient();
    CGSim::GlobalManagers::PolicyManager* pm = CGSim::GlobalManagers::get_policy_manager();
    CGSim::GlobalManagers::ResourceManager* rm = CGSim::GlobalManagers::get_resource_manager();
    CGSim::GlobalManagers::FileManager* fm = CGSim::GlobalManagers::get_file_manager();

    
    std::map<std::string, std::function<CGSim::Policy*()>> policy_map = 
    {
        {"simple_data_replication", [this]() {
            return simple_data_replication_policy();
        }}
    };

    std::map<std::string, std::function<std::string()>> tools = 
    {
        {"calculate_storage_imbalance", [this]() {
            return calculate_storage_imbalance();
        }}
    };
};

#endif