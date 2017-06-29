#include "coffeenet_manager.h"
#include "coffeenet_db.h"

#include "process/rawmsg_manger.h"
#include "component/logger.h"
#include "msg/coffeenet_msg.h"
#include "../game.h"
#include "../user/user_manager.h"

namespace game{

CoffeenetManager& CoffeenetManager::me()
{
    static CoffeenetManager me;
    return me;
}

void CoffeenetManager::regMsgHandler()
{
    using namespace std::placeholders;
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::logoff, std::bind(&CoffeenetManager::logoff, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::areaCoffeenet, std::bind(&CoffeenetManager::areaCoffeenet, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::province, std::bind(&CoffeenetManager::province, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::city, std::bind(&CoffeenetManager::city, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::coffeenet, (MsgCode2)msg::CoffeenetMsgType::area, std::bind(&CoffeenetManager::area, this, _1, _2, _3));
}

Coffeenet::Ptr CoffeenetManager::getCoffeenet(CoffeenetId coffeenetId)
{
    auto it = m_coffeenets.find(coffeenetId);
    if(it == m_coffeenets.end())
        return nullptr;
    return it->second;
}

void CoffeenetManager::online(Coffeenet::Ptr coffeenet)
{
    //这里已经保证不会出现重复的用户，因为登录检查时如果有老用户直接踢掉了
    m_delconnectCoffenetsLock.lock();
    m_coffeenets[coffeenet->getId()] = coffeenet;
    m_delconnectCoffenetsLock.unlock();
    coffeenet->online();
}

void CoffeenetManager::delCoffeenet(CoffeenetId coffeenetId)
{
    //需要加锁，异步
    m_delconnectCoffenetsLock.lock();
    auto it = m_coffeenets.find(coffeenetId);
    if(it == m_coffeenets.end())
    {
        LOG_ERROR("网吧删除, 网吧管理中不存在");
        return;
    }
    Coffeenet::Ptr temp = it->second;
    //删除
    m_coffeenets.erase(it);
    m_delconnectCoffenetsLock.unlock();

    //网吧内部下线处理
    it->second->offline();
}

void CoffeenetManager::offline(CoffeenetId coffeenetId)
{
    auto it = m_coffeenets.find(coffeenetId);
    if(it == m_coffeenets.end())
    {
        LOG_ERROR("网吧删除, 网吧管理中不存在");
        return;
    }

    //网吧内部下线处理
    it->second->offline();

    //网吧连接处理
    Game::me().erasePublicConn(coffeenetId);

    m_coffeenets.erase(it);
}

void CoffeenetManager::offlineIfExist(CoffeenetId coffeenetId)
{
    auto it = m_coffeenets.find(coffeenetId);
    if(it == m_coffeenets.end())
        return;
    LOG_DEBUG("客户端登录, 用户已经在线，执行原用户下线, coffeenetId={}", coffeenetId);
    offline(coffeenetId);
}

void CoffeenetManager::logoff(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
}

void CoffeenetManager::areaCoffeenet(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::AreaCoffeenet))
    {
        LOG_DEBUG("区域网吧查询, 消息size错误", msgSize);
        return;
    }
    const msg::AreaCoffeenet* msg = reinterpret_cast<const msg::AreaCoffeenet*>(msgData);

    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("区域网吧查询, 未登录");
        return;
    }
    CoffeenetDb::me().areaCoffeenet(msg->userId, msg->province, msg->city, msg->area);
}

void CoffeenetManager::province(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::Province))
    {
        LOG_DEBUG("区域查询, 消息size错误", msgSize);
        return;
    }
    const msg::Province* msg = reinterpret_cast<const msg::Province*>(msgData);

    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("区域查询, 未登录");
        return;
    }
    CoffeenetDb::me().province(msg->userId);
}

void CoffeenetManager::city(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::City))
    {
        LOG_DEBUG("区域查询, 消息size错误", msgSize);
        return;
    }
    const msg::City* msg = reinterpret_cast<const msg::City*>(msgData);

    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("区域查询, 未登录");
        return;
    }
    std::string code(msg->code, strlen(msg->code) < MAX_AREA_CODE_SIZE?strlen(msg->code):MAX_AREA_CODE_SIZE);
    CoffeenetDb::me().city(msg->userId, code);
}

void CoffeenetManager::area(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::Area))
    {
        LOG_DEBUG("区域查询, 消息size错误", msgSize);
        return;
    }
    const msg::Area* msg = reinterpret_cast<const msg::Area*>(msgData);

    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("区域查询, 未登录");
        return;
    }
    std::string code(msg->code, strlen(msg->code) < MAX_AREA_CODE_SIZE?strlen(msg->code):MAX_AREA_CODE_SIZE);
    CoffeenetDb::me().area(msg->userId, code);
}

void CoffeenetManager::provinceBack(uint64_t userId, std::vector<AreaData>& data)
{
    User::Ptr user = UserManager::me().getUser(userId);
    if(!user)
    {
        LOG_DEBUG("区域查询检测, 未登录");
        return;
    }

    msg::RetProvince send;
    uint32_t i = 0;
    for(auto& item: data) 
    {
        item.code.copy(send.data[i].code, MAX_AREA_CODE_SIZE);
        memset(send.data[i].name, 0, MAX_NAME_SIZE);
        item.name.copy(send.data[i].name, MAX_NAME_SIZE);
        i++;
        if(i == 5)
        {
            send.userId = userId;
            send.total = data.size();
            sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::province), &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == data.size())
    {
        send.userId = userId;
        send.total = data.size();
        sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::province), &send, sizeof(send));
    }
}

void CoffeenetManager::cityBack(uint64_t userId, std::vector<AreaData>& data)
{
    User::Ptr user = UserManager::me().getUser(userId);
    if(!user)
    {
        LOG_DEBUG("区域查询检测, 未登录");
        return;
    }

    msg::RetCity send;
    uint32_t i = 0;
    for(auto& item: data) 
    {
        item.code.copy(send.data[i].code, MAX_AREA_CODE_SIZE);
        item.name.copy(send.data[i].name, MAX_NAME_SIZE);
        i++;
        if(i == 5)
        {
            send.userId = userId;
            send.total = data.size();
            sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::city), &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == data.size())
    {
        send.userId = userId;
        send.total = data.size();
        sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::city), &send, sizeof(send));
    }
}

void CoffeenetManager::areaBack(uint64_t userId, std::vector<AreaData>& data)
{
    User::Ptr user = UserManager::me().getUser(userId);
    if(!user)
    {
        LOG_DEBUG("区域查询检测, 未登录");
        return;
    }

    msg::RetArea send;
    uint32_t i = 0;
    for(auto& item: data) 
    {
        item.code.copy(send.data[i].code, MAX_AREA_CODE_SIZE);
        item.name.copy(send.data[i].name, MAX_NAME_SIZE);
        i++;
        if(i == 5)
        {
            send.userId = userId;
            send.total = data.size();
            sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::area), &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == data.size())
    {
        send.userId = userId;
        send.total = data.size();
        sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::area), &send, sizeof(send));
    }
}

void CoffeenetManager::areaCoffeenetBack(const uint64_t userId, std::vector<CoffeenetData>& coffDatas)
{
    User::Ptr user = UserManager::me().getUser(userId);
    if(!user)
    {
        LOG_DEBUG("区域网吧查询检测, 未登录");
        return;
    }

    msg::RetAreaCoffeenet send;
    uint32_t i = 0;
    for(auto& item : coffDatas)
    {
        send.coffData[i].coffeenetId = item.coffeenetId;
        item.name.copy(send.coffData[i].name, MAX_NAME_SIZE);

        i++;
        if(i == 5)
        {
            send.userId = userId;
            send.total = coffDatas.size();
            sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::areaCoffeenet), &send, sizeof(send));
            i = 0;
            memset(&send, 0, sizeof(send));
        }
    }

    if(i != 0 || i == coffDatas.size())
    {
        send.userId = userId;
        send.total = coffDatas.size();
        sendtoClient(user->getCoffeenetId(), water::process::MsgCode2(msg::CoffeenetMsgType::areaCoffeenet), &send, sizeof(send));
    }
}

void CoffeenetManager::sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize)
{
    MsgCode1 code1 = (MsgCode1)msg::MsgType::coffeenet;
    Game::me().sendtoClient(coffeenetId, code1, code2, data, dataSize); 
}

}
