#include "process_config.h"

#include "../component/logger.h"
#include "../component/xmlparse.h"
#include "../component/exception.h"
#include "../component/string_kit.h"

namespace water{
namespace process{

ProcessConfig::ProcessConfig(const std::string& name, const uint64_t ProcessId): m_myName(name), m_ProcessId(ProcessId)
{
}

void ProcessConfig::load(const std::string& cfgDir)
{
    using component::XmlParseDoc;
    using component::XmlParseNode;

    const std::string configFile = cfgDir + "/process.xml";

    LOG_TRACE("读取主配置文件 {}", configFile);

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if(!root)
        EXCEPTION(LoadProcessConfigFailed, configFile + " parse root node failed");    

    {//解析数据库配置
        XmlParseNode dbNode = root.getChild("mysql");
        if(!dbNode)
            EXCEPTION(LoadProcessConfigFailed, "mysql node not exisit");
        m_dbName = dbNode.getAttr<std::string>("dbName"); 
        m_dbHost = dbNode.getAttr<std::string>("host");
        m_dbUser = dbNode.getAttr<std::string>("userName");
        m_dbPasswd = dbNode.getAttr<std::string>("pwd");
        m_dbPort = dbNode.getAttr<uint16_t>("port");

    }

    {//解析所有进程私网监听
        XmlParseNode allProcessesNode = root.getChild("allProcesses");
        if(!allProcessesNode)
            EXCEPTION(LoadProcessConfigFailed, "allProcesses node not exisit");

        XmlParseNode theProcessNode = XmlParseNode::getInvalidNode();
        for(XmlParseNode processTypeNode = allProcessesNode.getChild("processType"); processTypeNode; ++processTypeNode)
        {
            //进程名称和id
            auto name = processTypeNode.getAttr<std::string>("name");
            auto id = processTypeNode.getAttr<uint64_t>("id");
            LOG_DEBUG("id={}", id);

            //进程内地址
            XmlParseNode privateNode = processTypeNode.getChild("private");
            if(!privateNode)
                EXCEPTION(ProcessCfgNotExisit, "process cfg {} do not has {} node", m_ProcessId, "private");
            auto endPointStr = privateNode.getAttr<std::string>("listen");
            std::shared_ptr<net::Endpoint> privateListen;
            if(!endPointStr.empty())
            {
                privateListen.reset(new net::Endpoint(endPointStr));
                auto temp = std::pair<ProcessId, net::Endpoint>(id, *privateListen);
                auto itallListen = m_allListen.insert(temp);
                if(!itallListen.second)
                    EXCEPTION(ProcessCfgNotExisit, "process cfg {} has one more", id);
            }

            //自己
            if(name == m_myName && m_ProcessId == id)
            {
                theProcessNode = processTypeNode;
                m_myInfo.privateNet.listen = privateListen;
                m_myFullName = component::format("{}-{}", m_myName, m_ProcessId);
            }
        }

        if(!theProcessNode)
        {
            //出错，日志中的processFullName要自己拼
            auto processFullName = component::format("[{}:{}]", m_myName, m_ProcessId);
            EXCEPTION(ProcessCfgNotExisit, "进程{}在配置文件中不存在", processFullName);
        }

        //开始开始解析私网除listen外的部分
        ProcessInfo& info = m_myInfo;
        XmlParseNode privateNode = theProcessNode.getChild("private");
        if(!privateNode)
            EXCEPTION(ProcessCfgNotExisit, "进程{}下缺少private配置", m_ProcessId);
        //私网接出
        parseEndpointList(&info.privateNet.connect, privateNode.getAttr<std::string>("connectProcess"));

        //解析公网
        XmlParseNode publicNode = theProcessNode.getChild("public");
        if(publicNode)
            parseEndpointList(&info.publicNet.listen, publicNode.getAttr<std::string>("listen"));  

        //解析数据库连接
        XmlParseNode dbNode = theProcessNode.getChild("dbconnection");
        if(dbNode)
        {
            std::string readStr = dbNode.getAttr<std::string>("read");
            std::string writeStr = dbNode.getAttr<std::string>("write");
            m_dbRead = (readStr == "true") ? true : false;
            m_dbWrite = (writeStr == "true") ? true : false;
        }
    }

}

const std::string& ProcessConfig::getFullName() const
{
    return m_myFullName; 
}

ProcessId ProcessConfig::getId() const
{
    return m_ProcessId;
}

const ProcessConfig::ProcessInfo& ProcessConfig::getInfo()
{
    return m_myInfo;
}

bool ProcessConfig::getDbWrite()
{
    return m_dbWrite;
}

bool ProcessConfig::getDbRead()
{
    return m_dbRead;
}

const std::string& ProcessConfig::getDbHost()
{
    return m_dbHost;
}

const std::string& ProcessConfig::getDbUser()
{
    return m_dbUser;
}

const std::string& ProcessConfig::getDbName()
{
    return m_dbName;
}

const std::string& ProcessConfig::getDbpasswd()
{
    return m_dbPasswd;
}

const uint16_t ProcessConfig::getDbPort()
{
    return m_dbPort;
}

void ProcessConfig::parseEndpointList(std::set<net::Endpoint>* ret, const std::string& str)
{
    ret->clear();
    std::vector<std::string> endpointStrs = component::splitString(str, " ");
    for(const auto& endpointStr : endpointStrs)
    {
        net::Endpoint ep(endpointStr);
        ret->insert(ep);
    }
}

}}
