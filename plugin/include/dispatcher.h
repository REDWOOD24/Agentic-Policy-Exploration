#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <map>
#include <iostream>
#include <string>
#include "CGSim.h"

class DISPATCHER
{

public:
  DISPATCHER(){};
 ~DISPATCHER(){};
  void assignJob(CGSim::Job* job);
  
private:
  double      storage_needed(const std::unordered_map<std::string,std::string>& files);
  void        findBestSite(CGSim::Job* j);  
  void        findAvailableCPU(CGSim::Job* j);
  std::string most_data_located(CGSim::Job* j);

  CGSim::GlobalManagers::ResourceManager* rm = CGSim::GlobalManagers::get_resource_manager();
  CGSim::GlobalManagers::FileManager*     fm = CGSim::GlobalManagers::get_file_manager();
};

#endif
