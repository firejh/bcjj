#ifndef AREA_FIGHT_H
#define AREA_FIGHT_H

#include "fight.h"

namespace game{

class AreaFight : public Fight
{
public:
    AreaFight(FightData, uint32_t nextNum, uint64_t parentId, std::string area);
    ~AreaFight();
    TYPEDEF_PTR(AreaFight)
    CREATE_FUN_NEW(AreaFight)

    bool init();

    const uint64_t getParentId() const override; 
    uint32_t getNextLvNum();

private:

    uint32_t m_nextNum;         //下级赛事需要晋级的名额
    uint64_t m_parentId;        //上级赛事id
    std::string m_area;          //级别对应的区域代码
};

}
#endif
