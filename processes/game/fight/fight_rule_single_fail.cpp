#include "fight_rule_single_fail.h"
#include "fight_manager.h"
#include "fight_db.h"

#include "component/logger.h"
#include "msg/fight_msg.h"

namespace game{

FightRuleSingleFail::FightRuleSingleFail(uint64_t fightId, FightRule2 rule2, uint64_t parentId): FightRule(fightId, rule2, parentId)
{}

bool FightRuleSingleFail::start()
{
    return initPosition();
}

bool FightRuleSingleFail::over()
{
    return m_challDatas.empty();
}

bool FightRuleSingleFail::initPosition()
{
    LOG_DEBUG("赛事开始，初始化排位, objNum={}", m_objs.size());
    std::vector<uint64_t> validObjs;
    for(auto& item : m_objs) 
    {
        validObjs.push_back(item.second.id);
    }

    //排位后生成所有的对战记录，相邻的两个一组
    if(validObjs.empty())
        return true;

    //得到应该排位的数量，不足的先在后面填0
    uint32_t size = validObjs.size();//实际参赛个数
    uint32_t positionVal = getPositionVal(size);//应该的排位个数
    while(validObjs.size() < positionVal)
        validObjs.push_back(0);

    //所有的成员位置重新排
    std::vector<uint32_t> allObjs;
    uint32_t i = 0;
    while(i < positionVal / (uint32_t)2)
    {
        LOG_DEBUG("positionVal ={},  {}->{}", positionVal, validObjs[i], validObjs[positionVal - 1 - i]);
        allObjs.push_back(validObjs[i]);
        allObjs.push_back(validObjs[positionVal - 1 - i]);
        i++;
    }

    for(uint32_t i = 0; i < positionVal - 1; i = i + 2)
    {
        auto it = m_objs.find(allObjs[i]);
        auto itCom = m_objs.find(allObjs[i + 1]);
        if(it == m_objs.end())
        {
            LOG_ERROR("单败赛事生成本轮次对战记录和对手, validObjs中的数据在m_objs中不存在, objId={}", allObjs[i]);
            return false;
        }

        //设置对手
        it->second.competitor = allObjs[i + 1];
        it->second.position = i;
        FightDb::me().updateFightObj(it->second.id, it->second.fightId, it->second.status, it->second.value, it->second.valueScore, it->second.position, it->second.competitor);
        if(itCom != m_objs.end())
        {
            itCom->second.competitor = allObjs[i];
            itCom->second.position = i + 1;
            FightDb::me().updateFightObj(itCom->second.id, itCom->second.fightId, itCom->second.status, itCom->second.value, itCom->second.valueScore, itCom->second.position, itCom->second.competitor);
        }

        //生成该有的对战记录
        auto& challData = m_challDatas[it->second.id];
        challData.objId1 = allObjs[i];
        challData.objId2 = allObjs[i + 1];

        //入库
        FightDb::me().updateChallData(m_fightId, challData.objId1, challData.objId2, challData.result1, challData.result2, challData.pic1, challData.pic2);
        LOG_DEBUG("对战: {}<->{}", allObjs[i], allObjs[i + 1]);
    }
    return true;
}

bool FightRuleSingleFail::createChallData(std::vector<FightObjData*>& validObjs)
{
    LOG_DEBUG("单败赛事生成本轮次对战记录和对手, size={}", validObjs.size());
    if(validObjs.empty())
        return true;

    std::map<uint32_t, FightObjData*> temp;
    for(auto& item : validObjs)
    {
        temp.insert({item->position, item});
        LOG_DEBUG("单败赛事生成本轮次对战记录和对手, 有效成员size={}", temp.size());
    }

    //此时有效用户一定是偶数个，直接完成对阵即可
    for(auto it = temp.begin(); it != temp.end();)
    {
        auto current = it;
        it++;
        //设置对手
        it->second->competitor = current->second->id;
        current->second->competitor = it->second->id;
        FightDb::me().updateFightObj(it->second->id, it->second->fightId, it->second->status, it->second->value, it->second->valueScore, it->second->position, it->second->competitor);
        FightDb::me().updateFightObj(current->second->id, current->second->fightId, current->second->status, current->second->value, current->second->valueScore, current->second->position, current->second->competitor);

        //生成战斗
        //生成该有的对战记录
        auto& challData = m_challDatas[current->second->id];
        challData.objId1 = current->second->id;
        challData.objId2 = it->second->id;

        //入库
        FightDb::me().updateChallData(m_fightId, challData.objId1, challData.objId2, challData.result1, challData.result2, challData.pic1, challData.pic2);
        LOG_DEBUG("对战: {}<->{}", current->second->id, it->second->id);

        it++;
    }

    return true;
}

void FightRuleSingleFail::contrl()
{
    //执行这里一定是赛事正在进行中才执行

    if(m_challDatas.empty())
        return;

    //遍历场次的战斗，看是否本场次结束，如果结束obj增加胜利场次、生成场次记录
    //如果场次结束，判断是否轮次结束，结束后删除场次，obj做晋级处理，胜利场次清空，对手清空，等待下一轮次
    for(auto it = m_challDatas.begin(); it != m_challDatas.end();)
    {
        bool eraseItem = false;

        auto itObj1 = m_objs.find(it->second.objId1);
        if(itObj1 == m_objs.end())
        {
            LOG_ERROR("m_challDatas中的objId1没有对应的个体, objId1={}", it->second.objId1);
            return;
        }
        //轮空
        if(it->second.objId2 == 0)
        {//轮空，直接可以晋级下一轮了
            itObj1->second.value++;
            itObj1->second.valueScore = 0;
            itObj1->second.competitor = 0;
            itObj1->second.team.clear();
            FightDb::me().updateFightObj(itObj1->second.id, itObj1->second.fightId, itObj1->second.status, itObj1->second.value, itObj1->second.valueScore, itObj1->second.position, itObj1->second.competitor);

            //删除场次
            FightDb::me().eraseChallData(m_fightId, it->second.objId1, 0);//轮次结束，删除本场次即可
            it = m_challDatas.erase(it);
            eraseItem = true;

        }
        else if((it->second.result1 == 1 && it->second.result2 == 2) || (it->second.result1 == 2 && it->second.result2 == 1))
        {//正常提交了比赛结果

            auto itObj2 = m_objs.find(it->second.objId2);
            if(itObj2 == m_objs.end())
            {
                LOG_ERROR("m_challDatas中的objId2没有对应的个体, objId2={}", it->second.objId2);
                continue;
            }
            //找胜负方
            auto winner = &(itObj1->second);
            auto failer = &(itObj2->second);
            if(it->second.result1 == 2)
            {
                winner = &(itObj2->second);
                failer = &(itObj1->second);
            }

            //...生成场次记录

            //场次复原
            it->second.clear();
            FightDb::me().updateChallData(m_fightId, it->second.objId1, it->second.objId2, it->second.result1, it->second.result2, it->second.pic1, it->second.pic2);

            //胜方场次++
            winner->valueScore++;
            //是否轮次结束
            if(bioOver(winner->valueScore, failer->valueScore))
            {
                winner->value++;//记录下在第几轮次被淘汰，根据此可以算出名次，以及客户端对阵显示
                winner->valueScore = 0;
                winner->competitor = 0;
                winner->team.clear();
                failer->status = 2;//被淘汰
                failer->valueScore = 0;
                failer->competitor = 0;
                failer->team.clear();
                //备份胜负方obj数据
                FightDb::me().updateFightObj(winner->id, winner->fightId, winner->status, winner->value, winner->valueScore, winner->position, winner->competitor);
                FightDb::me().updateFightObj(failer->id, failer->fightId, failer->status, failer->value, failer->valueScore, failer->position, failer->competitor);
                
                //删除场次
                FightDb::me().eraseChallData(m_fightId, it->second.objId1, 0);//轮次结束，删除本场次即可
                it = m_challDatas.erase(it);
                eraseItem = true;
            }
        }

        if(!eraseItem)
            ++it;
    }

    
    if(!m_challDatas.empty())
    {//如果非空，那说明本轮次未结束
        return;
    }

    //轮次结束后
    //找出未淘汰的obj
    std::vector<FightObjData*> validObjs;
    for(auto itObj = m_objs.begin(); itObj != m_objs.end(); itObj++)
    {
        if(itObj->second.status == 1)
            validObjs.push_back(&(itObj->second));
    }
    if(validObjs.size() < 2)
    {//都淘汰了，结束赛事
        stopRule();
        return;
    }

    //正常流程打下去
    createChallData(validObjs);
    //暂时不考虑3.4名争夺

}

void FightRuleSingleFail::stopRule()
{
    LOG_TRACE("单败赛事轮次结束, 赛事结束, fightId={}", m_fightId);
    if(m_parentId != 0)
    {
        auto fight = FightManager::me().getFight(m_parentId);
        if(fight)
        {
            uint32_t num = fight->getNextLvNum();
            std::vector<uint64_t> temp = getTop(num);
            fight->addNextLvMem(temp);
            LOG_TRACE("单败赛事轮次结束, fightId={}", m_fightId);
        }
    }
}

std::vector<uint64_t> FightRuleSingleFail::getTop(uint32_t num)
{
    LOG_DEBUG("单败赛事, 获取名次:num={}", num);
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

void FightRuleSingleFail::fightResult(uint64_t objId, User::Ptr user, uint8_t result, const std::string& pic)
{
    if(result != 2 && result != 1)
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
    //回复提交成功
    FightManager::me().sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send));

    //通知己方队员，提交的结果
    msg::ResultFightToC sendC;
    sendC.result = result;
    for(auto& item : it->second.team)
    {   
        if(item.m_userId == userId)
            item.m_status = 1;
        sendC.userId = item.m_userId;
        User::Ptr mem = UserManager::me().getUser(item.m_userId);
        if(mem)
            FightManager::me().sendtoClient(mem->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToC, &sendC, sizeof(send));
    }   


    //找到对方战队
    auto itCom = m_objs.find(it->second.competitor);
    if(itCom == m_objs.end())
    {
        LOG_ERROR("提交战斗结果, 找不到对方战队");
        return;
    }
    if(itCom->second.team.empty())
    {
        LOG_ERROR("提交战斗结果, 对方战队上阵为空");
        return;
    }


    //更改战斗数据
    {
        LOG_DEBUG("提交战斗结果, 单败赛制, objId={}, result={}", it->second.id, result);
        if(it->second.status != 0)
        {   
            //找到战斗的主键
            uint64_t objId1 = it->second.id;
            if(objId1 > itCom->second.id)
                objId1 = itCom->second.id;

            //找到ChallengeData
            auto itChall = m_challDatas.find(objId1);
            if(itChall != m_challDatas.end())
            {
                if(objId1 == it->second.id)
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
                FightDb::me().updateChallData(m_fightId, challData.objId1, challData.objId2, challData.result1, challData.result2, challData.pic1, challData.pic2);
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
    contrl();
}

void FightRuleSingleFail::signUp(uint64_t objId) 
{
    FightRule::signUp(objId);
}

uint32_t FightRuleSingleFail::getPositionVal(uint32_t num)
{
    if(num == 0)
        return num;

    uint32_t ret = 2;
    while(ret < num)
        ret = ret * 2;
    return ret;
}

bool FightRuleSingleFail::bioOver(uint32_t value1, uint32_t value2)
{
    if(m_rule2 == FightRule2::bio1)
        return (value1 + value2) >= 1;

    if(m_rule2 == FightRule2::bio3)
        return (value1 + value2) >= 3;

    if(m_rule2 == FightRule2::bio5)
        return (value1 + value2) >= 5;

    return false;
}

uint16_t FightRuleSingleFail::getNum()
{
    return getPositionVal(m_objs.size());
}

}
