#ifndef FIGHT_RULE_H
#define FIGHT_RULE_H

#include "fight_def.h"
#include "component/class_helper.h"
 #include "../user/user_manager.h"

#include <vector>
#include "string.h"
#include <set>

namespace game{

//战斗上阵成员数据
struct FightTeamData
{
    FightTeamData(uint64_t userId): m_userId(userId), m_status(0){}
    uint64_t m_userId;
    uint16_t m_status;//是否准备，0：未准备，1：准备
};

//参赛个体的数据
struct FightObjData
{
    uint64_t id; 
    uint64_t fightId;       //赛事id
    uint16_t status = 1;    //淘汰赛代表是否被淘汰，1：正常，2：淘汰;
    uint32_t value = 0;     //用于记录轮次，如淘汰赛达到第几轮，淘汰后就固定了
    uint32_t score = 0;     //积分
    uint32_t valueScore = 0;//每轮次的积分，每轮结束都会变化，暂时用于淘汰赛
    uint32_t position = 0;  //位置，决定了对手
    uint64_t competitor = 0;//对手
    std::vector<FightTeamData> team;//最后一个人是队长
    std::set<uint64_t> haveChalled;//已经挑战过的人，暂时单败无用，但是循环赛可能有用
};

//当前轮次的所有战斗记录，本轮次结束即可统一结算参赛个体的状态
struct ChallengeData
{
    ChallengeData()
    {
        clear();
        objId1=0;
        objId2=0;
    }
    void clear(){
        result1 = 0;
        result2=0;
        pic1.clear();
        pic2.clear();
        reference = 0;}
    uint64_t objId1;    //id小的战队id
    uint64_t objId2;    //id大的战队id
    uint16_t result1;   //obj1提交的结果1：胜利,2:失败
    uint16_t result2;   //
    std::string pic1;   //obj1提交的截图
    std::string pic2;
    uint16_t reference;//是否仲裁，仲裁暂时没做
};

class FightRule
{
public:
    TYPEDEF_PTR(FightRule)
    CREATE_FUN_NEW(FightRule) 

    FightRule(uint64_t fightId, FightRule2 rule2, uint64_t parentId);

    //比赛流程控制
    virtual bool start() = 0;
    virtual bool over() = 0;
    virtual void contrl();

    //获取成员数据
    const std::map<uint64_t, FightObjData>& getMemData();
    //添加成员
    void addObj(const FightObjData& data);
    //添加战斗
    void addChall(const ChallengeData& data);

    //是否报名
    bool haveSignUp(uint64_t objId);
    //是否在赛事中
    bool inFight(const uint64_t objId) const;

    //消息接口，来自赛事消息接口，参考fight_manager.h
    virtual void signUp(uint64_t objId);
    void createHouse(uint64_t objId, std::vector<FightTeamData>& mem);
    void ready(uint64_t objId, User::Ptr user);
    virtual void fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic) = 0;

    //是否全部准备
    bool allReady(std::vector<FightTeamData>& team);
    //获取对手数据
    const FightObjData* getCompData(uint64_t objId);
    //获取参赛成员数量
    virtual uint16_t getNum();

protected:

    //所属赛事的数据
    uint64_t m_fightId;
    //规则对象指针
    FightRule2 m_rule2;
    //上级赛事的id
    uint64_t m_parentId;

    //成员管理
    std::map<uint64_t, FightObjData> m_objs;

    //本轮次的所有在进行的战斗
    std::map<uint64_t, ChallengeData> m_challDatas;
};

}
#endif
