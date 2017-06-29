#include "fight_rule_time_score.h"
#include "fight_manager.h"
#include "fight_db.h"

#include "component/logger.h"
#include "msg/fight_msg.h"

namespace game{

FightRuleLoop::FightRuleSingleFail(uint64_t fightId, FightRule2 rule2, uint64_t parentId): FightRule(fightId, rule2, parentId)
{}

bool FightRuleLoop::start()
{
    //排位赛开打之后,直接开始，没有任何限制，等待参赛人员加入匹配池即可
    return true;
}

bool FightRuleLoop::over()
{
    //时间到了立刻结束，跟规则无关
    return true;
}


bool FightRuleLoop::createChallData(std::vector<uint64_t>& validObjs)
{
    
    return true;
}

void FightRuleLoop::fourPkChallData(std::vector<uint64_t> objs)
{

}

void FightRuleLoop::contrl()
{
    if(m_tempReady.empty())
        return;
    if(m_tempReady.size() == 1)
    {
        return;
    }

    //结算
    //是否决出胜负，决出胜负的结算，有异常的放里面即可，不影响流程，等待仲裁即可
    for(auto it = m_challDatas.begin(); it != m_challDatas.end();)
    {
        //只有bio1模式，不管bio
        if((it->second.result1 == 1 && it->second.result2 == 2) || (it->second.result1 == 2 && it->second.result2 == 1))
        {
            //结束
             
            it = m_challDatas.erase(it);
        }
    }
}

void FightRuleLoop::stopRule()
{
    LOG_TRACE("积分赛事轮次结束, 赛事结束, fightId={}", m_fightId);
    if(m_parentId != 0)
    {
        auto fight = FightManager::me().getFight(m_parentId);
        if(fight)
        {
            uint32_t num = fight->getNextLvNum();
            std::vector<uint64_t> temp = getTop(num);
            fight->addNextLvMem(temp);
            LOG_TRACE("积分赛事轮次结束, fightId={}", m_fightId);
        }
    }
}

std::vector<uint64_t> FightRuleLoop::getTop(uint32_t num)
{
    LOG_DEBUG("积分赛事, 获取名次:num={}", num);
    std::multimap<uint32_t, uint64_t> temp;
    uint32_t i = 0;
    for(auto it = m_objs.begin(); it != m_objs.end(); ++it)
    {
        temp.insert({it->second.value, it->second.id});
        LOG_DEBUG("objid={}", it->second.id);
        i++;
        if(i >= num)
            break;
    }
    std::vector<uint64_t> ret;
    for(auto it = temp.begin(); it != temp.end(); ++it)
        ret.push_back(it->second);
    return ret;
}

void FightRuleLoop::fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic)
{
    if(result != 0 || result != 1)
    {
        LOG_DEBUG("提交战斗结果, result={}, 流程结束", result);
        return;
    }
    //user一定不为空
    uint64_t userId = user->getId();
    msg::RetResultFightToS send;
    send.userId = userId;
    send.result = 1;
    auto it = m_objs.find(objId);
    if(it == m_objs.end())
    {   
        LOG_DEBUG("提交战斗结果, 赛事中不存在objId={}", objId);
        return;
    }   

    if(it->second.team.empty())
    {   
        LOG_DEBUG("提交战斗结果, 房间为空", objId);
        send.result = 4;
        FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send));
        return;
    }   

    if(!it->second.team.empty() && it->second.team.back().m_userId != userId)
    {   
        LOG_DEBUG("提交战斗结果, 不是队长");
        send.result = 2;
        FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send));
        return;
    }   
    FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send));

    msg::ResultFightToC sendC;
    sendC.result = result;
    for(auto& item : it->second.team)
    {   
        if(item.m_userId == userId)
            item.m_status = 1;
        send.userId = item.m_userId;
        User::Ptr mem = UserManager::me().getUser(item.m_userId);
        if(mem)
            FightManager::me().sendtoClient(mem->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToC, &sendC, sizeof(send));
    }   

    //更改战斗数据
    {
        LOG_DEBUG("提交战斗结果, 积分赛制, objId={}, result={}", it->second.id, result);
        if(it->second.status != 0)
        {   
            uint64_t objId1 = it->second.id;
            auto itCom = m_objs.find(it->second.competitor);
            if(itCom != m_objs.end())
            {
                if(userId > itCom->second.id)
                    objId1 = itCom->second.id;
            }

            //找到ChallengeData
            auto itChall = m_challDatas.find(objId1);
            if(itChall != m_challDatas.end())
            {
                if(objId1 == userId)
                {
                    itChall->second.result1 = result;
                    itChall->second.pic1 = pic;
                }
                else
                {
                    itChall->second.result2 = result;
                    itChall->second.pic2 = pic;
                }
                //存入数据库
                auto& challData = itChall->second;
                FightDb::me().updateChallData(m_fightId, challData.objId1, 0, challData.objId2, challData.result1, challData.result2, challData.pic1, challData.pic2);
            }
            else
            {
                LOG_ERROR("交战斗结果, 未找到对应战斗");
                return;
            }

        }
        else
        {
            LOG_DEBUG("提交战斗结果, 已经被淘汰, objId={}", it->second.id);
        }
    }
}

uint32_t FightRuleLoop::getPositionVal(uint32_t num)
{
    if(num == 0)
        return num;

    uint32_t ret = 2;
    while(ret < num)
        ret = ret * 2;
    return ret;
}

bool FightRuleLoop::bioOver(uint32_t value1, uint32_t value2)
{
    if(m_rule2 == FightRule2::bio1)
        return (value1 + value2) >= 1;

    if(m_rule2 == FightRule2::bio3)
        return (value1 + value2) >= 3;

    if(m_rule2 == FightRule2::bio5)
        return (value1 + value2) >= 5;

    return false;
}

}
