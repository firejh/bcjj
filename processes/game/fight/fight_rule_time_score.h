/*
 * Author: JiangHeng
 *
 * Created: 2016-12-19 14:50 +0800
 *
 * Modified: 2016-12-19 14:50 +0800
 *
 * Description: 限时积分赛事，20s检测一次，是否匹配对战，匹配的是否正常开打(有可能有的客户端关了，退出的意外情况)
 */
#ifndef FIGHT_RULE_TIME_SCORE_H
#define FIGHT_RULE_TIME_SCORE_H

#include "fight_rule.h"

namespace game{

class FightRuleTimeScore : public FightRule
{
public:
    TYPEDEF_PTR(FightRuleTimeScore)
    CREATE_FUN_NEW(FightRuleTimeScore)
    FightRuleTimeScore(uint64_t fightId, FightRule2 rule2, uint64_t parentId);

    void stopRule();
    bool start() override;
    bool over() override;
    bool createChallData(std::vector<uint64_t>& validObjs);
    void contrl() override;
    void fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic) override;

    std::vector<uint64_t> getTop(uint32_t num);
private:
    uint32_t getPositionVal(uint32_t num);

    bool bioOver(uint32_t value1, uint32_t value2);

    void fourPkChallData(std::vector<uint64_t> objs);

    uint32_t getFront();

private:
    //临时进入的人
    std::list<uint64_t> m_tempReady;
};

}
#endif
