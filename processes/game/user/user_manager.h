#ifndef USER_USER_MANAGER_H
#define USER_USER_MANAGER_H

#include "user.h"

#include "../coffeenet/def.h"
#include "process/rawmsg_manger.h"
#include "common/def.h"

#include "stdint.h"
#include <string>
#include <map>

namespace game{

enum class SignInType : uint8_t
{
    tel = 1,
};

class UserManager
{
    //未登录的用户信息
    struct UserInInfo
    {
        water::component::TimePoint addTime;//用于超时处理
        std::string key;//tel/
        CoffeenetId coffeenetId;//网ger.h吧id，用于处理消息
        std::string passwd;
    };
public:
    ~UserManager(){}
    static UserManager& me();

    //消息注册
    void regMsgHandler();

    //账户注册
    void signIn(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void signInTelCheck(const std::string tel, const uint8_t result);

    //账户登录
    void loginTel(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void loginTelCheck(const uint64_t userId, 
                       const std::string userName, 
                       const std::string userAddress, 
                       const std::string passwd,
                       const std::string qq,
                       const std::string tel);
    //用户更新实名认证
    void identifyData(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void identifyDataCheck(const uint64_t userId, const std::string& identify, const std::string& name, const std::string& address, const std::string& tel);

    //用户更新游戏信息
    void gameInfo(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);

    //来自网吧的被动删除角色，与自己下线处理不同
    void delUser(uint64_t userId);
    
    //获取user的对象指针
    User::Ptr getUser(uint64_t userId);

    //心跳
    void heartBeat(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId, const water::component::TimePoint& now);
    
private:
    UserManager(){}

    //正常下线处理，与delUser相比需要处理网吧中的角色
    void offIfExist(uint64_t userId);

    //发送消息到客户端
    void sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize);
private:
    //在注册的用户
    std::map<std::string, UserInInfo> m_signInTels;
    //在登录的用户
    std::map<std::string, UserInInfo> m_loginTels;
    //已经登录的用户，暂时还没有与网吧服务器对接一个消息，用户检测是否在线，登录后的就一直在线，后期一定要做下线处理！
    std::unordered_map<uint64_t, User::Ptr> m_users;

};

}

#endif
