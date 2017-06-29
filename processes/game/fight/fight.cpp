#include "fight.h"
#include "fight_db.h"
#include "fight_manager.h"

#include "component/logger.h"
#include "msg/fight_msg.h"

namespace game{

Fight::Fight(FightData fightData): m_fightData(fightData)
{
}

Fight::~Fight()
{}

bool Fight::init()
{
    return true;
}

void Fight::contrl()
{
    LOG_DEBUG("赛事控制检测, fight_id={}, status={}", m_fightData.m_id, m_fightData.m_status);
    //只有战斗状态才进入规则的一个循环，这个检测时间间隔尽量长一点
    if(beFight())
    {
        LOG_DEBUG("处于战斗状态");
        m_rule->contrl();
        if(m_rule->over() && reachStopFightTime())
        {
            LOG_TRACE("检测到赛事结束");
            stopFight();
        }
    }
    //是否开始报名
    if(reachStartApplyTime() && m_fightData.m_status == FightStatus::set)
    {
        //开始报名了
        LOG_DEBUG("检测到赛事开始报名");
        startApply();
    }
    //是否开始比赛
    if(reachStartFightTime() &&  m_fightData.m_status == FightStatus::apply)
    {
        LOG_DEBUG("检测到比赛开始, fightId={}", m_fightData.m_id);
        startFight();
    }
    //是否结束比赛，一个是规则是否结束，一个是时间是否到达结束时间
    if(reachStopFightTime() && m_rule->over())
    {
        LOG_DEBUG("检测到比赛结束, fightId={}", m_fightData.m_id);
        stopFight();
    }
}

void Fight::startApply()
{
    m_fightData.m_status = FightStatus::apply;
    FightDb::me().updateFightStatus(m_fightData.m_id, (uint16_t)m_fightData.m_status);
}

void Fight::startFight()
{
    //时间到了，但是不一定就能比赛，可能会加一些限制，比如人数不足，比如下级赛事没有提交结果等，待补充
    if(!m_rule->start())
        return;
    m_fightData.m_status = FightStatus::fight;
    FightDb::me().updateFightStatus(m_fightData.m_id, (uint16_t)m_fightData.m_status);
}

void Fight::stopFight()
{
    m_fightData.m_status = FightStatus::stop;
    //结束的删除即可，先变为stop状态，由管理类来删除
    FightDb::me().updateFightStatus(m_fightData.m_id, (uint16_t)m_fightData.m_status);
}

const uint64_t Fight::getId() const
{
    return m_fightData.m_id;
}

const std::string& Fight::getName() const
{
    return m_fightData.m_name;
}

const std::string& Fight::getPic() const
{
    return m_fightData.m_pic;
}

const GameType Fight::getGameType() const
{
    return m_fightData.m_gameType;
}

const uint64_t Fight::getStartApply() const
{
    return m_fightData.m_startApplyTime;
}

const uint64_t Fight::getStopApply() const
{
    return m_fightData.m_stopApplyTime;
}

const uint64_t Fight::getStartFight() const
{
    return m_fightData.m_startFightTime;
}

const uint64_t Fight::getStopFight() const
{
    return m_fightData.m_stopFightTime;
}

FightStatus Fight::getStatus()
{
    return m_fightData.m_status;
}

uint16_t Fight::getGameServer()
{
     return m_fightData.m_gameServer;
}

const std::map<uint64_t, FightObjData>& Fight::getMemData()
{
     return m_rule->getMemData();
}

const uint64_t Fight::getParentId() const
{
    return 0;
}

FightRule1 Fight::getRule1()
{
    return m_fightData.m_rule1;
}

FightRule2 Fight::getRule2()
{
    return m_fightData.m_rule2;
}

uint16_t Fight::getNum()
{
    return m_rule->getNum();
}

void Fight::addObj(const FightObjData& data)
{
    m_rule->addObj(data);
}

void Fight::ready(uint64_t objId, User::Ptr user)
{
    m_rule->ready(objId, user);
}

bool Fight::inFight(const uint64_t objId) const
{
    return m_rule->inFight(objId);
}

bool Fight::beApply()
{
    if(m_fightData.m_status == FightStatus::apply)
        return true;
    return false;
}

bool Fight::beFight()
{
    if(m_fightData.m_status == FightStatus::fight)
        return true;
    return false;
}

bool Fight::beStop()
{
   if(m_fightData.m_status == FightStatus::stop)
      return true;
   return false;
}

const FightObjData* Fight::getCompData(uint64_t objId)
{
    return m_rule->getCompData(objId);
}

bool Fight::bePause()
{
    if(m_fightData.m_stop == 0)
        return false;
    return true;
}

bool Fight::reachStartApplyTime()
{
    uint64_t now = time(NULL);
    if(now < getStartApply())
        return false;
    return true;
}

bool Fight::reachStartFightTime()
{
    uint64_t now = time(NULL);
    if(now < getStartFight())
        return false;
    return true;
}

bool Fight::reachStopFightTime()
{
    uint64_t now = time(NULL);
    if(now < getStopFight())
        return false;
    return true;
}

bool Fight::oneGame()
{
    if(getGameType() == GameType::lushi)
        return true;
    return false;
}

void Fight::signUp(uint64_t objId)
{
    m_rule->signUp(objId);
}

bool Fight::haveSignUp(uint64_t objId)
{
    return m_rule->haveSignUp(objId);
}

void Fight::createHouse(uint64_t objId, std::vector<FightTeamData>& mem)
{
    m_rule->createHouse(objId, mem);
}

void Fight::fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic)
{
    m_rule->fightResult(objId, user, result, pic);
}

void Fight::addNextLvMem(std::vector<uint64_t>& mem)
{
    for(auto& item : mem)
    {
        LOG_DEBUG("下级赛事提交的晋级成员:{}", item);
        m_rule->signUp(item); 
    }
}

void Fight::addMem(FightObjData& data)
{
    m_rule->addObj(data); 
}

void Fight::addChall(ChallengeData& challs)
{
    m_rule->addChall(challs);
}

}
