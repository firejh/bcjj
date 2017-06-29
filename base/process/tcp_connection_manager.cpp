#include "../component/logger.h"
#include "tcp_connection_manager.h"

namespace water{
namespace process{

void TcpConnectionManager::addPrivateConnection(net::PacketConnection::Ptr conn, 
                                                ClinetIdentity processId)
{
    conn->setNonBlocking();

    auto item = std::make_shared<ConnectionHolder>();
    item->type = ConnType::privateType;
    item->id = processId;
    item->conn = conn;

    std::lock_guard<component::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), item});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_allConns failed, processId={}", processId);
        return;
    }

    auto insertPrivateRet = m_priConns.insert({processId, item});
    if(insertPrivateRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_privateConns failed, processId={}", processId);
        m_allConns.erase(insertAllRet.first);
        return;
    }

    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
        conn->setRecvPacket(TcpPacket::create());
    }
    catch (const component::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to epoller failed, ex={}, processId={}",
                  ex.what(), processId);
        m_allConns.erase(insertAllRet.first);
        m_priConns.erase(insertPrivateRet.first);
    }

    //添加privateConn完成的事件
    e_afterAddPrivateConn(processId);
    LOG_TRACE("ConnectionManager, insert privateConn, fd={}, pid={}, ep={}", 
              conn->getFD(), processId, conn->getRemoteEndpoint()); 
}

//注意任何重复的id无法插入，客户端登录一定要先判断在线情况
bool TcpConnectionManager::addPublicConnection(net::PacketConnection::Ptr conn, ClinetIdentity clientId)
{
    conn->setNonBlocking();
    auto item = std::make_shared<ConnectionHolder>();
    item->type = ConnType::publicType;
    item->id = clientId;
    item->conn = conn;
    
    std::lock_guard<component::Spinlock> lock(m_lock);
    auto insertAllRet = m_allConns.insert({conn->getFD(), item});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert publicConn to m_allConns failed, id={}", clientId);
        return false;
    }
   
    auto insertPublicRet = m_pubConns.insert({clientId, item});
    if(insertPublicRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert publicConn tm_publicConns failed, id={}", clientId);
        m_allConns.erase(insertAllRet.first);
        return false;
    }
    
    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
        conn->setRecvPacket(TcpPacket::create());
    }
    catch (const component::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager, insert publicConn to m_publicConns failed, id={}", 
                  ex.what(), clientId);

        m_allConns.erase(insertAllRet.first);
        m_pubConns.erase(insertPublicRet.first);
        return false;
    }
    LOG_TRACE("ConnectionManager, insert publicConn, fd={}, id={}, ep={}", conn->getFD(), clientId, conn->getRemoteEndpoint());
    return true;
}

void TcpConnectionManager::erasePublicConnection(ClinetIdentity clientId)
{
    std::lock_guard<component::Spinlock> lock(m_lock);
    auto it = m_pubConns.find(clientId);
    if(it == m_pubConns.end())
    {
        LOG_TRACE("ConnectionManager, erase publicConn, not exist");
        return;
    }
    m_allConns.erase(it->second->conn->getFD());
    m_pubConns.erase(it);
    LOG_DEBUG("ConnectionManager, erase publicConn, id={}", clientId);
}

bool TcpConnectionManager::exec()
{
    try
    {
        if(!checkSwitch())
            return true;

        //绑定epoll消息处理器
        using namespace std::placeholders;
        m_epoller.setEventHandler(std::bind(&TcpConnectionManager::epollerEventHandler, this, _2, _3));

        while(checkSwitch())
        {
            m_epoller.wait(std::chrono::milliseconds(1000)); //10 milliseconds 一轮
        }
    }
    catch (const net::NetException& ex)
    {
        LOG_ERROR("connManager 异常退出 , {}", ex.what());
        return false;
    }

    return true;
}

bool TcpConnectionManager::getPacket(ConnectionHolder::Ptr* conn, net::Packet::Ptr* packet)
{
    std::pair<ConnectionHolder::Ptr, net::Packet::Ptr> ret;
    if(!m_recvQueue.pop(&ret))
        return false;

    *conn = ret.first;
    *packet = ret.second;
    return true;
}

void TcpConnectionManager::epollerEventHandler(int32_t socketFD, net::Epoller::Event event)
{
    ConnectionHolder::Ptr connHolder;
    {
        std::lock_guard<component::Spinlock> lock(m_lock);
        auto connsIter = m_allConns.find(socketFD);
        if(connsIter == m_allConns.end()) 
        {
            if(net::Epoller::Event::error != event)
            {
                LOG_ERROR("epoll报告一个socket事件，但该socket不在manager中, fd={}, event={}", socketFD, event);
            }
            m_epoller.delSocket(socketFD);
            return;
        }
        connHolder = connsIter->second;
    }
    net::PacketConnection::Ptr conn = connHolder->conn;
    try
    {
        switch (event)
        {
        case net::Epoller::Event::read:
            {
                while(conn->tryRecv())
                {
                    if(!m_recvQueue.push({connHolder, conn->getRecvPacket()}) )
                    {
                        LOG_ERROR("epoll响应后对应的接收队列满, 无法接收, 消息丢失, fd={}, connType={}, clientId={}", 
                                  socketFD, connHolder->type, connHolder->id);
                        break;//队列满，停止处理
                    }
                    conn->setRecvPacket(TcpPacket::create()); //这里，接收逻辑按TcpPacket
                }
            }
            break;
        case net::Epoller::Event::write:
            {
                std::lock_guard<component::Spinlock> lock(connHolder->sendLock);
                while(connHolder->conn->trySend())
                {
                    if(connHolder->sendQueue.empty())
                    {
                        m_epoller.modifySocket(socketFD, net::Epoller::Event::read);
                        break;
                    }

                    net::Packet::Ptr packet = connHolder->sendQueue.front();
                    connHolder->sendQueue.pop_front();
                    conn->setSendPacket(packet);
                }
            }
            break;
        case net::Epoller::Event::error:
            {
                LOG_ERROR("epoll error, {}", conn->getRemoteEndpoint());
                eraseConnection(conn);
                conn->close();
            }
            break;
        default:
            LOG_ERROR("epoll, unexcept event type, {}", conn->getRemoteEndpoint());
            break;
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        LOG_TRACE("对方断开连接, {}, fd={}", conn->getRemoteEndpoint(), conn->getFD());
        eraseConnection(conn);
        conn->close();
    }
    catch (const net::NetException& ex)
    {
        LOG_TRACE("连接异常, {}, {}", conn->getRemoteEndpoint(), ex.what());
        eraseConnection(conn);
        conn->close();
    }
}

void TcpConnectionManager::eraseConnection(net::PacketConnection::Ptr conn)
{
    std::lock_guard<component::Spinlock> lock(m_lock);

    auto it = m_allConns.find(conn->getFD());
    if(it == m_allConns.end())
        return;

    auto publicIt = m_pubConns.find(it->second->id);
    if(publicIt != m_pubConns.end())
    {
        LOG_TRACE("ConnectionManager, erase publicConn, socketFd={}, type={}, id={}, ep={}", 
                  conn->getFD(), it->second->type, it->second->id, conn->getRemoteEndpoint());
        m_pubConns.erase(publicIt);
        e_afterErasePublicConn(it->second->id);
    }
    else
    {
        auto iter = m_priConns.find(it->second->id);
        if(iter != m_priConns.end())
        {
            LOG_TRACE("ConnectionManager, erase privateConn, socketFd={}, type={}, id={}, ep={}", 
                      conn->getFD(), it->second->type, it->second->id, conn->getRemoteEndpoint());
            m_priConns.erase(iter);
            e_afterErasePrivateConn(it->second->id);
        }
        else
        {
            LOG_ERROR("ConnectionManager, erase conn, notPublic and notPrivate, socketFd={}, type={}, id={}, ep={}", 
                      conn->getFD(), it->second->type, it->second->id, conn->getRemoteEndpoint());
        }
    }
    m_epoller.delSocket(conn->getFD());
    m_allConns.erase(it);
}

uint32_t TcpConnectionManager::totalPrivateConnNum() const
{
    return m_priConns.size();
}

bool TcpConnectionManager::sendPacketToPrivate(ClinetIdentity processId, net::Packet::Ptr packet)
{
    std::lock_guard<component::Spinlock> lock(m_lock);

    auto iterPri = m_priConns.find(processId);
    if(iterPri == m_priConns.end())
    {
        LOG_ERROR("send private packet, 目标进程未连接, processId={}", processId);
        return false;
    }

    try
    {
        sendPacket(iterPri->second, packet);
    }
    catch (const component::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, remoteProcessId={}", ex.what(), processId);
        return false;
    }
    return true; 
}

bool TcpConnectionManager::sendPacketToPublic(ClinetIdentity clientId, net::Packet::Ptr packet)
{
    std::lock_guard<component::Spinlock> lock(m_lock);
    auto it = m_pubConns.find(clientId);
    if(it == m_pubConns.end())
    {
        LOG_ERROR("send packet to public, socket not found, clientId={}", clientId);
        return false;
    }

    try
    {
       return  sendPacket(it->second, packet);
    }
    catch (const component::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, id={}", ex.what(), clientId);
        return false;
    }
    return true;
}

bool TcpConnectionManager::sendPacket(ConnectionHolder::Ptr connHolder, net::Packet::Ptr packet)
{
    std::lock_guard<component::Spinlock> lock(connHolder->sendLock);
    if(!connHolder->sendQueue.empty()) //待发送队列非空, 直接排队
    {
        if(connHolder->sendQueue.size() > 512)
            return false;

        connHolder->sendQueue.push_back(packet);
        LOG_TRACE("发送过慢，发送队列出现排队, connId:{}", connHolder->id);
        return true;
    }

    if(connHolder->conn->setSendPacket(packet))
    {
        if(connHolder->conn->trySend())
            return true; //直接发送成功

        //setPaccket成了，但trySend没成，注册epoll_write事件
        m_epoller.modifySocket(connHolder->conn->getFD(), net::Epoller::Event::read_write);
        return true;
    }

    //setPacket失败，packet未被接受，需要排队
    connHolder->sendQueue.push_back(packet);

    //注册epoll事件
    m_epoller.modifySocket(connHolder->conn->getFD(), net::Epoller::Event::read_write);
    return true;
}

}}
