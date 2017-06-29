#include "user_manager.h"
#include "user_db.h"

#include "../game.h"
#include "msg/user_msg.h"
#include "../coffeenet/coffeenet_manager.h"
#include "component/logger.h"
#include "common/def.h"
#include "process/rawmsg_manger.h"
#include "msg/user_msg.h"

#include "../fight/team_manager.h"

namespace game{

UserManager& UserManager::me()
{
    static UserManager me;
    return me;
}

void UserManager::regMsgHandler()
{
    using namespace std::placeholders;
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::user, (MsgCode2)msg::UserMsgType::signIn, std::bind(&UserManager::signIn, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::user, (MsgCode2)msg::UserMsgType::loginTel, std::bind(&UserManager::loginTel, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::user, (MsgCode2)msg::UserMsgType::identify, std::bind(&UserManager::identifyData, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::user, (MsgCode2)msg::UserMsgType::gameInfo, std::bind(&UserManager::gameInfo, this, _1, _2, _3));
}

void UserManager::signIn(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::UserSignIn))
    {
        LOG_DEBUG("用户注册, 错误的消息大小, size={}, needSize={}", msgSize, sizeof(msg::UserSignIn));
        return;
    }

    const msg::UserSignIn* msg = reinterpret_cast<const msg::UserSignIn*>(msgData);
    SignInType signIntype = (SignInType)msg->signInType;

    //db
    if(signIntype == SignInType::tel)
    {
        LOG_DEBUG("用户注册, TEL注册, TEL={}", msg->userData.tel);        
        UserInInfo info;
        info.key.append(msg->userData.tel, MAX_TEL_SIZE < strlen(msg->userData.tel)?MAX_TEL_SIZE:strlen(msg->userData.tel));
        info.coffeenetId = coffeenetId;
        info.addTime = water::component::Clock::now();
        auto it = m_signInTels.find(info.key);
        if(it != m_signInTels.end())
        {
            m_signInTels.erase(it);
        }
        else
        {
            //注册
            std::string name(msg->userData.userName, 
                             MAX_NAME_SIZE < strlen(msg->userData.userName)?MAX_NAME_SIZE:strlen(msg->userData.userName));
            std::string address(msg->userData.userAddress, 
                                MAX_ADDRESS_SIZE < strlen(msg->userData.userAddress)?MAX_ADDRESS_SIZE:strlen(msg->userData.userAddress));
            std::string passwd(msg->userData.passwd, 
                               MAX_PASSWD_SIZE < strlen(msg->userData.passwd)?MAX_PASSWD_SIZE:strlen(msg->userData.passwd));
            std::string tel(msg->userData.tel, 
                            MAX_TEL_SIZE < strlen(msg->userData.tel)?MAX_TEL_SIZE:strlen(msg->userData.tel));
            std::string qq(msg->userData.qq, 
                           MAX_QQ_SIZE < strlen(msg->userData.qq)? MAX_QQ_SIZE:strlen(msg->userData.qq));
            std::string pic(msg->userData.pic, 
                            MAX_PIC_SIZE < strlen(msg->userData.pic)?MAX_PIC_SIZE:strlen(msg->userData.pic));
            UserDb::me().signInTelUserInfo(name, address, passwd, tel, qq, pic); 

            m_signInTels[info.key] = info;
        }
    }
    else
    {
        LOG_DEBUG("用户注册, 错误的注册类型");
        return;
    }


}

void UserManager::signInTelCheck(const std::string tel, const uint8_t result)
{
    auto it = m_signInTels.find(tel);
    if(it == m_signInTels.end())
    {
        //LOG_ERROR("用户注册, 内存错误, signInTelCheck不存在对应的tel, tel={}", tel);
        return;
    }
    msg::RetUserSignIn send;
    send.result = 1;
    if(result == (uint8_t)0)
        send.result = 2;
    tel.copy(send.tel, MAX_TEL_SIZE);
    sendtoClient(it->second.coffeenetId, (MsgCode2)msg::UserMsgType::signIn, &send, sizeof(send));
    m_signInTels.erase(it);

    LOG_DEBUG("用户注册, tel={}, result={}", tel, result);
}

void UserManager::loginTel(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::TelUserLogin))
    {
        LOG_DEBUG("用户登录, 错误的消息大小, size={}", msgSize);
        return;
    }

    const msg::TelUserLogin* msg = reinterpret_cast<const msg::TelUserLogin*>(msgData);

    std::string passwd(msg->passwd, 
                       MAX_PASSWD_SIZE < strlen(msg->passwd)?MAX_PASSWD_SIZE:strlen(msg->passwd));
    std::string tel(msg->tel, 
                    MAX_TEL_SIZE < strlen(msg->tel)?MAX_TEL_SIZE:strlen(msg->tel));

    //检测是否正在登陆，如果正在登陆则失败处理（概率极低）
    auto itLogin = m_loginTels.find(tel);
    if(itLogin != m_loginTels.end())
    {
        LOG_DEBUG("用户登录, 重复登录, tel={}, 登录失效", tel);
        return;
    }

    //登陆验证开始
    UserDb::me().loginTelUser(tel);

    UserInInfo info;
    info.key = tel;
    info.passwd = passwd;
    info.addTime = water::component::Clock::now();
    info.coffeenetId = coffeenetId;
    m_loginTels[tel] = info;

}

void UserManager::loginTelCheck(const uint64_t userId, 
                                const std::string userName, 
                                const std::string userAddress, 
                                const std::string passwd,
                                const std::string qq,
                                const std::string tel)
{
    auto it = m_loginTels.find(tel);
    if(it == m_loginTels.end())
    {
        LOG_ERROR("用户登录, 内存错误, loginTelCheck不存在对应的tel, tel={}", tel);
        return;
    }

    msg::RetTelUserLogin send;
    if(std::string(passwd) != it->second.passwd || userId == 0)
    {
        //密码错误或用户不存在 
        LOG_DEBUG("用户登录, 用户不存在或密码错误, tel={}, id={}", tel, userId);
        send.result = 2;
        send.id = 0;
        tel.copy(send.tel, MAX_TEL_SIZE);
        sendtoClient(it->second.coffeenetId, (MsgCode2)msg::UserMsgType::loginTel, &send, sizeof(send));
        m_loginTels.erase(it);
        return;
    }

    User::Ptr user = User::create(userId, userName, userAddress, tel, qq, it->second.coffeenetId);

    //检测是否已经在线，如果在线，踢下线
    offIfExist(userId);
    m_users[userId] = user;

    //添加到相应网吧，类似于场景中有角色，角色中有场景这种; 注意包含关系，网吧要直接操作user，user一般不能操作网吧，所有网吧包含user，uer只需记录网吧id即可
    Coffeenet::Ptr coffeenet = CoffeenetManager::me().getCoffeenet(it->second.coffeenetId);
    if(!coffeenet)
    {
        LOG_ERROR("用户登录, 内存错误, loginTelCheck不存在对应的网吧, coffeenetId={}", it->second.coffeenetId);
        m_loginTels.erase(it);
        return; 
    }
    coffeenet->addUser(user);

    //上线后角色处理
    user->online();
    
    //从登录列表中删除登录信息
    m_loginTels.erase(it);

    //返回登录信息，不再user内处理，划分不明显，在哪处理都可以，失败和成功放一起处理比较方便
    send.id = userId;
    send.result = 1;
    userName.copy(send.userName, MAX_NAME_SIZE);
    userAddress.copy(send.userAddress, MAX_ADDRESS_SIZE);
    tel.copy(send.tel, MAX_TEL_SIZE);
    qq.copy(send.qq, MAX_TEL_SIZE);
    sendtoClient(it->second.coffeenetId, (MsgCode2)msg::UserMsgType::loginTel, &send, sizeof(send));
    LOG_DEBUG("用户登录, 流程结束结束, result={}, id={}, tel={}", send.result, send.id, tel);

    BaseUserData data;
    data.id = userId;
    //pic...
    data.name = userName;

    //如果不是第一次那就无效
    TeamManager::me().addUser(data);
}

void UserManager::identifyData(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::IdentifyData))
    {
        LOG_DEBUG("实名认证, 错误的消息size");
        return;
    }

    const msg::IdentifyData* msg = reinterpret_cast<const msg::IdentifyData*>(msgData);
    User::Ptr user = getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("实名认证, 角色未登录, userId={}", msg->userId);
        return;
    }

    std::string identify(msg->identify, strlen(msg->identify) < MAX_IDENTIFY_SIZE?strlen(msg->identify):MAX_IDENTIFY_SIZE);
    std::string name(msg->name, strlen(msg->name) < MAX_NAME_SIZE?strlen(msg->name):MAX_NAME_SIZE);
    std::string address(msg->address, strlen(msg->address) < MAX_ADDRESS_SIZE?strlen(msg->address):MAX_ADDRESS_SIZE);
    std::string tel(msg->tel, strlen(msg->tel)< MAX_TEL_SIZE?strlen(msg->tel):MAX_TEL_SIZE);
    UserDb::me().identifyData(msg->userId, identify, name, address, tel);
}

void UserManager::identifyDataCheck(uint64_t userId, const std::string& identify, const std::string& name, const std::string& address, const std::string& tel)
{
    LOG_DEBUG("实名认证检查, 内存设置, identify={}, name={}, tel={}", identify, name, tel);
    User::Ptr user = UserManager::me().getUser(userId);
    if(user)
    {
        user->setIdentify(identify, name, address, tel); 
        msg::RetIdentifyData send;
        send.userId = userId;
        send.result = 1;//成功
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::UserMsgType::identify, &send, sizeof(send));
    }

}

void UserManager::gameInfo(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId)
{
    LOG_DEBUG("用户更新游戏信息");
    if(msgSize < sizeof(msg::UserGameInfo))
    {
        LOG_DEBUG("用户更新游戏信息, 错误的size");
        return;
    }

    const msg::UserGameInfo* msg = reinterpret_cast<const msg::UserGameInfo*>(msgData);
    User::Ptr user = getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("用户更新游戏信息, 未登录");
        return;
    }
    msg::RetUserGameInfo send;
    send.userId = msg->userId;
    send.result = 1;

    //是否有战队
    GameType gameType = GameType(msg->gameType);
    if(TeamManager::me().haveTeam(msg->userId, gameType))
    {
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::UserMsgType::gameInfo, &send, sizeof(send));
        LOG_DEBUG("用户更新游戏信息, 已经存在对应战队，不允许更改");
        return;
    }

    std::string name(msg->gameName, MAX_NAME_SIZE < sizeof(msg->gameName) ? MAX_NAME_SIZE : sizeof(msg->gameName));
    UserDb::me().updateGameInfo(msg->userId, (uint16_t)gameType, msg->gameServer, name);
    TeamManager::me().insertMemGame(msg->userId, gameType, msg->gameServer, name);
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::UserMsgType::gameInfo, &send, sizeof(send));
}

void UserManager::delUser(uint64_t userId)
{
    auto it = m_users.find(userId);
    it->second->del();
    if(it == m_users.end())
    {
        LOG_ERROR("删除角色, 内存错误, 被删除的角色在角色管理中不存在, id={}", userId);
        return;
    }

    m_users.erase(it);
}

User::Ptr UserManager::getUser(uint64_t userId)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
       return nullptr;
    return it->second; 
}

void UserManager::offIfExist(uint64_t userId)
{
    auto itUsers = m_users.find(userId);
    if(itUsers != m_users.end())
    {
        LOG_DEBUG("用户登录, 已经在线, 将被踢, userId={}", userId);
        //本身正常下线处理
        itUsers->second->offline();
        //网吧角色同步处理
        Coffeenet::Ptr coffeenet = CoffeenetManager::me().getCoffeenet(itUsers->second->getCoffeenetId());
        coffeenet->delUser(itUsers->second->getId());
    }
}

void UserManager::heartBeat(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId, const water::component::TimePoint& now)
{
    if(msgSize < sizeof(msg::HeartBeat))
    {
        LOG_DEBUG("心跳size错误");
        return;
    }
    const msg::HeartBeat* msg = reinterpret_cast<const msg::HeartBeat*>(msgData);
    User::Ptr user = getUser(msg->userId); 
    if(!user)
    {
        LOG_DEBUG("心跳, 未登录");
    }
    else
        user->heartBeat(now);

    msg::HeartBeat send;
    send.userId = msg->userId;
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::UserMsgType::heartBeat, &send, sizeof(send));
}

void UserManager::sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize)
{
    MsgCode1 code1 = (MsgCode1)msg::MsgType::user;
    Game::me().sendtoClient(coffeenetId, code1, code2, data, dataSize);
}

}
