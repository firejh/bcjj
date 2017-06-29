/*
 * Author: JiangHeng
 *
 * Created: 2016-11-03 09:50 +0800
 *
 * Modified: 2016-11-03 09:50 +0800
 *
 * Description: 
 */
#ifndef BASE_PROCESS_PROCESS_H
#define BASE_PROCESS_PROCESS_H

#include "tcp_server.h"
#include "tcp_client.h"
#include "process_config.h"
#include "tcp_connection_manager.h"
#include "process_timer.h"
#include "private_connection_checker.h"
#include "db_connection.h"

namespace water{
namespace process{

class Process
{
public:
    Process(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir);
    virtual ~Process();

    void start();
    void stop();
    const std::string& getFullName() const;
    ProcessId getId() const;
    void regTimer(std::chrono::milliseconds interval, const ProcessTimer::EventHandler& handler);

    void execDbWriteSql(std::string sql);
    void execDbReadSql(std::string sql, DbConnection::EventHandler handerler, uint64_t data = 0);
private:
    void dealTcpPackets(const component::TimePoint& now);
    virtual void tcpPacketHandle(TcpPacket::Ptr packet, 
                                  TcpConnectionManager::ConnectionHolder::Ptr conn, 
                                  const component::TimePoint& now);

protected:
    virtual void init();
    virtual void lanchThreads();
    virtual void joinThreads();

protected:
    //相关的配置
    const std::string m_cfgDir;
    ProcessConfig m_cfg;
    const std::string m_logDir;
    
    //所有子线程
    std::map<std::string, ProcessThread*> m_threads;
    //数据库
    DbServer::Ptr m_dbWrite;
    DbServer::Ptr m_dbRead;
    //私网
    TcpServer::Ptr m_privateNetServer;
    TcpClient::Ptr m_privateNetClient;
    //公网
    TcpServer::Ptr m_publicNetServer;
    //连接检查器
    ConnectionChecker::Ptr m_privateConnChecker;
    //连接管理器，提供消息接收和发送的epoll事件
    TcpConnectionManager m_conns;
    //主定时器，处理一切业务处理
    ProcessTimer m_timer;
};

}}

#endif
