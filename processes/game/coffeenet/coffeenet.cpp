#include "coffeenet.h"

#include "../game.h"
#include "component/logger.h"
#include "msg/coffeenet_msg.h"
#include "../user/user_manager.h"

namespace game{

Coffeenet::Coffeenet(CoffeenetId coffeenetId, 
                     const char* account,
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
: m_id(coffeenetId), m_account(account), m_name(name), m_simpleName(simpleName), m_ownerName(ownerName), m_ownerTel(ownerTel), m_managerTel(managerTel), m_competitivePlatporm(competitivePlatporm), m_pcNum(pcNum), m_competitiveClub(competitiveClub), m_vip(vip), m_instruction(instruction)
{}

void Coffeenet::online()
{
    LOG_DEBUG("网吧登录, 成功, account={}", m_account);
    //上线处理
    msg::RetCoffLogin send;

    send.id = m_id;
    m_account.copy(send.account, MAX_ACCOUNT_SIZE);
    send.vip = m_vip;
    m_name.copy(send.name, MAX_NAME_SIZE);

    sendtoMe((water::process::MsgCode1)msg::MsgType::coffeenet, (water::process::MsgCode2)msg::CoffeenetMsgType::login, &send, sizeof(send));
}

void Coffeenet::offline()
{
    LOG_DEBUG("网吧下线, id={}, account={}", m_id, m_account);

    //清空包含角色
    for(auto& item : m_users)
    {
        UserManager::me().delUser(item.first);
    }

}

void Coffeenet::addUser(User::Ptr user)
{
    uint64_t id = user->getId();

    m_users[id] = user; 
}

void Coffeenet::delUser(uint64_t userId)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
    {
        LOG_ERROR("网吧中删除角色, 内存错误, 不存在该角色, userId={}", userId);
    }
}

CoffeenetId Coffeenet::getId()
{
    return m_id;
}

std::string Coffeenet::getAccount()
{
    return m_account;
}

const std::string& Coffeenet::getProvince() const
{
    return m_province;
}

const std::string& Coffeenet::getCity() const
{
    return m_city;
}

void Coffeenet::sendtoMe(water::process::MsgCode1 code1, water::process::MsgCode2 code2, const void* data, uint32_t dataSize)
{
    Game::me().sendtoClient(m_id, code1, code2, data, dataSize);
}

}


