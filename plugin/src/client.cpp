#include "client.h"
#include <arpa/inet.h>
#include <unistd.h>

bool SocketClient::connectSocket(){
    s=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port);
    inet_pton(AF_INET,host.c_str(),&a.sin_addr);
    return connect(s,(sockaddr*)&a,sizeof(a))==0;
}

bool SocketClient::sendJson(const json& j){
    std::string x=j.dump()+"\n";
    return send(s,x.data(),x.size(),0)>0;
}

bool SocketClient::receiveJson(json& j){
    std::string x; char c;
    while(recv(s,&c,1,0)>0&&c!='\n')x+=c;
    if(x.empty())return false;
    j=json::parse(x); return true;
}

SocketClient::~SocketClient(){if(s!=-1)close(s);}
