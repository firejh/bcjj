/*
 * Author: JiangHeng
 *
 * Created: 2016-11-03 10:51 +0800
 *
 * Modified: 2016-11-03 10:51 +0800
 *
 * Description: 
 */
#ifndef BASE_PROCESS_TCPCONNECTIONMANAGER_H
#define BASE_PROCESS_TCPCONNECTIONMANAGER_H

#include "../net/packet_connection.h"
#include "../net/epoller.h"
#include "../component/event.h"
#include "../component/class_helper.h"
#include "../component/spinlock.h"
#include "../component/lock_free_circular_queue_ss.h"

#include "process_thread.h"
#include "tcp_packet.h"

#include <unordered_map>
#include <map>
#include <list>

namespace water{
namespace process{

class TcpConnectionManager : public ProcessThread
{
public:
    using ClinetIdentity = int64_t;
    enum class ConnType {privateType, publicType};

    struct ConnectionHolder
    {
        TYPEDEF_PTR(ConnectionHolder);
        ConnType type;
        ClinetIdentity id;

        net::PacketConnection::Ptr conn;

        //由于socke太忙而暂时无法发出的包，缓存在这里
        std::list<net::Packet::Ptr> sendQueue; 
        //发包时需要一个锁，因为conn的发送函数和sendQueue都是不可重入的
        component::Spinlock sendLock;
    };

public:
    TYPEDEF_PTR(TcpConnectionManager)
    CREATE_FUN_MAKE(TcpConnectionManager)

    TcpConnectionManager() = default;
    ~TcpConnectionManager() = default;

    bool exec() override;
    void addPrivateConnection(net::PacketConnection::Ptr conn, ClinetIdentity processId);
    bool addPublicConnection(net::PacketConnection::Ptr conn, ClinetIdentity clientId);
    void erasePublicConnection(ClinetIdentity clientId);
    uint32_t totalPrivateConnNum() const;
    bool getPacket(ConnectionHolder::Ptr* conn, net::Packet::Ptr* packet);
    bool sendPacketToPrivate(ClinetIdentity processId, net::Packet::Ptr packet);
    bool sendPacketToPublic(ClinetIdentity clientId, net::Packet::Ptr packet);

public:
    public:
    //添加conn成功的事件
    component::Event<void (ClinetIdentity id)> e_afterAddPrivateConn;

    //删除conn成功的事件
    component::Event<void (ClinetIdentity id)> e_afterErasePrivateConn;
    component::Event<void (ClinetIdentity id)> e_afterErasePublicConn;

private:
    void epollerEventHandler(int32_t socketFD, net::Epoller::Event event);
    void eraseConnection(net::PacketConnection::Ptr conn);

    bool sendPacket(ConnectionHolder::Ptr connHolder, net::Packet::Ptr packet);

private:
    //所有的连接, 以socketFd为索引 {socketFd, conn}
    std::unordered_map<int32_t, ConnectionHolder::Ptr> m_allConns;

    //公网连接
    std::map<ClinetIdentity, ConnectionHolder::Ptr> m_pubConns;
    //私网
    std::unordered_map<ClinetIdentity, ConnectionHolder::Ptr> m_priConns;
    component::LockFreeCircularQueueSPSC<std::pair<ConnectionHolder::Ptr, net::Packet::Ptr>> m_recvQueue;

    component::Spinlock m_lock;
    net::Epoller m_epoller;

};

}}

#endif
