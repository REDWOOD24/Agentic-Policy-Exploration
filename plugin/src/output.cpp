#include "output.h"
#include <filesystem>

void OUTPUT::initialize()
{
    if (initialized) return;

    auto file = rm->get_custom_parameter("output_file");
    if (std::filesystem::exists(file)) std::filesystem::remove(file);

    if (sqlite3_open(file.c_str(), &db) != SQLITE_OK)
        throw std::runtime_error("Cannot open SQLite file");

    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db,
        "CREATE TABLE EVENTS ("
        "_ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "EVENT TEXT, STATE TEXT, STATUS TEXT, JOB_ID TEXT,"
        "TIME REAL, METADATA TEXT);",
        nullptr, nullptr, nullptr);

    initialized = true;
}

void OUTPUT::insert_event(const std::string& event,
                          const std::string& state,
                          const std::string& job,
                          const std::string& status,
                          double time,
                          const std::string& payload)
{
    sqlite3_stmt* s;
    sqlite3_prepare_v2(db,
        "INSERT INTO EVENTS(EVENT,STATE,JOB_ID,STATUS,TIME,METADATA)"
        " VALUES(?,?,?,?,?,?)", -1, &s, nullptr);

    sqlite3_bind_text(s,1,event.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(s,2,state.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(s,3,job.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(s,4,status.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(s,5,time);
    sqlite3_bind_text(s,6,payload.c_str(),-1,SQLITE_TRANSIENT);

    if (sqlite3_step(s) != SQLITE_DONE) {
        sqlite3_finalize(s);
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(s);
}

void OUTPUT::onSimulationEnd()
{
    sqlite3_close(db);
}

void OUTPUT::onJobTransferStart(CGSim::Job* j)
{
    insert_event("JobAllocation","Started",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{{"site",j->get_site()},{"host",j->get_cpu()}}.dump());
}

void OUTPUT::onJobTransferEnd(CGSim::Job* j)
{
    auto* s = rm->get_site(j->get_site());

    insert_event("JobAllocation","Finished",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{
            {"site",j->get_site()},
            {"host",j->get_cpu()},
            {"site_storage_util",s->get_storage_utilization()},
            {"grid_storage_util",rm->get_grid_storage_utilization()},
            {"site_cpu_util",s->get_cpu_utilization()},
            {"grid_cpu_util",rm->get_grid_cpu_utilization()}
        }.dump());
}

void OUTPUT::onJobExecutionStart(CGSim::Job* j)
{
    insert_event("JobExecution","Started",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{
            {"flops",j->get_flops()},
            {"site",j->get_site()},
            {"host",j->get_cpu()},
            {"cores",j->get_cores()},
            {"speed",j->get_cpu_speed()},
            {"site_cpu_util",rm->get_site(j->get_site())->get_cpu_utilization()},
            {"grid_cpu_util",rm->get_grid_cpu_utilization()}
        }.dump());
}

void OUTPUT::onJobExecutionEnd(CGSim::Job* j)
{
    insert_event("JobExecution","Finished",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{
            {"flops",j->get_flops()},
            {"cores",j->get_cores()},
            {"site",j->get_site()},
            {"host",j->get_cpu()},
            {"speed",j->get_cpu_speed()},
            {"duration",j->get_cpu_consumption_time()},
            {"retries",j->get_retries()},
            {"total_io_read_time",j->get_total_io_read_time()},
            {"total_io_write_time",j->get_total_io_write_time()},
            {"file_transfer_queue_time",j->get_file_transfer_queue_time()},
            {"resource_waiting_queue_time",j->get_resource_waiting_queue_time()},
            {"site_cpu_util",rm->get_site(j->get_site())->get_cpu_utilization()},
            {"grid_cpu_util",rm->get_grid_cpu_utilization()}
        }.dump());
}

void OUTPUT::onFileTransferStart(CGSim::Job* j, const std::string& f,
                                 unsigned long long size,
                                 const std::string& src,
                                 const std::string& dst)
{
    insert_event("FileTransfer","Started",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{
            {"file",f},{"size",size},
            {"source_site",src},{"destination_site",dst},
            {"src_storage_util",rm->get_site(src)->get_storage_utilization()},
            {"dst_storage_util",rm->get_site(dst)->get_storage_utilization()},
            {"grid_storage_util",rm->get_grid_storage_utilization()}
        }.dump());
}

void OUTPUT::onFileTransferEnd(CGSim::Job* j, const std::string& f,
                               unsigned long long size,
                               const std::string& src,
                               const std::string& dst)
{
    insert_event("FileTransfer","Finished",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{
            {"file",f},{"size",size},
            {"source_site",src},{"destination_site",dst},
            {"file_transfer_time",j->get_file_transfer_queue_time()},
            {"src_storage_util",rm->get_site(src)->get_storage_utilization()},
            {"dst_storage_util",rm->get_site(dst)->get_storage_utilization()},
            {"grid_storage_util",rm->get_grid_storage_utilization()}
        }.dump());
}

void OUTPUT::onFileReadStart(CGSim::Job* j, const std::string& f,
                             unsigned long long size)
{
    insert_event("FileRead","Started",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{{"file",f},{"size",size},{"site",j->get_site()},
             {"host",j->get_cpu()},{"disk",j->get_disk()},
             {"disk_read_bw",j->get_disk_read_bw()}}.dump());
}

void OUTPUT::onFileReadEnd(CGSim::Job* j, const std::string& f,
                           unsigned long long size)
{
    insert_event("FileRead","Finished",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{{"file",f},{"size",size},{"site",j->get_site()},
             {"host",j->get_cpu()},{"disk",j->get_disk()},
             {"total_io_read_time",j->get_total_io_read_time()}}.dump());
}

void OUTPUT::onFileWriteStart(CGSim::Job* j, const std::string& f,
                              unsigned long long size)
{
    insert_event("FileWrite","Started",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{{"file",f},{"size",size},{"site",j->get_site()},
             {"host",j->get_cpu()},{"disk",j->get_disk()},
             {"disk_write_bw",j->get_disk_write_bw()}}.dump());
}

void OUTPUT::onFileWriteEnd(CGSim::Job* j, const std::string& f,
                            unsigned long long size)
{
    insert_event("FileWrite","Finished",j->get_id(),j->get_status(),
        CGSim::Utilities::get_simulation_clock(),
        json{{"file",f},{"size",size},{"site",j->get_site()},
             {"host",j->get_cpu()},{"disk",j->get_disk()},
             {"total_io_write_time",j->get_total_io_write_time()},
             {"site_storage_util",rm->get_site(j->get_site())->get_storage_utilization()},
             {"grid_storage_util",rm->get_grid_storage_utilization()}}.dump());
}

void OUTPUT::onUserFileTransferStart(const std::string& f,
                                     unsigned long long size,
                                     const std::string& src,
                                     const std::string& dst,
                                     const std::string& policy)
{
    insert_event("UserFileTransfer","Started","","none",CGSim::Utilities::get_simulation_clock(),
        json{{"policy",policy},{"file",f},{"size",size},
             {"source_site",src},{"destination_site",dst}}.dump());
}

void OUTPUT::onUserFileTransferEnd(const std::string& f,
                                   unsigned long long size,
                                   const std::string& src,
                                   const std::string& dst,
                                   const std::string& policy)
{
    insert_event("UserFileTransfer","Finished","","none",CGSim::Utilities::get_simulation_clock(),
        json{{"policy",policy},{"file",f},{"size",size},
             {"source_site",src},{"destination_site",dst},
             {"src_storage_util",rm->get_site(src)->get_storage_utilization()},
             {"dst_storage_util",rm->get_site(dst)->get_storage_utilization()},
             {"grid_storage_util",rm->get_grid_storage_utilization()}}.dump());
}
