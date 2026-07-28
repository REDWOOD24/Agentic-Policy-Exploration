#pragma once
#include <nlohmann/json.hpp>
#include <string>

class SocketClient{
    int s=-1; std::string host; int port;
public:
    using json=nlohmann::json;
    SocketClient(std::string h="127.0.0.1",int p=5000):host(h),port(p){}
    ~SocketClient();
    bool connectSocket(),sendJson(const json&),receiveJson(json&);
};
