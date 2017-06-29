#include "process.h"
#include "rawmsg_manger.h" 
#include "signal_handler.h"

#include "../component/scope_guard.h"
#include "../component/logger.h"

namespace water{
namespace process{

Process::Process(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir):
m_cfgDir(configDir), m_cfg(name, processId), m_logDir(logDir)
{
    
}

Process::~Process()
{
    m_threads.clear();
}

void Process::start()
{
    try
    {
        //init()为虚函数， 不能在constructor中调用
        init();

        ON_EXIT_SCOPE_DO(SignalHandler::resetSignalHandle({SIGINT, SIGTERM}));

        lanchThreads();

        joinThreads();
    }
    catch (const component::ExceptionBase& ex)
    {
        LOG_ERROR("process {} start, fatal error: [{}]", getFullName(), ex.what());
        stop();
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("process {} start, fatal error: [{}]", getFullName(), ex.what());
        stop();
    }
    catch (...)
    {
        LOG_ERROR("process {} start, unkonw error", getFullName());
    }
    LOG_TRACE("{} exited", getFullName());
    LOG_CLEAR_FILE;
}

void Process::stop()
{
    LOG_TRACE("exiting ...");
    for(auto& item : m_threads)
    {
        item.second->stop();
    }
    LOG_TRACE("stoped");
}

const std::string& Process::getFullName() const
{
    return m_cfg.getFullName();
}

ProcessId Process::getId() const
{
    return m_cfg.getId();
}

void Process::execDbWriteSql(std::string sql)
{
    if(m_dbWrite == nullptr)
    {
        LOG_ERROR("execDbWriteSql，m_dbWrite not runningi, sql={}", sql);
        return;
    }
    m_dbWrite->exeStoreSql(sql, nullptr);
}

void Process::execDbReadSql(std::string sql, DbConnection::EventHandler handerler, uint64_t data)
{
    if(m_dbRead == nullptr)
    {
        LOG_ERROR("execDbReadSql，m_dbread not runningi, sql={}", sql);
        return;
    }
    m_dbRead->exeStoreSql(sql, handerler, data);
}

void Process::init()
{
    //处理linux信号
    SignalHandler::setSignalHandle({SIGINT, SIGTERM}, std::bind(&Process::stop, this));
    //配置解析

    m_cfg.load(m_cfgDir);

    //指定日志文件
    LOG_ADD_FILE(m_logDir + "/" + getFullName() + ".log"); 

    {//初始化各个组件

        //数据库线程
        if(m_cfg.getDbRead())
        {
            m_dbRead = DbConnection::create(m_cfg.getDbHost(), m_cfg.getDbUser(), m_cfg.getDbpasswd(), m_cfg.getDbName(), m_cfg.getDbPort());
            m_dbRead->init();
        }
        if(m_cfg.getDbWrite())
        {
            m_dbWrite = DbConnection::create(m_cfg.getDbHost(), m_cfg.getDbUser(), m_cfg.getDbpasswd(), m_cfg.getDbName(), m_cfg.getDbPort());
            m_dbWrite->init();
        }

        const ProcessConfig::ProcessInfo& cfg = m_cfg.getInfo();
        const auto& privateNet = cfg.privateNet;
        const auto& publicNet = cfg.publicNet;

        //私网连接检查
        m_privateConnChecker = PrivateConnectionChecker::create(getId());
        //私网监听
        if(privateNet.listen != nullptr)
        {
            m_privateNetServer = TcpServer::create();
            m_privateNetServer->addLocalEndpoint(*privateNet.listen);
        }

        //私网连出
        if(!privateNet.connect.empty())
        {
            m_privateNetClient = TcpClient::create();
            for(const net::Endpoint& ep : privateNet.connect)
                m_privateNetClient->addRemoteEndpoint(ep, std::chrono::seconds(5));
        }

        //公网监听
        if(!publicNet.listen.empty())
        {
            m_publicNetServer = TcpServer::create();
            for(const net::Endpoint& ep : publicNet.listen)
                m_publicNetServer->addLocalEndpoint(ep);
        }
    }

    {//绑定各种核心事件的处理函数
        using namespace std::placeholders;
        //私网的新连接, 放入连接检查器
        if(m_privateNetServer)
        {
            auto checker = std::bind(&ConnectionChecker::addUncheckedConnection
                                     , m_privateConnChecker
                                     , _1
                                     , ConnectionChecker::ConnType::in);
            m_privateNetServer->e_newConn.reg(checker);
        }
        if(m_privateNetClient)
        {
            auto checker = std::bind(&ConnectionChecker::addUncheckedConnection
                                     , m_privateConnChecker
                                     , _1
                                     , ConnectionChecker::ConnType::out);
            m_privateNetClient->e_newConn.reg(checker);
        }

        if(m_publicNetServer)
        {
            //具体登录逻辑来检测，没法在这里做
        }

        //通过检查的连接加入连接管理器
        m_privateConnChecker->e_connConfirmed.reg(std::bind(&TcpConnectionManager::addPrivateConnection, 
                                                            &m_conns, _1, _2));
        //处理消息接收队列
        regTimer(std::chrono::milliseconds(20), std::bind(&Process::dealTcpPackets, this, _1));

        //数据库定时注册
        if(m_dbRead)
            regTimer(std::chrono::milliseconds(50), std::bind(&DbServer::callBackTimerExec, m_dbRead));
        if(m_dbWrite)
            regTimer(std::chrono::milliseconds(50), std::bind(&DbServer::callBackTimerExec, m_dbWrite));
    }
}

void Process::lanchThreads()
{
    if(m_dbRead)
    {
        m_dbRead->run();
        const std::string name = "db read";
        m_threads.insert({name, m_dbRead.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    if(m_dbWrite)
    {
        m_dbWrite->run();
        const std::string name = "db write";
        m_threads.insert({name, m_dbWrite.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    if(m_privateNetServer)
    {
        m_privateNetServer->run();
        const std::string name = "private server";
        m_threads.insert({name, m_privateNetServer.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    if(m_privateNetClient)
    {
        m_privateNetClient->run();
        const std::string name = "private client";
        m_threads.insert({name, m_privateNetClient.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    if(m_publicNetServer)
    {
        m_publicNetServer->run();
        const std::string name =  "public server";
        m_threads.insert({name, m_publicNetServer.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    if(m_privateConnChecker)
    {
        m_privateConnChecker->run();
        const std::string name = "connection checker";
        m_threads.insert({name, m_privateConnChecker.get()});
        LOG_DEBUG("{} thread start ok", name);
    }
    {
        m_conns.run();
        const std::string name = "tcp conns";
        m_threads.insert({name, &m_conns});
        LOG_DEBUG("{} thread start ok", name);
    }
    {
        m_timer.suspend(); //设为挂起状态
        m_timer.run();
        const std::string name = "main timer";
        m_threads.insert({name, &m_timer});
        LOG_DEBUG("{} thread start ok", name);
    } 
}

void Process::joinThreads()
{
    while(!m_threads.empty())
    {
        if(m_timer.isSuspend())
        {
            const ProcessConfig::ProcessInfo& cfg = m_cfg.getInfo();
            //发起私网连接全部成功，主定时器开始执行
            if(m_conns.totalPrivateConnNum() == cfg.privateNet.connect.size())
            {
                LOG_DEBUG("{} lanuch successful", getFullName());

                //启动成功，关掉标准输出日志
                LOG_CLEAR_STD;

                //touch启动好的进程
                std::string shell_cmd = "touch " + m_logDir + "/touchfile/" + getFullName();
                system(shell_cmd.c_str());

                //主定时器恢复
                m_timer.resume();
            }
        }

        for(auto& threadInfo : m_threads)
        {
            const std::string& name = threadInfo.first;
            ProcessThread* thread = threadInfo.second;

            bool threadRetValue;
            const auto waitRet = thread->wait(&threadRetValue, std::chrono::milliseconds(0));
            if(waitRet == ProcessThread::WaitRet::timeout)
                continue;

            if(threadRetValue)
            {
                LOG_TRACE("thread {}  stoped", name, getFullName());
            }
            else
            {
                LOG_ERROR("thread {}  abort", name, getFullName());
                stop();
            }
            m_threads.erase(name);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Process::regTimer(std::chrono::milliseconds interval, const ProcessTimer::EventHandler& handler)
{
    m_timer.regEventHandler(interval, handler);
}
 
void Process::dealTcpPackets(const component::TimePoint& now)
{
    TcpConnectionManager::ConnectionHolder::Ptr conn;
    net::Packet::Ptr packet;
    while(m_conns.getPacket(&conn, &packet))
    {
        TcpPacket::Ptr tcpPacket = std::static_pointer_cast<TcpPacket>(packet);

        tcpPacketHandle(tcpPacket, conn, now);
    }
}

void Process::tcpPacketHandle(TcpPacket::Ptr packet,TcpConnectionManager::ConnectionHolder::Ptr conn, const component::TimePoint& now)
{
    if(packet == nullptr)
        return;
    auto rawMsg = reinterpret_cast<RawMsg*>(packet->content()); 
    RawmsgManager::me().dealTcpMsg(rawMsg, packet->contentSize(), conn->id, now);
}

}}
