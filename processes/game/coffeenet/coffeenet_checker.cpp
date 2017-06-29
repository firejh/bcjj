#include "coffeenet_checker.h"
#include "coffeenet_db.h"

#include "../game.h"
#include "../fight/team_manager.h"

#include "process/process.h"
#include "process/tcp_packet.h"
#include "process/rawmsg_manger.h"
#include "component/logger.h"
#include "component/scope_guard.h"

#include "msg/coffeenet_msg.h"

#include <thread>
#include <cstring>

namespace game{

ClientChecker& ClientChecker::me()
{
    static ClientChecker me;
    return me;
}

void ClientChecker::addUncheckedConnection(water::net::PacketConnection::Ptr conn)
{
    if(conn == nullptr)
        return;

    try
    {
        conn->setNonBlocking();
        conn->setRecvPacket(process::TcpPacket::create());

        ClientInfo info;
        info.addTime =  water::component::Clock::now();
        info.conn = conn;
        m_clients.push_back(info);
        //LOG_DEBUG("客户端连接验证, 新的客户端接入, {}, fd={}", conn->getRemoteEndpoint(), conn->getFD());
    }
    catch(const net::NetException& ex)
    {
        LOG_ERROR("客户端连接验证, conn加入检查失败, {}, {}", conn->getRemoteEndpoint(), ex); 
    }
}

void ClientChecker::timerExec(const component::TimePoint& now)
{
    LockGuard lock(m_clientsLock);
    for(auto it = m_clients.begin(); it != m_clients.end(); it++)
    {
        //LOG_DEBUG("客户端连接验证, 循环检测, {}, fd={}", it->conn->getRemoteEndpoint(), it->conn->getFD());
        //检查超时
        if((std::chrono::duration_cast<std::chrono::minutes>(now - it->addTime)).count() > 5)
        {
            LOG_TRACE("客户端连接超时, 主动断开连接, ep={}", it->conn->getRemoteEndpoint());
            it = m_clients.erase(it);
        } 

        try
        {
            //接收登录信息
            if(!it->conn->tryRecv())
            {
                continue;
            }

            it->addTime = now;//防止数据库回调后此连接已经被超时处理的临界情况，回调后的操作也是本线程操作，不存在异步

            //第以此连接独立解包，登录验证后才能加入连接管理，之后才能统一解包
            water::net::Packet::Ptr packet = it->conn->getRecvPacket();
            it->conn->setRecvPacket(TcpPacket::create());
            water::process::TcpPacket::Ptr tcpPacket = std::static_pointer_cast<water::process::TcpPacket>(packet);
            water::process::RawMsg* rawMsg = reinterpret_cast<water::process::RawMsg*>(tcpPacket->content());
            if(tcpPacket->contentSize() < sizeof(water::process::MsgCode))
            {
                LOG_DEBUG("客户端连接验证, 信息size错误, 小于code 主动断开连接, ep={}", it->conn->getRemoteEndpoint());
                it = m_clients.erase(it);
                continue;
            }
            water::process::MsgCode msgCode = rawMsg->code;
            uint32_t msgDataSize = tcpPacket->contentSize() - sizeof(water::process::RawMsg);

            //得到typ1,type2
            water::process::MsgCode1* code1 = reinterpret_cast<water::process::MsgCode1*>((uint8_t*)&msgCode);
            water::process::MsgCode2* code2 = reinterpret_cast<water::process::MsgCode2*>((uint8_t*)&msgCode + sizeof(MsgCode1));

            //type1有误
            if(*code1 != static_cast<water::process::MsgCode1>(msg::MsgType::coffeenet))
            {
                LOG_DEBUG("客户端连接验证, 信息code1错误, 主动断开连接, ep={}", it->conn->getRemoteEndpoint());
                it = m_clients.erase(it);
                continue;
            }

            //登录
            if(*code2 == static_cast<water::process::MsgCode2>(msg::CoffeenetMsgType::login))
            {
                if(msgDataSize < sizeof(msg::CoffLogin))
                {
                    LOG_DEBUG("客户端连接验证, 登录信息size错误, 主动断开连接, ep={}", it->conn->getRemoteEndpoint());
                    it = m_clients.erase(it);
                    continue;
                }
                const msg::CoffLogin* coffLogin = reinterpret_cast<msg::CoffLogin*>(rawMsg->data);
                login(coffLogin->account, coffLogin->passwd, *it);
                it = m_clients.erase(it);
            }
            //注册
            else if(*code2 == static_cast<water::process::MsgCode2>(msg::CoffeenetMsgType::signIn))
            {
                if(msgDataSize < sizeof(msg::CoffSignIn))
                {
                    LOG_DEBUG("客户端连接验证, 注册信息size错误, 主动断开连接, ep={}", it->conn->getRemoteEndpoint());
                    it = m_clients.erase(it);
                    continue;
                }
                const msg::CoffSignIn* coffSignIn = reinterpret_cast<msg::CoffSignIn*>(rawMsg->data);
                signIn(coffSignIn->coffData.account,
                       coffSignIn->coffData.passwd,
                       coffSignIn->coffData.name,
                       coffSignIn->coffData.simpleName,
                       coffSignIn->coffData.ownerName,
                       coffSignIn->coffData.ownerTel,
                       coffSignIn->coffData.managerTel,
                       coffSignIn->coffData.competitivePlatporm,
                       coffSignIn->coffData.pcNum,
                       coffSignIn->coffData.competitiveClub,
                       coffSignIn->coffData.instruction,
                       *it);
                it = m_clients.erase(it);
            }
            //type2有误
            else
            {
                LOG_DEBUG("客户端连接验证, 信息code2错误, 主动断开连接, ep={}, code2={}", it->conn->getRemoteEndpoint(), *code2);
                continue;
            }

        }
        catch (const net::ReadClosedConnection& ex) 
        {
            LOG_DEBUG("客户端连接验证, 连接关闭失败, 对方提前断开了连接, ep={}", it->conn->getRemoteEndpoint());
            it = m_clients.erase(it);
            continue;
        } 
        catch (const net::NetException& ex)
        {
            LOG_ERROR("客户端连接验证, 网络错误, ep={}, {}", it->conn->getRemoteEndpoint(), ex);
            it = m_clients.erase(it);
            continue;
        }
    }
}

void ClientChecker::login(const char* account, const char* passwd, ClientInfo& clientInfo)
{
    std::string sAccount(account, MAX_ACCOUNT_SIZE < strlen(account) ? MAX_ACCOUNT_SIZE : strlen(account));
    std::string sPasswd(passwd, MAX_PASSWD_SIZE < strlen(passwd) ? MAX_PASSWD_SIZE : strlen(passwd));
    clientInfo.passwd = sPasswd;
    LOG_DEBUG("客户端登录验证, 连接转入m_login, account={}, size={}, passswd={}, size={}", sAccount,strlen( account), passwd, strlen(passwd));
    //如果有在登录的直接返回
    auto it = m_login.find(sAccount);
    if(it != m_login.end())
    {
        LOG_TRACE("客户端登录验证, 同时两个在登录, 无法登录");
        return;
    }
    m_login[sAccount] = clientInfo;
    CoffeenetDb::me().selectCoffeenetInfo(sAccount.c_str());
}

void ClientChecker::loginCheck(const CoffeenetId coffeenetId, 
                               const char* account, 
                               const char* passwd,
                               const char* name, 
                               const char* simpleName, 
                               const char* ownerName, 
                               const char* ownerTel, 
                               const char* managerTel, 
                               const uint8_t competitivePlatporm, 
                               const uint32_t pcNum, 
                               const char* competitiveClub,
                               const uint8_t vip,
                               const char* instruction)
{
    LOG_DEBUG("客户端登录验证, 验证结果, account={}, id={}", account, coffeenetId);
    std::string sAccount(account);
    auto it = m_login.find(sAccount);
    if(it == m_login.end())
    {
        LOG_ERROR("客户端登录验证, 内存错误, 找不到account, account={}", sAccount);
        return;
    }

    if(std::string(passwd) == it->second.passwd && strlen(account) != 0)
    {
        LOG_DEBUG("客户端登录验证, 验证成功, account={}", sAccount);

        //去除已经在线的老用户的所有数据，包括连接管理中的
        CoffeenetManager::me().offlineIfExist(coffeenetId);

        //加入连接管理，注意这里一定要保证连接管理中没有该用户
        e_clientConfirmed(it->second.conn, coffeenetId);

        //加入网吧角色管理，如果有老用户，此时已经被上面的注册清除下线了，这里正常上线
        Coffeenet::Ptr coffeenet = Coffeenet::create(coffeenetId, account, name, simpleName, ownerName, ownerTel, managerTel, competitivePlatporm, pcNum, competitiveClub, vip, instruction);
        CoffeenetManager::me().online(coffeenet);

        //缓存添加
        CoffeenetBaseData data;
        data.id = coffeenetId;
        data.name = name;
        TeamManager::me().insertCoff(data);

        //验证（登录）成功，从验证管理删除
        m_login.erase(it);
    }
    else
    {
        LOG_DEBUG("客户端登录验证, 密码或账户错误, account={}");
        //返回处理
        msg::RetCoffLogin send;
        send.id = 0;
        //这里要直接发送，还不能使用统一发送
        sendtoConn(it->second.conn, (MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::login, &send, sizeof(send));

        //...先放回去，继续等待登录，可断要求
        LockGuard lock(m_clientsLock);
        m_clients.push_back(it->second);

        m_login.erase(it);
    }
}

void ClientChecker::signIn(const char* account, 
                           const char* passwd, 
                           const char* name, 
                           const char* simpleName, 
                           const char* ownerName, 
                           const char* ownerTel, 
                           const char* managerTel, 
                           const uint8_t competitivePlatporm, 
                           const uint32_t pcNum, 
                           const char* competitiveClub,
                           const char* instruction,
                           ClientInfo& clientInfo)
{
    //这里必须转为string或者保证最后一位是'\0'，否则不安全
    std::string sAccount(account, MAX_ACCOUNT_SIZE < strlen(account) ? MAX_ACCOUNT_SIZE : MAX_ACCOUNT_SIZE);
    std::string sPasswd(passwd, MAX_PASSWD_SIZE < strlen(passwd)?MAX_PASSWD_SIZE:strlen(passwd));
    std::string sName(name, MAX_NAME_SIZE < strlen(name)?MAX_NAME_SIZE:strlen(name));
    std::string sSimpleName(simpleName, MAX_NAME_SIZE < strlen(simpleName)?MAX_NAME_SIZE:strlen(simpleName));
    std::string sOwnerName(ownerName, MAX_NAME_SIZE < strlen(ownerName)?MAX_NAME_SIZE:strlen(ownerName));
    std::string sOwnerTel(ownerTel, MAX_TEL_SIZE<strlen(ownerTel)?MAX_TEL_SIZE:strlen(ownerTel));
    std::string sManagerTel(managerTel, MAX_TEL_SIZE<strlen(managerTel)?MAX_TEL_SIZE:strlen(managerTel));
    std::string sCompetitiveClub(competitiveClub, MAX_NAME_SIZE<strlen(competitiveClub)?MAX_NAME_SIZE:strlen(competitiveClub));
    std::string sInstruction(instruction, MAX_INSTRUCTION_SIZE<strlen(instruction)?MAX_INSTRUCTION_SIZE:strlen(instruction));

    m_signIn[sAccount] = clientInfo;

    CoffeenetDb::me().signInCoffeenetInfo(sAccount, sPasswd, sName, sSimpleName, sOwnerName, sOwnerTel, sManagerTel, competitivePlatporm, pcNum, sCompetitiveClub, sInstruction);

}

void ClientChecker::signInCheck(const char* account, uint8_t result)
{
    LOG_DEBUG("客户端注册验证, 验证结果, account={}", account);
    std::string sAccount(account, MAX_ACCOUNT_SIZE);
    auto it = m_signIn.find(sAccount);
    if(it == m_signIn.end())
    {
        LOG_ERROR("客户端注册验证, 内存错误, 找不到account, account={}", sAccount);
        return;
    }

    msg::RetCoffSignIn send;
    if(result == (uint8_t)0)
    {//成功
        send.result = 1;//成功 
    }
    else
    {//已经注册
        send.result = 2;//已经注册    
    }
    
    //这里要直接发送，还不能使用统一发送
    sendtoConn(it->second.conn, (MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::signIn, &send, sizeof(send));

    //加锁，将注册完的连接重新放回m_clients，等待登录
    LockGuard lock(m_clientsLock);
    m_clients.push_back(it->second);

    m_signIn.erase(it);
}

void ClientChecker::sendtoConn(water::net::PacketConnection::Ptr conn, MsgCode1 code1, MsgCode2 code2, void* data, uint32_t dataSize){
    MsgCode code = RawmsgManager::me().getCode(code1, code2);
    const uint32_t bufSize = sizeof(MsgCode) + dataSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    RawMsg* msg = new(buf) RawMsg(code);
    memcpy(msg->data, data, dataSize);
    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(msg, bufSize);
    
    if(conn->setSendPacket(packet))
    {
        try
        {
            if(conn->trySend())
            {
                uint64_t id= *(uint64_t*)((uint8_t*)data+8);
                LOG_DEBUG("注册和登录, 发送消息成功, contentSize={}, id={}", dataSize, id);
                return;
            }
            else
                LOG_ERROR("注册和登录, 发送消息失败, trySend失败");
        }
        catch (const net::ReadClosedConnection& ex) 
        {
            LOG_DEBUG("注册和登录, 发送消息失败, ep={}, {}", conn->getRemoteEndpoint(), ex);
        } 
        catch (const net::NetException& ex)
        {
            LOG_ERROR("注册和登录, 网络错误, ep={}, {}", conn->getRemoteEndpoint(), ex);
        }
    }
    else
        LOG_ERROR("注册和登录, 发送消息失败, setSendPacket失败");
}

}
