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

  double      storage_needed(std::unordered_map<std::string, long long>& files);
  Job*        assignJob(Job* job);
  void        findBestSite(Job* j);  
  void        findAvailableCPU(Job* j);
  std::string most_data_located(Job* j);

private:
  SocketClient socket = SocketClient();
};

#endif
