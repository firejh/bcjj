#ifndef FIGHT_MSG_H
#define FIGHT_MSG_H

#include "msg.h"

namespace msg{

enum class FightMsgType : uint16_t
{
    error = 0,
    recommondFight = 1,             //首页推荐
    coffRecommondFight = 2,         //点击首页推荐后跳转到对应的网吧赛事
    fightMemData = 3,               //赛事成员列表
    myCoffFight = 4,                //自己所在网吧的推荐赛事
    /***单败淘汰赛消息***/
    signUpFight = 5,                //报名
    myFight = 6,                    //自己的比赛列表
    //队长请求战队列表选人，        //队长请求战队列表准备选人    
    challengeMem = 7,               //队长确定挑战人员    
    houseCreate = 8,                //通知队员进入准备页面
    readyFightToS = 9,              //客户端发送给服务器准备，队长的话就是开始比赛
    readyFightToC = 10,             //服务器通知所有队员某个人准备了
    start = 11,                     //两边都开始后给两边队员发送通知，表示两边开始进入比赛
    resultFightToS = 12,            //队长提交比赛结果
    resultFightToC = 13,            //队长结果同步给队员

    resultToLeaderToS = 14,         //通知对方队长，本方的结果
    resultToLeaderToC = 15,         //通知本方队长，对方的回应
    resultToAllLeader = 16,         //通知双方队长一个最后结果
};


#pragma pack(1)

//1.首页推荐赛事
//c->s
struct RecommondFight
{
    uint64_t userId;                        //用户id

};
//s->c
struct RetRecommondFight
{
    RetRecommondFight(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id
    uint16_t total;                         //总共多少个
    struct FightData
    {
        uint64_t fightId;                   //赛事id
        char pic[MAX_PIC_SIZE];             //头像id
        char name[MAX_NAME_SIZE];
        uint16_t gameType;                  //游戏类型,1:lol,2:dota2,3:守望先锋，4：炉石床说
    }fightData[5];
};

//2.点击首页推荐后跳转到对应的可以报名的赛事
//c->s
struct CoffRecommondFight
{
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //赛事id

};
//s->c
struct RetCoffRecommondFight
{
    RetCoffRecommondFight(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id

    struct FightData
    {
        uint64_t fightId;
        char pic[MAX_PIC_SIZE];             //头像id
        char name[MAX_NAME_SIZE];
        uint16_t gameType;                  //游戏类型
        uint8_t inFight;                    //是否在本赛事中，0：不在，1：在
        uint16_t status;                    //赛事状态，0：只是发布，1：可以报名，2：已经开赛
        uint16_t rule1;                     //赛制                      
        uint16_t rule2;                     //几局几胜
        uint16_t num;                       //几人制比赛
        //...
    }fightData;
};

//3.请求赛事成员列表
//c->s
struct FightMemData
{
    uint64_t userId;                        //用户id
    uint64_t fightId;                       //赛事id
};
//s->c
struct RetFightMemData
{
    RetFightMemData(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id

    uint16_t total;
    struct MemData
    {
        uint64_t memId;                     //成员id(战队)
        char name[MAX_NAME_SIZE];           //名字
        uint32_t position;                  //位置
        uint32_t value;                     //达到的轮次
    }data[5];
};

//4.自己网吧的对应游戏的可打赛事列表
//c->s
struct MyCoffFight
{
    uint64_t userId;                        //用户id

};
//s->c
struct RetMyCoffFight
{
    RetMyCoffFight(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id
    uint16_t total;

    struct FightData
    {
        uint64_t fightId;
        char pic[MAX_PIC_SIZE];             //头像id
        char name[MAX_NAME_SIZE];
        uint16_t gameType;                  //游戏类型,1:lol,2:dota2,3:守望先锋，4：炉石床说
        uint8_t inFight;                    //是否在本赛事中，0：不在，1：在
        uint16_t status;                    //赛事状态，0：只是发布，1：可以报名，2：已经开赛
        uint16_t rule1;                     //赛制                      
        uint16_t rule2;                     //几局几胜
        uint16_t num;                       //几人制比赛
        //...
    }fightData[5];
};

//4.报名
//c->s
struct SignUp
{
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //赛事id
};
//s->c
struct RetSignUp
{
    uint64_t userId;                        //用户id

    uint8_t result;                         //结果，1：成功，2：无对应赛事，3：赛事不是报名状态，4：没有实名认证，5：没有对应的战队，6：不是队长或副队长，7：个人游戏服务器与赛事服务器不符（或没有注册个人游戏信息），8:已经报名，9：战队成员数量不足
};

//5.自己的赛事列表
//c->s
struct MyFight
{
    uint64_t userId;                        //用户id
};
//s->c
struct RetMyFight
{
    RetMyFight(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id
    
    uint16_t total;
    struct FightData
    {
        uint64_t fightId;
        char pic[MAX_PIC_SIZE];             //头像id
        char name[MAX_NAME_SIZE];
        uint16_t gameType;                  //游戏类型
        uint16_t rule1;                     //赛制规则，单败、双败、循环等(以此定显示格式)
        uint16_t rule2;                     //1：一局定胜负、2：3局2胜:3：5局3省()
    }fightData[5];
};

//7.发送上阵成员
//c->s
struct ChallengerMem
{
    ChallengerMem(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //参与的赛事id
     
    uint64_t mem[6];                        //上阵队员id
};
//s->c
struct RetChallengerMem
{
    uint64_t userId;                        //用户id

    uint8_t result;                         //1:成功，2：有不在线的队员（客户端必须选在线的，人员不够不能请求上阵的），3：不存在赛事，4:有的成员不在战队，5:不是队长，6:未报名,7:人员重复或者包含了队长
};

//8.通知队员进入准备界面，只要发一次即可，这里已经确保队员都在线
//s->c
struct HouseCreate
{
    HouseCreate(){memset(this, 0, sizeof(*this));}
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //赛事id
    char name[MAX_NAME_SIZE];               //赛事名
    uint16_t gameType;                      //游戏类型,1:lol,2:dota2,3:守望先锋，4：炉石传说
    struct Mem
    {
        uint64_t memId;
        char name[MAX_NAME_SIZE];
        char pic[MAX_PIC_SIZE];   
        uint16_t status;                    //0：未准备，1：已经准备，己方的一定为0
    }data[6];
    struct MemComp
    {
        uint64_t memId;
        char name[MAX_NAME_SIZE];
        char pic[MAX_PIC_SIZE];   
        uint16_t status;                    //0：未准备，1：已经准备
    }dataCom[6];
};

//9.点击准备（签到），是队长的话代表开始，其他队员没签到队长不能签到
//c->s
struct ReadyFightToS
{
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //赛事id
};
//s->c
struct RetReadyFightToS
{
    uint64_t userId;                        //用户id
    
    uint8_t result;                         //1成功，2:其他队员未准备（验证错误队长会收到）3：找不到房间（队长取消之类）4:没有赛事
};

//10.队员准备通知
//s->c
struct ReadyFightToC
{
    uint64_t userId;                        //用户id

    uint64_t readyMemId;                   //已经准备的队员或者队长，都准备的时候就是开始了  
};

//11.通知所有队员比赛开始
//s->c
struct FightStart
{
    uint64_t userId;                        //用户id

};

//12.战斗结果提交
//c->s
struct ResultFightToS
{
    uint64_t userId;                        //用户id

    uint64_t fightId;                       //赛事id，以后可能会去掉
    uint8_t result;                         //1:胜利，2：失败
    char resultPic[MAX_PIC_SIZE];           //图片，字符串格式，否则id获取太难，头像以后也改为字符串
};
//s->c
struct RetResultFightToS
{
    uint64_t userId;                        //用户id

    uint8_t result;                         //1:提交成功，2:找不到赛事(逻辑错误), 3:没有对应的战队, 4:房间为空，不是队长, 5：对方已经提交，等待确认即可
};

//13.s->c，通知队员(包括队长)比赛已经结束
struct ResultFightToC
{
    uint64_t userId;                        //用户id

    uint8_t result;                         //本队提交的结果，1：胜利，2：失败
};

//14.s->c，通知对方队长本方的提交结果
struct ResultToLeaderToS
{
    uint64_t userId;

    uint8_t result;                         //1:胜利，2：失败
};

//15.c->s，对方队长回复结果
struct ResultToLeaderToC
{
    uint64_t userId;

    uint64_t fightId;                       //赛事id
    uint8_t result;                         //1:同意，2：拒绝，3：仲裁
    char resultPic[MAX_PIC_SIZE];           //图片，截图
};

//16.s->c，通知两边一个最终结果
struct ResultToAllLeader
{
    uint64_t userId;

    uint16_t status;                        //1:同意，2：拒绝，3：仲裁
};

#pragma pack()

}
#endif
