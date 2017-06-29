#ifndef COFFEENET_MSG_H
#define COFFEENET_MSG_H

#include "msg.h"

#include "common/def.h"

#include <cstring>

namespace msg{

enum class CoffeenetMsgType: uint16_t
{
    error = 0,
    login = 1,
    signIn = 2,
    logoff = 3,
    areaCoffeenet = 4,                  //区域网吧列表
    province = 5,
    city = 6,
    area = 7,
};

#pragma pack(1)

//通用结构
struct CoffData
{
    CoffData(){memset(this, 0, sizeof(*this));}
    char account[MAX_ACCOUNT_SIZE];         //登录账户
    char passwd[MAX_PASSWD_SIZE];           //登录密码，MAX_PASSWD_SIZE是32
    char name[MAX_NAME_SIZE];               //网吧名称，MAX_NAME_SIZE是32
    char simpleName[MAX_NAME_SIZE];         //网吧简称，MAX_ACCOUNT_SIZE是32，注意最后一位可能不是'\0'
    char ownerName[MAX_NAME_SIZE];          //业主姓名
    char ownerTel[MAX_TEL_SIZE];            //MAX_TEL_SIZE是12
    char managerTel[MAX_TEL_SIZE];          //店长电话
    uint8_t competitivePlatporm;            //是否有竞技平台，0:没有，1:有
    uint32_t pcNum;                         //网吧机器数量
    char competitiveClub[MAX_NAME_SIZE];    //电竞俱乐部名称
    char instruction[MAX_INSTRUCTION_SIZE]; //暂定600，注意最后一位可能不是'\0'
};

//1.登录
//c->s
struct CoffLogin
{
    char account[MAX_ACCOUNT_SIZE];         //登录账户，MAX_ACCOUNT_SIZE是32，注意最后一位可能不是'\0'
    char passwd[MAX_PASSWD_SIZE];           //登录密码，MAX_PASSWD_SIZE是32
};
//s->c
struct RetCoffLogin
{
    RetCoffLogin(){memset(this, 0, sizeof(*this));}
    char account[MAX_ACCOUNT_SIZE];         //账号
    uint64_t id;                            //网吧Id
    char name[MAX_NAME_SIZE];               //网吧名称，MAX_NAME_SIZE是32
    uint8_t vip;                            //vip，0:非vip，1:1级vip
    uint64_t vipInvalidTime;                //vip到期日期
};

//2.注册
//c->s
struct CoffSignIn
{
    CoffData coffData;                      //网吧基础数据
};
//s->c
struct RetCoffSignIn
{
    uint8_t result;                         //注册结果，0:失败，1:成功，2:已经注册
};

//3.下线，待定
//c->s
struct CoffLogoff
{
};

//4.区域网吧列表
//c->s
struct AreaCoffeenet
{
    AreaCoffeenet(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id

    char province[MAX_NAME_SIZE];           //省代号
    char city[MAX_NAME_SIZE];               //城市代号
    char area[MAX_NAME_SIZE];               //区代号
};

struct RetAreaCoffeenet
{
    RetAreaCoffeenet(){memset(this, 0, sizeof(*this));}
    uint64_t userId;

    uint16_t total;                         //总数
    struct CoffeenetData
    {
        uint64_t coffeenetId;               //网吧id
        char name[MAX_NAME_SIZE];           //姓名
    }coffData[5];
};

struct AreaData
{
    char code[MAX_AREA_CODE_SIZE];             //编码
    char name[MAX_NAME_SIZE];                //名字
};
//5.查询所有省
//c->s
struct Province
{
    uint64_t userId;

};
struct RetProvince
{
    RetProvince(){memset(this, 0, sizeof(*this));}
    uint64_t userId;

    uint16_t total;
    AreaData data[5];
};

//6.查询某个省的市
//c->s
struct City
{
    uint64_t userId;

    char code[MAX_AREA_CODE_SIZE];
};

struct RetCity
{
    RetCity(){memset(this, 0, sizeof(*this));}
    uint64_t userId;

    uint16_t total;
    AreaData data[5];
};

//7.查询某个市的区
struct Area
{
    uint64_t userId;

    char code[MAX_AREA_CODE_SIZE];
};

struct RetArea
{
    RetArea(){memset(this, 0, sizeof(*this));}
    uint64_t userId;
    
    uint16_t total;
    AreaData data[5];
};
#pragma pack()

}
#endif
