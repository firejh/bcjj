/*
 * Author: JiangHeng
 *
 * Created: 2016-11-30 17:37 +0800
 *
 * Modified: 2016-11-30 17:37 +0800
 *
 * Description: 赛事基类
 */
#ifndef FIGHT_H
#define FIGHT_H

#include "fight_def.h"

#include "component/class_helper.h"
#include "../user/user_manager.h"

#include "fight_rule_single_fail.h"
#include <vector>
#include <map>

namespace game{

//基础的赛事数据
struct FightData
{
    uint64_t m_id;              //用户id
    GameType m_gameType;        //游戏类型
    FightType m_fightType;      //赛事类型
    uint16_t m_gameServer;      //游戏对应的游戏服
    FightRule1 m_rule1;         //单败、双败等
    FightRule2 m_rule2;         //一局、三局绝胜负等
    uint64_t m_startApplyTime;  //赛事开始报名时间
    uint64_t m_stopApplyTime;   //赛事结束报名时间
    uint64_t m_startFightTime;  //赛事开始时间
    uint64_t m_stopFightTime;   //赛事结束时间，赛事到结束时间，赛事不一定打完，一直等打完再处理
    std::string m_name;         //赛事名称
    std::string m_pic;          //赛事图标，小图标，似乎还有个大图标
    UserType m_userTye;         //赛事发布者的类型
    uint64_t m_from;            //发布者id
    FightStatus m_status;       //默认是0：未开始，1：已经开始报名，2：正在进行，3:结束，结束后放进记录表
    uint64_t m_createTime;      //创建时间
    std::string m_instruction;  //似乎无用
    uint16_t m_stop = 0;        //0：正常，1：暂停，
    bool m_stopFlag = 0;        //0：默认，1：需要改变是否暂停的状态，web改变m_stop的同时必须改变m_stopFlag

};

class Fight
{
public:
    TYPEDEF_PTR(Fight)
    CREATE_FUN_NEW(Fight)
    Fight(FightData);
    virtual ~Fight();
    //虚函数
    virtual const uint64_t getParentId() const;//获取上级赛事的id
    virtual uint32_t getNextLvNum() = 0;//获取下级赛事的科晋级名额
    virtual bool init();//初始化，需要初始化rule等所属类

    //基础数据获取
    const uint64_t getId() const;//id
    const std::string& getName() const;//名字
    const std::string& getPic() const;//图片
    const GameType getGameType() const;//游戏类型
    const uint64_t getStartApply() const;//开始报名时间
    const uint64_t getStopApply() const;//结束报名时间
    const uint64_t getStartFight() const;//开始比赛时间
    const uint64_t getStopFight() const;//结束比赛时间
    FightStatus getStatus(); //获取赛事状态，是否开始以此状态为准
    uint16_t getGameServer();//获取游戏服
    FightRule1 getRule1();//获取规则1
    FightRule2 getRule2();//获取规则2
    const FightObjData* getCompData(uint64_t objId);//获取对手的数据

    //赛事属性验证
    bool beApply();
    bool beFight();
    bool beStop();
    bool bePause();
    bool reachStartApplyTime();
    bool reachStartFightTime();
    bool reachStopFightTime();
    bool oneGame();


    //赛流程控制
    void contrl();
    //比赛正式开赛
    void startFight();
    //比赛结束，清算
    virtual void stopFight();
    //开始报名
    void startApply();

    //下级赛事提供上来的成员，相当于报名
    void addNextLvMem(std::vector<uint64_t>& mem);
    //普通的添加成员接口
    void addMem(FightObjData& data);
    //增加一场战斗，主要是服务器启动时，从数据库加载的战斗
    void addChall(ChallengeData& challs);

    /**************依赖规则（赛制规则)********************************/
    //获取所有参赛者的数据
    const std::map<uint64_t, FightObjData>& getMemData();
    //获取参数者数量
    uint16_t getNum();
    //添加成员
    void addObj(const FightObjData& data);
    //对应战队是否报名
    bool haveSignUp(uint64_t objId);
    //对应战队是否在赛事中
    bool inFight(const uint64_t objId) const;

    //规则处理的消息请求
    void signUp(uint64_t objId);//报名
    void createHouse(uint64_t objId, std::vector<FightTeamData>& mem);//队长提交上阵成员后服务器创建一个房间
    void ready(uint64_t objId, User::Ptr user);//准备
    void fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic);//提交结果
private:

protected:
    //赛事规则
    FightRule::Ptr m_rule;
    //赛事基础数据
    FightData m_fightData;
};

}

#endif
