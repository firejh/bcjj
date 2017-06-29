/*
 * Author: JiangHeng
 *
 * Created: 2016-11-07 17:00 +0800
 *
 * Modified: 2016-11-07 17:00 +0800
 *
 * Description: 负责客户端连接验证和登录
 */
#ifndef PROCESSES_COFFEENET_CHECKER_H
#define PROCESSES_COFFEENET_CHECKER_H

#include "coffeenet_manager.h"

#include "component/spinlock.h"
#include "component/datetime.h"
#include "net/packet_connection.h"

#include <list>
#include <map>
#include <mutex>

namespace game
{

using namespace water;
using namespace component;

class ClientChecker
{
    struct ClientInfo
    {
        water::component::TimePoint addTime;//用于超时处理
        std::string passwd;//密码
        water::net::PacketConnection::Ptr conn;
    };

public:
    TYPEDEF_PTR(ClientChecker)
    CREATE_FUN_MAKE(ClientChecker)
    ~ClientChecker(){};
    static ClientChecker& me();

    //添加一个链接
    void addUncheckedConnection(water::net::PacketConnection::Ptr conn);
    //定时做连接验证处理
    void timerExec(const component::TimePoint& now);

    //登录回调验证
    void loginCheck(const CoffeenetId coffeenetId, 
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
                    const char* instruction);
    //注册回调验证
    void signInCheck(const char* account, uint8_t result);
private:
    //构造私有
    ClientChecker(){};

    //登录
    void login(const char* account, const char* passwd, ClientInfo& clientInfo);

    //注册
    void signIn(const char* account, 
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
                ClientInfo& clientInfo);

    //给连接发送消息
    void sendtoConn(water::net::PacketConnection::Ptr conn, MsgCode1 code1, MsgCode2 code2, void* data, uint32_t dataSize);
public:
    component::Event<void (water::net::PacketConnection::Ptr, CoffeenetId)> e_clientConfirmed;

private:
    typedef std::lock_guard<component::Spinlock> LockGuard;

    //新连接
    std::list<ClientInfo> m_clients;
    component::Spinlock m_clientsLock;

    //登录用户
    std::map<std::string, ClientInfo> m_signIn;
    std::map<std::string, ClientInfo> m_login;
};

}

#endif
