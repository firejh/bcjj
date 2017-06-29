#include "fight_rule.h"
#include "fight_db.h"
#include "fight_manager.h"

#include "component/logger.h"
#include "msg/fight_msg.h"

namespace game{

FightRule::FightRule(uint64_t fightId, FightRule2 rule2, uint64_t parentId): m_fightId(fightId), m_rule2(rule2), m_parentId(parentId)
{}

void FightRule::contrl()
{

}

const std::map<uint64_t, FightObjData>& FightRule::getMemData()
{
    return m_objs;
}

bool FightRule::haveSignUp(uint64_t objId)
{
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
        return false;
    return true;
}

bool FightRule::inFight(const uint64_t objId) const
{
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
        return false;
    return true;
}

void FightRule::signUp(uint64_t objId)
{
    FightObjData data;
    data.fightId = m_fightId;
    m_objs[objId] = data;
    FightDb::me().updateFightObj(data.id, data.fightId, data.status, data.value, data.valueScore, data.position, data.competitor);
}

void FightRule::addObj(const FightObjData& data)
{
    LOG_DEBUG("初始化参赛成员:id={}", data.id);
    m_objs[data.id] = data;
}

void FightRule::addChall(const ChallengeData& data)
{
    m_challDatas[data.objId1] = data;
}

void FightRule::createHouse(uint64_t objId, std::vector<FightTeamData>& mem)
{
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
    {
        LOG_ERROR("创建赛事个体的战队, 没有报名");
        return;
    }
    it->second.team.swap(mem);
    LOG_DEBUG("创建赛事个体的战队, 成功, 进入准备流程");
}

const FightObjData* FightRule::getCompData(uint64_t objId)
{
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
        return nullptr;
    auto itComp = m_objs.find(it->second.competitor);
    if(itComp == m_objs.end())
        return nullptr;
    return &(itComp->second);
}

void FightRule::ready(uint64_t objId, User::Ptr user)
{
    //user一定不为空
    uint64_t userId = user->getId();
    msg::RetReadyFightToS send;
    send.userId = userId;
    send.result = 1;
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
    {   
        LOG_DEBUG("准备, 赛事中不存在objId={}", objId);
        return;
    }   

    if(it->second.team.empty())
    {   
        LOG_DEBUG("准备, 房间为空", objId);
        send.result = 3;
        FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToS, &send, sizeof(send));
        return;
    }   
    auto itComp = m_objs.find(it->second.competitor);
    if(itComp == m_objs.end())
    {
        LOG_ERROR("准备, 对方没找到");
        return;
    }

    if(it->second.team.back().m_userId == userId)
    {   
        //队长
        if(!allReady(it->second.team))
        {   
            LOG_DEBUG("准备, 队员未全部准备");
            send.result = 2;
            FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToS, &send, sizeof(send));
            return;
        }   
        //所有人准备成功，要做某些处理
        //找到对手看是否已经开始
        {
            if(!itComp->second.team.empty() && itComp->second.team.back().m_status == 1)
            {
                msg::FightStart send;
                for(auto& item : itComp->second.team)
                {
                    send.userId = item.m_userId;
                    auto member = UserManager::me().getUser(item.m_userId);
                    if(member)
                        FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::start, &send, sizeof(send));
                }
                for(auto& item : it->second.team)
                {
                    send.userId = item.m_userId;
                    auto member = UserManager::me().getUser(item.m_userId);
                    if(member)
                        FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::start, &send, sizeof(send));
                }
            }
        }

    }   
    FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToS, &send, sizeof(send));

    msg::ReadyFightToC sendC;
    sendC.readyMemId = userId;
    for(auto& item : it->second.team)
    {   
        if(item.m_userId == userId)
            item.m_status = 1;
        //通知每个队员，有人准备成功
        sendC.userId = item.m_userId;
        User::Ptr mem = UserManager::me().getUser(item.m_userId);
        if(mem)
            FightManager::me().sendtoClient(mem->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToC, &sendC, sizeof(sendC));
    }
    for(auto& item : itComp->second.team)
    {
        //通知每个队员，有人准备成功
        sendC.userId = item.m_userId;
        User::Ptr mem = UserManager::me().getUser(item.m_userId);
        if(mem)
            FightManager::me().sendtoClient(mem->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToC, &sendC, sizeof(sendC));
        
    }
}

bool FightRule::allReady(std::vector<FightTeamData>& team)
{
    for(uint32_t i = 0; i + 1 < team.size(); i++)
    {
        if(team[i].m_status == 0)
            return false;
    }

    return true;
}

uint16_t FightRule::getNum()
{
    return 0;
}

}
