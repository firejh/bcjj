#include "user.h"
#include "user_db.h"
#include "user_manager.h"

#include "../fight/team_manager.h"
#include "msg/user_msg.h"

#include "component/logger.h"

namespace game{

User::User(uint64_t id,
     std::string name,
     std::string address,
     std::string tel,
     std::string qq,
     CoffeenetId coffeenetId): m_id(id), m_userName(name), m_userAddress(address), m_tel(tel), m_qq(qq), m_coffeenetId(coffeenetId)
{
    m_lastHeart = time(NULL);
}

void User::del()
{
    LOG_DEBUG("角色被踢, m_id={}", m_id);
}

void User::offline()
{
    LOG_DEBUG("角色下线, m_id={}", m_id);
}

void User::online()
{
    //需要做的上线初始化

    //初始化角色身份认证数据
    select();
    initUserIdentify();

    //初始化角色游戏数据
    select();
    initUserGame();
}

void User::initUserIdentify()
{
    UserDb::me().selectIdentify(m_id);
}

void User::initUserIdentifyCheck(const std::string identify,
                                 const std::string name,
                                 const std::string address,
                                 const std::string tel)
{
    m_identify = identify;
    m_name = name;
    m_acceptAddress = address;
    m_acceptTel = tel;
}

void User::initUserGame()
{
    UserDb::me().selectUserGame(m_id);
}

void User::initUserGameCheck(const uint32_t gameType, 
                             uint16_t gameServer,
                             std::string gameName)
{
    UserGameInfo data;
    data.gameType = (GameType)gameType;
    data.gameServer = gameServer;
    data.gameName = gameName; 

    m_userGame[data.gameType] = data;
}

uint64_t User::getId()
{
    return m_id;
}

CoffeenetId User::getCoffeenetId()
{
    return m_coffeenetId;
}

uint16_t User::getGameServer(const GameType gameType)
{
    auto it = m_userGame.find(gameType);
    if(it == m_userGame.end())
        return false;
    return true;
}

std::string User::getName()
{
    return m_userName;
}

std::string User::getPic()
{
    return m_pic;
}

void User::sendToMe(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize)
{
    MsgCode1 code1 = (MsgCode1)msg::MsgType::user;
    Game::me().sendtoClient(coffeenetId, code1, code2, data, dataSize);
}

void User::select()
{
    selectRecord++;
}

void User::selectRet()
{
    if(selectRecord == 0)
        return;
    selectRecord--;
    if(selectRecord == 0)
        sendUserData();
}

void User::sendUserData()
{
    if(selectRecord != 0)
        return;
    //基础信息
    msg::UserMainData send;
    send.userId = m_id;
    m_pic.copy(send.pic, MAX_PIC_SIZE);
    send.score = m_score;
    //实名认证信息
    m_identify.copy(send.identify, MAX_IDENTIFY_SIZE);
    //战队信息
    const BaseUserData* userData = TeamManager::me().getUserData(m_id);
    if(!userData)
    {
        LOG_ERROR("内存错误, 角色缓存中找不到角色, userId={}", m_id);
        return;
    }
    int i = 0;
    for(auto& item : userData->teams)
    {
        send.team[i].gametype = (uint16_t)item.first;
        send.team[i].teamId = item.second;
        i++;
    }
    //游戏信息
    i = 0;
    for(auto it = m_userGame.begin(); it != m_userGame.end(); ++it)
    {
        send.gameInfo[i].gameType = (uint16_t)it->second.gameType;
        send.gameInfo[i].gameServer = it->second.gameServer;
        it->second.gameName.copy(send.gameInfo[i].name, MAX_NAME_SIZE);
        i++;
    }

    LOG_DEBUG("角色同步主数据, identify={}", send.identify);
    sendToMe(m_coffeenetId, (uint32_t)msg::UserMsgType::userData, &send, sizeof(send));
}

void User::setIdentify(const std::string& identify,
                       const std::string& name,
                       const std::string& address,
                       const std::string& tel)
{
    m_identify = identify;
    m_name = name;
    m_acceptAddress = address;
    m_acceptTel = tel; 
    sendUserData();
}

bool User::haveIdentify()
{
    //return m_name.empty()&&m_identify.empty()&&m_acceptAddress.empty()&&m_acceptTel.empty();
    return !m_identify.empty();
}

void User::setGameInfo(GameType gameType, uint16_t gameServer, std::string name)
{
    UserGameInfo data;
    data.gameType = gameType;
    data.gameServer = gameServer;
    data.gameName = name;
    m_userGame[gameType] = data;
    sendUserData();
}

bool User::haveGameInfo(GameType gameType)
{
    auto it = m_userGame.find(gameType);
    if(it == m_userGame.end())
       return false;
    return true;
}

bool User::isGameServer(GameType gameType, uint16_t gameServer)
{
    auto it = m_userGame.find(gameType);
    if(it == m_userGame.end())
       return false;
    if(it->second.gameServer != gameServer)
        return false;
    return true;
}

void User::heartBeat(const  water::component::TimePoint& now)
{
    m_lastHeart = water::component::toUnixTime(now); 
}

void User::timerHeartBeat(const water::component::TimePoint& now)
{
    if(m_lastHeart + 5 * 60 < water::component::toUnixTime(now))
    {
        LOG_DEBUG("心跳超时, 将被动下线, userId={}, name={}", m_id, m_userName);
        UserManager::me().delUser(m_id);
    }
}

}
