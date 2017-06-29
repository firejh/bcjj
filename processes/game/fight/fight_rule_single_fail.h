#ifndef FIGHT_RULE_SINGLE_FAIL_H
#define FIGHT_RULE_SINGLE_FAIL_H

#include "fight_rule.h"

namespace game{

class FightRuleSingleFail : public FightRule
{
public:
    TYPEDEF_PTR(FightRuleSingleFail)
    CREATE_FUN_NEW(FightRuleSingleFail)
    FightRuleSingleFail(uint64_t fightId, FightRule2 rule2, uint64_t parentId);

    //规则流程已经结束
    void stopRule();

    //重载基类，参考fight.h
    bool start() override;
    bool over() override;
    void contrl() override;
    void signUp(uint64_t objId) override;
    void fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic) override;

    //获取钱几名，num代表获取的个数，目前只能获取到前两名，3、4名的比赛没有打
    std::vector<uint64_t> getTop(uint32_t num);

    //一轮结束后重新安排对手并生成对应的战斗
    bool createChallData(std::vector<FightObjData*>& validObjs);

    //获取参赛成员数量
    uint16_t getNum() override;
private:
    //根据赛事成员数量获得对阵坐位个数
    uint32_t getPositionVal(uint32_t num);
    //比赛开始执行的初始化
    bool initPosition();
    //本轮是否结束，这里还需要完善，2:0可能就算结束
    bool bioOver(uint32_t value1, uint32_t value2);

};

}
#endif
