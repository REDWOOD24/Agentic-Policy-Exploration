#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <map>
#include <iostream>
#include <string>
#include "CGSim.h"
#include "client.h"

class DISPATCHER
{

public:
  DISPATCHER(){socket.connectSocket();};
 ~DISPATCHER(){};

  double      storage_needed(const std::unordered_map<std::string,std::string>& files);
  void        assignJob(CGSim::Job* job);
  void        findBestSite(CGSim::Job* j);  
  void        findAvailableCPU(CGSim::Job* j);
  std::string most_data_located(CGSim::Job* j);
  void        onSimulationEnd(){
    socket.sendJson
        ({
          {"request_type","end_simulation"},
          {"expect_reply",false}
        });}
  
private:
  SocketClient socket = SocketClient();
  CGSim::GlobalManagers::ResourceManager* rm = CGSim::GlobalManagers::get_resource_manager();
  CGSim::GlobalManagers::FileManager*     fm = CGSim::GlobalManagers::get_file_manager();
};

#endif
