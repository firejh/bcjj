#ifndef USER_USER_H
#define USER_USER_H

#include "stdint.h"

#include "../coffeenet/def.h"
#include "component/class_helper.h"
#include "../fight/fight_def.h"
#include "process/rawmsg_manger.h"

#include <string>
#include <map>

namespace game{

//用户游戏注册信息
struct UserGameInfo
{
    GameType gameType;
    uint16_t gameServer;
    std::string gameName;
};

class User
{
public:
    TYPEDEF_PTR(User);
    CREATE_FUN_NEW(User);
    User(uint64_t id,
         std::string name,
         std::string address,
         std::string tel,
         std::string qq,
         CoffeenetId coffeenetId);

    //来自网吧的被动删除
    void del();

    //正常下线，可能要发送消息
    void offline();
    void online();

    //初始化角色认证信息
    void initUserIdentify();
    //初始化色认证信息返回，这里后缀是check，改为dbback会好些
    void initUserIdentifyCheck(const std::string identify,
                               const std::string name,
                               const std::string address,
                               const std::string tel);
    //初始化用户游戏信息
    void initUserGame();
    //初始化用户游戏信息数据返回，里后缀是check，改为dbback会好些
    void initUserGameCheck(const uint32_t gameType, 
                           uint16_t gameServer,
                           std::string gameName);

    //获取基本信息
    //获取角色id
    uint64_t getId();
    //获取网吧id
    CoffeenetId getCoffeenetId();
    //获取角色游戏服
    uint16_t getGameServer(const GameType gameType);
    //名字
    std::string getName();
    //头像
    std::string getPic();

    //发送信息
    void sendToMe(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize);

    //发送角色主数据给客户端
    void sendUserData();

    //记录初始化进度，有些数据库查询，必须返回后才能进行下一步操作，select标记去查询了，selectRet代表查询完毕
    void select();
    void selectRet();


    //实名认证设置
    void setIdentify(const std::string& identify,
                     const std::string& name,
                     const std::string& address,
                     const std::string& tel);
    //是否已经实名认证
    bool haveIdentify();

    //游戏信息设置
    void setGameInfo(GameType gameType, uint16_t gameServer, std::string name);
    //是否已经注册游戏信息
    bool haveGameInfo(GameType gameType);
    //是否有此游戏服
    bool isGameServer(GameType gameType, uint16_t gameServer);

    void heartBeat(const  water::component::TimePoint& now);
    //心跳检测，5分钟一次即可
    void timerHeartBeat(const water::component::TimePoint& now);
    
private:

    //用户注册基本信息
    uint64_t m_id;
    std::string m_userName;
    std::string m_userAddress;
    std::string m_tel;
    std::string m_qq;
    std::string m_pic;

    //实名认证
    std::string m_name;
    std::string m_identify;
    std::string m_acceptAddress;
    std::string m_acceptTel;

    //游戏信息
    std::map<GameType, UserGameInfo> m_userGame;

    std::string m_apliPay;
    std::string m_weixin;
    uint32_t m_score;

    //网吧信息
    CoffeenetId m_coffeenetId;

    //标记基础数据是否加载完毕
    int selectRecord = 0;

    //心跳包时间
    uint64_t m_lastHeart = 0;
};


}

#endif
