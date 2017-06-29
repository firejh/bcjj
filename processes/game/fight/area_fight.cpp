#include "area_fight.h"
#include "component/logger.h"

namespace game{

AreaFight::AreaFight(FightData data, uint32_t nextNum, uint64_t parentId, std::string area): 
  Fight(data), m_nextNum(nextNum), m_parentId(parentId), m_area(area)
{
}

AreaFight::~AreaFight()
{}

bool AreaFight::init()
{
    LOG_DEBUG("区域赛初始化, 申城区域赛:rule1={}", m_fightData.m_rule1);
    if(m_fightData.m_rule1 == FightRule1::singleFail)
    {
        LOG_DEBUG("生成rule");
        m_rule = FightRuleSingleFail::create(m_fightData.m_id, m_fightData.m_rule2, m_parentId);
        return true;
    }
    else
    {
        return false;
    }
    
}

const uint64_t AreaFight::getParentId() const
{
    return m_parentId;
}

uint32_t AreaFight::getNextLvNum()
{
    return m_nextNum;
}

}
