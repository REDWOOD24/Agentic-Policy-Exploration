#include "plugin.h"
#include "dispatcher.h"
#include "workload_manager.h"
#include "output.h"
#include "policy.h"

class AgenticPolicyExplorationPlugin : public CGSim::Plugin {
public:
    AgenticPolicyExplorationPlugin() {}

    void setWorkload(CGSim::JobQueue& jobs) final override { wm->setWorkload(jobs); }
    void assignJob(CGSim::Job* job) final override { di->assignJob(job); }

    void onSimulationStart() final override { po->onSimulationStart(); }
    void onSimulationEnd() final override { po->onSimulationEnd(); }

    void onJobExecutionEnd(CGSim::Job* j) final override { ou->onJobExecutionEnd(j); }
    void onUserFileTransferStart(const std::string& f,unsigned long long s,const std::string& src,const std::string& dst,const std::string& p) final override { ou->onUserFileTransferStart(f,s,src,dst,p); }
    void onUserFileTransferEnd(const std::string& f,unsigned long long s,const std::string& src,const std::string& dst,const std::string& p) final override { ou->onUserFileTransferEnd(f,s,src,dst,p); }

    void onFileRequest(CGSim::Job* j, const std::string& file_name, const long long& filesize, 
    const std::unordered_set<std::string>& file_locations, std::string& source_site, 
    CGSim::FileTransferDecisionMode& mode) final override {
        source_site = file_locations.count(j->get_site()) ? j->get_site() : *file_locations.begin();
    }

private:
    std::unique_ptr<DISPATCHER> di = std::make_unique<DISPATCHER>();
    std::unique_ptr<WORKLOAD_MANAGER> wm = std::make_unique<WORKLOAD_MANAGER>();
    std::shared_ptr<OUTPUT> ou = std::make_unique<OUTPUT>();
    std::shared_ptr<POLICY> po = std::make_unique<POLICY>();
};

extern "C" AgenticPolicyExplorationPlugin* createAgenticPolicyExplorationPlugin() {
    return new AgenticPolicyExplorationPlugin;
}
