/*
 * Author: JiangHeng
 *
 * Created: 2016-11-01 19:42 +0800
 *
 * Modified: 2016-11-01 19:42 +0800
 *
 * Description: 读取架构配置，各个进程之间可以根据配置互相通信
 */

#include "../component/exception.h"
#include "../net/endpoint.h"

#include <map>
#include <set>

#ifndef BASE_PROCESS_CONFIG
#define BASE_PROCESS_CONFIG

namespace water{
namespace process{

DEFINE_EXCEPTION(LoadProcessConfigFailed, component::ExceptionBase)
DEFINE_EXCEPTION(ProcessCfgNotExisit, component::ExceptionBase)

using ProcessId = uint64_t;
class ProcessConfig final
{
public:
    struct ProcessInfo
    {
        //私网路由
        struct
        {
            std::shared_ptr<net::Endpoint> listen;

            std::set<net::Endpoint> connect;
        } privateNet;

        //公网路由
        struct
        {
            std::set<net::Endpoint> listen;
        } publicNet;

    };
    //构造函数，参数是自身进程的名称
    ProcessConfig(const std::string& name, const uint64_t ProcessId);
    void load(const std::string& cfgDir);
    const ProcessInfo& getInfo();
    const std::string& getFullName() const;
    ProcessId getId() const;
    bool getDbWrite();
    bool getDbRead();
    const std::string& getDbHost();
    const std::string& getDbUser();
    const std::string& getDbName();
    const std::string& getDbpasswd();
    const uint16_t getDbPort();

private:
    void parseEndpointList(std::set<net::Endpoint>* ret, const std::string& str);

private:
    std::string m_myFullName;
    std::string m_myName;
    uint64_t m_ProcessId;
    ProcessInfo m_myInfo;
    //数据库
    std::string m_dbHost;
    std::string m_dbUser;
    std::string m_dbName;
    std::string m_dbPasswd;
    uint16_t m_dbPort;
    bool m_dbRead;
    bool m_dbWrite;
    //私网信息
    std::map<ProcessId, net::Endpoint> m_allListen;
};

}}

#endif
