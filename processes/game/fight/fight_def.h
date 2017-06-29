#ifndef FIGHT_FIGHT_DEF_H
#define FIGHT_FIGHT_DEF_H

#include <string>

namespace game{

//游戏类型，这里必须与数据库的类型相互对应，不能随意改动
enum class GameType : uint16_t
{
    error = 0,
    lol = 1,                //LOL     
    dota2 = 2,              //dota2
    shouwangxianfeng = 3,   //守望先锋
    lushi = 4,              //炉石传说，单人游戏
};

//大的赛事类型分类，5比较模糊，似乎跟单店赛事属于一类，但是又有区别，目前先做好区分，防止互相影响
enum class FightType : uint16_t
{
    areaF = 1,          //区域赛，按区域可以继承，如果不可继承，那就是可直接报名的
    allianceF = 2,      //联盟赛事，没有上级赛事，网吧继承
    publicF = 3,        //公共赛事，没有上级赛事，用户直接玩
    coffnetF = 4,       //单店赛事，本网吧自己的赛事，没有继承
};

//赛制类型
enum class FightRule1 : uint16_t
{
    singleLoop = 1,     //单败 
    timeScore = 2,      //限时积分
    timeMoney = 3,
    task = 4,
    singleFail = 5,     //单败
    doubleFail4 = 6,    //四人双败
    doubleFail8 = 7,    //八人双败
    doubleFail16 = 8,   //八人双败
    tempScore = 9,
};

//赛事规则
enum class FightRule2 : uint16_t
{
    bio1 = 1,           //1局
    bio3 = 2,           //3局
    bio5 = 3,           //5局
    bio7 = 4,           //7局
};

//用户类型
enum class UserType : uint16_t
{
    admin = 1,          //超级管理员
    provinceD = 2,      //升级代理
    cityD = 3,          //市级代理
    coffeenet = 4,      //网吧用户
    user = 5,           //普通用户
};

//奖励类型
enum class AwardType : uint16_t
{
    money = 1,          //现金
    obj = 2,            //物品奖励
    score = 3,          //分数
    up = 4,             //晋级名额
};

enum class FightStatus : uint16_t
{
    set = 0,            //发布的默认状态
    apply = 1,          //可报名状态
    fight = 2,          //在战斗状态
    stop = 3,           //停止
};

//奖励数据
struct AwardData
{
    uint64_t fightId;   //所在赛事
    uint16_t mark;      //名次
    AwardType type;     //奖励类型
    uint32_t num;       //奖励数量
    std::string instruction;//奖励说明
    
};

}

#endif
