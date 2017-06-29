#ifndef USER_MSG_H
#define USER_MSG_H

#include "msg.h"

#include "common/def.h"

#include <cstring>

namespace msg{

enum class UserMsgType: uint16_t
{
    error = 0,
    signIn = 1,
    loginTel = 2,
    logoff = 3,
    userData = 4,                           //角色上线后初始话主数据
    identify = 5,                           //实名认证
    gameInfo = 6,                           //角色游戏信息提交
    heartBeat = 7,                          //心跳
};

#pragma pack(1)

//通用结构
struct UserData
{
    UserData(){memset(this, 0, sizeof(*this));}
    char userName[MAX_NAME_SIZE];           //账户名称，MAX_NAME_SIZE是32
    char userAddress[MAX_ADDRESS_SIZE];     //账户地区, MAX_ADDRESS_SIZE是60
    char passwd[MAX_PASSWD_SIZE];           //登录密码，MAX_PASSWD_SIZE是32
    char tel[MAX_TEL_SIZE];                 //手机号，MAX_TEL_SIZE是12
    char qq[MAX_QQ_SIZE];                   //qq账号，MAX_QQ_SIZE是20
    char pic[MAX_PIC_SIZE];
};

//2.登录
//c->s
struct TelUserLogin
{
    char tel[MAX_TEL_SIZE];                 //手机号，MAX_TEL_SIZE是12
    char passwd[MAX_PASSWD_SIZE];           //登录密码，MAX_PASSWD_SIZE是32
};
//s->c
struct RetTelUserLogin
{
    RetTelUserLogin(){memset(this, 0, sizeof(*this));}
    uint64_t id;                            //账户Id
    uint8_t result;                         //登录结果，1:成功，2:用户名或密码错误
    char userName[MAX_NAME_SIZE];           //账户名称，MAX_NAME_SIZE是32
    char userAddress[MAX_ADDRESS_SIZE];     //账户地区
    char tel[MAX_TEL_SIZE];                 //手机号，MAX_TEL_SIZE是12
    char qq[MAX_QQ_SIZE];                   //qq账号，MAX_QQ_SIZE是20
};

//1.注册
//c->s
struct UserSignIn
{
    uint8_t signInType;                     //注册类型，1:手机（暂时先做手机）
    UserData userData;                      //账户基础数据
};
//s->c
struct RetUserSignIn
{
    RetUserSignIn(){memset(this, 0, sizeof(*this));}
    uint8_t result;                         //注册结果，0:失败，1:成功，2:已经注册
    char tel[MAX_TEL_SIZE];                 //手机号，MAX_TEL_SIZE是12，客户端登录之前的唯一标识
};

//3.下线，待定
//c->s
struct UserLogoff
{
};

//4.角色主数据
//s->c
struct UserMainData
{
    UserMainData(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //userId

    //角色基本信息
    char pic[MAX_PIC_SIZE];                 //角色头像
    uint32_t score;                         //积分

    //实名认证信息
    char identify[MAX_IDENTIFY_SIZE];       //身份证，是否认证的标识

    //战队
    struct TeamData
    {
        uint16_t gametype;                  //战队的游戏类型
        uint64_t teamId;                    //战队id
    }team[5];

    struct GameInfo
    {
        uint16_t gameType;
        uint16_t gameServer;                //战队的也是个人的
        char name[MAX_NAME_SIZE];           //角色的游戏名
    }gameInfo[5];

    //其他信息...
};

//5.实名认证
//c->s
struct IdentifyData
{
    uint64_t userId;                        //角色id
    
    char identify[MAX_IDENTIFY_SIZE];       //身份证
    char name[MAX_NAME_SIZE];               //名字
    char address[MAX_ADDRESS_SIZE];         //收货地址
    char tel[MAX_TEL_SIZE];                 //电话
};
//s->c
struct RetIdentifyData
{
    uint64_t userId;                        //角色id

    uint8_t result;                         //1：成功
};

//6.游戏角色信息
//c->s
struct UserGameInfo
{
    uint64_t userId;                        //角色id

    uint16_t gameType;                      //游戏类型（不是名称）
    uint16_t gameServer;                    //游戏区服
    char gameName[MAX_NAME_SIZE];           //游戏信息
};
//s->c
struct RetUserGameInfo
{
    uint64_t userId;                        //角色id

    uint8_t result;                         //1.成功,2:在对应战队中，不能修改游戏信息
};

//7.角色心跳
//c<->s
struct HeartBeat
{
    uint64_t userId;                        //角色id
};

#pragma pack()

}
#endif
