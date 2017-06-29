#ifndef TEAM_MSG_H
#define TEAM_MSG_H

#include "msg.h"

#include "common/def.h"

#include <cstring>

namespace msg{

enum class TeamMsgType: uint16_t
{
    error = 0,
    createTeam = 1,
    TeamInfo = 2,//请求战队信息
    JoinTeamToS = 3,//申请加入战队，发送到服务器，回复也是此消息号
    JointTeamToC = 4,//申请加入战队，服务器发送给队长(不知道是否需要实时？)
    DealJoinTeamToS = 5,//处理结果发送给服务器，回复也是此消息号
    DealJoinTeamToC = 6,//处理结果发送给客户端
    TeamerList = 7,//战队成员列表
    CoffTeam = 8,//网吧战队列表
    allGameServer = 9,//请求对应类型的游戏服列表
    findTeam = 10,//搜索战队
};

#pragma pack(1)
//通用数据结构
struct TeamerData
{
    TeamerData(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id
    char userName[MAX_NAME_SIZE];       //用户名
    uint16_t userStatus;                //1：正式成员，2：带批准成员
    uint8_t online;                     //1：在线，2：不在线
    //...
    char gameName[MAX_NAME_SIZE];       //游戏角色昵称
    char tel[MAX_TEL_SIZE];             //电话
};


//1.创建战队
//c->s
struct CreateTeam
{
    CreateTeam(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id
    
    char pic[MAX_PIC_SIZE];             //头像id
    char teamName[MAX_NAME_SIZE];       //战队名称
    uint32_t gameType;                  //游戏类型，1：lol，2：dota，其他待添加
    uint16_t gameServer;                //游戏服
    uint64_t coffeenetId;               //网吧id
};
//s->c
struct RetCreateTeam
{
    RetCreateTeam(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint8_t result;                     //结果，1：成功，2：已经加入了对应游戏的战队（这里客户端自己要确保有对应战队后不能再请求）,3：未实名认证，需要实名才能创建，4：未注册个人游戏信息或者个人与战队的信息不一致
};

//2.请求我的战队
//c->s
struct TeamInfo
{
    TeamInfo(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id
    uint64_t teamId;                    //战队id
};
//s->c
struct RetTeamInfo
{
    RetTeamInfo(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    //战队基础信息
    uint64_t teamId;                    //战队Id，如果为0说明请求的战队不存在
    char pic[MAX_PIC_SIZE];
    char teamName[MAX_NAME_SIZE];       //战队名称 
    uint32_t gameType;                  //游戏类型，1：lol，2：dota，其他待添加
    uint16_t gameServer;                //游戏服
    //char province[MAX_NAME_SIZE];       //省
    //char city[MAX_NAME_SIZE];           //城市
    //char area[MAX_NAME_SIZE];           //区域
    char coffeenetName[MAX_NAME_SIZE];  //网吧名

    //战队特殊职位
    char leaderName[MAX_NAME_SIZE];
    uint16_t victoryPer;
    char tel[MAX_TEL_SIZE];

    uint32_t gold;//金
    uint32_t yin;//银
    uint32_t tong;//铜
    uint64_t leader;//队长
    uint64_t viceLeader;//副队长
};

//3.申请加入战队
//c->s
struct JoinTeamToS
{
    JoinTeamToS(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint64_t teamId;                    //战队id
    char pic[MAX_PIC_SIZE];
    char teamName[MAX_NAME_SIZE];       //战队名称 
    uint32_t gameType;                  //游戏类型，1：lol，2：dota，其他待添加
    uint16_t gameServer;                //游戏服
    //char province[MAX_NAME_SIZE];       //省
    //char city[MAX_NAME_SIZE];           //城市
    //char area[MAX_NAME_SIZE];           //区域
    char coffeenetName[MAX_NAME_SIZE];  //网吧名

    //战队特殊职位
    char leaderName[MAX_NAME_SIZE];
    uint16_t victoryPer;
    char tel[MAX_TEL_SIZE];

    uint32_t gold;//金
    uint32_t yin;//银
    uint32_t tong;//铜
    uint64_t leader;//队长
    uint64_t viceLeader;//副队长
};
//s->c
struct RetJoinTeamToS
{
    RetJoinTeamToS(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint8_t result;                     //申请接受，1：已受理，2：战队人数已满，3:已经有对应游戏的战队（这里客户端自己要确保有对应战队后不能再请求），4：未认证
};

//4.申请加入战队，服务器发给队长，只要知道有申请即可，处理的话需要打开战队界面，会有战队信息请求
//s->c
struct JointTeamToC
{
    JointTeamToC(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

};

//5.队长对申请的处理
//c->s
struct DealJoinTeamToS
{
    DealJoinTeamToS(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint64_t teamId;                    //处理的战队id
    uint64_t dealUserId;                //处理的用户id
    uint8_t type;                       //处理结果，1：同意，2：拒绝
};
//s->c
struct RetDealJoinTeamToS
{
    RetDealJoinTeamToS(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint64_t teamId;                    //处理的战队id
    uint64_t dealUserId;                //处理的用户id
    uint8_t result;                     //服务器结果，1:加入，2：拒绝，3：已经加入其它的战队了（客户端自己删除该记录即可，服务器已经删除，再次发起无响应）
};

//6.服务器返回队长的处理结果发给申请人
//s->c
struct DealJoinTeamToC
{
    DealJoinTeamToC(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint64_t TeamId;                    //加入的战队id
};

//7.战队成员列表
//c->s
struct TeamerList
{
    TeamerList(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id
    uint64_t teamId;                    //战队id
};
//s->c，每次发5个
struct RetTeamerList
{
    RetTeamerList(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id
    uint16_t totalSize;                 //总共会发多少个
    TeamerData data[5];                 //成员数据
};

//8.对应网吧的所有战队
//c->s
struct CoffTeam
{
    uint64_t userId;                    //用户id
    uint64_t coffeenetId;               //网吧id
};
//s->c
struct RetCoffTeam
{
    RetCoffTeam(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint16_t total;                     //共有多少个战队
    struct TeamData
    {
        uint64_t teamId;                //战队id
        char pic[MAX_PIC_SIZE];         //头像
        char name[MAX_NAME_SIZE];       //名字
        uint16_t gameType;              //游戏类型
        uint16_t gameServer;            //游戏服
    }teamData[5];
};

//9.请求对应类型的游戏服务器列表
struct AllGameServer
{
    uint64_t userId;                    //用户id

    uint16_t gameType;
};

struct RetAllGameServer
{
    RetAllGameServer(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                    //用户id

    uint16_t total;                     //总数
    struct Server
    {
        uint16_t id;                    //游戏服id
        char serverName[MAX_NAME_SIZE]; //游戏服名称
    }data[5];
};

//10.搜索战队
//c->s
struct FindTeam
{
    uint32_t userId;
    uint64_t teamId;
};

struct RetFindTeam
{
    RetFindTeam(){memset(this, 0, sizeof(*this));}
    uint32_t userId;
    uint64_t teamId;    //为0代表搜索不到
    char name[MAX_NAME_SIZE];
};

#pragma pack()

}
#endif
