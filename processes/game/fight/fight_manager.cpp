#include "fight_manager.h"
#include "fight_db.h"
#include "team_manager.h"

#include "component/logger.h"
#include "msg/fight_msg.h"
#include "../game.h"
#include "../user/user_manager.h"
#include "../coffeenet/coffeenet_manager.h"

namespace game{

FightManager& FightManager::me()
{
    static FightManager me;
    return me;
}

void FightManager::contrl()
{
    LOG_DEBUG("赛事管理检测");
    if(!m_intitOver)
    {
        LOG_DEBUG("赛事检测，等待初始化结束");
        return;
    }
    else
    {
        LOG_DEBUG("***************赛事检测，初始化已经结束");
    }
    for(auto it = m_fights.begin(); it != m_fights.end(); ++it) 
    {
        it->second->contrl();
    }
}

void FightManager::init()
{
    LOG_DEBUG("*******赛事管理初始化");
    //加载区域赛事
    initAreaFight();

    //加载推荐赛事
    initRecommendFighting();
}

void FightManager::initOver()
{
    m_intitOver = true;
}

void FightManager::regMsgHandler()
{
    using namespace std::placeholders;
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::recommondFight, std::bind(&FightManager::recommondFight, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::coffRecommondFight, std::bind(&FightManager::coffFightByrRecommond, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::myCoffFight, std::bind(&FightManager::myCoffFight, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::fightMemData, std::bind(&FightManager::fightMemData, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::signUpFight, std::bind(&FightManager::signUp, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::myFight, std::bind(&FightManager::myFight, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::challengeMem, std::bind(&FightManager::ChallengerMem, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::readyFightToS, std::bind(&FightManager::ready, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::fight, (MsgCode2)msg::FightMsgType::resultFightToS, std::bind(&FightManager::fightResult, this, _1, _2, _3));
}

Fight::Ptr FightManager::getFight(uint64_t fightId)
{
    auto it = m_fights.find(fightId);
    if(it == m_fights.end())
        return nullptr;
    return it->second;
}

void FightManager::initAreaFight()
{
    FightDb::me().initAreaFight();
}

void FightManager::initFightObj(FightObjData& objs)
{
    auto fight = FightManager::me().getFight(objs.fightId); 
    if(!fight)
    {
        LOG_ERROR("赛事参赛成员加载, 对应赛事不存在, fightId={}", objs.fightId);
        return;
    }
    fight->addMem(objs);
    m_objToFights[objs.id].insert(objs.fightId);
}

void FightManager::initFightChall(uint64_t fightId, ChallengeData& challs)
{
    auto fight = FightManager::me().getFight(fightId);
    if(!fight)
    {
        LOG_ERROR("赛事战斗加载, 对应赛事不存在, fightId={}", fightId);
        return;
    }
    fight->addChall(challs);
}

//为了web方便，fighting全部放在一张表，有数据冗余
void FightManager::initAreaFightCheck(FightData& data, uint32_t nextNum, uint64_t parentId, std::string area)
{
    Fight::Ptr fight = nullptr;
    if(data.m_fightType == FightType::areaF)
    {
        fight = AreaFight::create(data, nextNum, parentId, area);
        if(!fight->init())
            return;
        LOG_DEBUG("区域赛事加载成功, id={}", data.m_id);
    }
    else
    {
        LOG_DEBUG("未知的赛事类型, fightType={}", data.m_fightType);
        return;
    }
    m_fights[data.m_id] = fight;
    LOG_DEBUG("赛事初始化, id={}, fightType={}", data.m_fightType);
    if(nextNum == 0 && data.m_userTye == UserType::coffeenet)
    {//如果是网吧发布的赛事加入网吧对应的容器
        m_coffFights[data.m_from].insert(data.m_id);
    }
    else if(nextNum == 0 && data.m_userTye == UserType::cityD)
    {//如果是市级代理发布的，加入市对应的赛事
       m_cityFights[area].insert(data.m_id); 
    }
    else if(nextNum == 0 && data.m_userTye == UserType::provinceD)
    {
       m_provinceFights[area].insert(data.m_id); 
    }
    else if(nextNum == 0)
    {
        LOG_ERROR("赛事错误, 全国发布了用户可打赛事, fightId={}", data.m_id);
        return;
    }
}

void FightManager::initRecommendFighting()
{
    FightDb::me().initRecommendFighting();
}

void FightManager::initRecommendFightingCheck(std::vector<uint64_t>& recommonFighting)
{
    m_recommendFights.swap(recommonFighting); 
    LOG_DEBUG("初始化推荐赛事, size={}", m_recommendFights.size());
}

void FightManager::initNewFighting()
{
    if(m_fights.empty())
        FightDb::me().initNewAreaFight(0);
    else
        FightDb::me().initNewAreaFight(m_fights.rbegin()->first);
}

void FightManager::recommondFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("推荐赛事请求, 收到请求");
    if(msgSize < sizeof(msg::RecommondFight))
    {
        LOG_DEBUG("推荐赛事请求, size错误");
        return;
    }

    const msg::RecommondFight* msg = reinterpret_cast<const msg::RecommondFight*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("荐赛事请求, 未登录, userid={}", msg->userId);
        return;
    }

    std::vector<Fight::Ptr> temp;
    for(auto& item : m_recommendFights)
    {
        auto it = m_fights.find(item);
        if(it != m_fights.end())
        {
            temp.push_back(it->second);
        }
    }

    LOG_DEBUG("推荐赛事请求, 回复数量={}", temp.size());
    msg::RetRecommondFight send;
    uint32_t i = 0;
    for(auto& item : temp)
    {
        send.fightData[i].fightId = item->getId();
        item->getPic().copy(send.fightData[i].pic, MAX_PIC_SIZE);
        send.fightData[i].gameType = (uint16_t)item->getGameType();
        i++;
        if(i == 5)
        {
            send.userId = msg->userId;
            send.total = temp.size();
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::recommondFight, &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == temp.size())
    {
        send.userId = msg->userId;
        send.total = temp.size();
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::recommondFight, &send, sizeof(send));
    }
}

void FightManager::coffFightByrRecommond(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("首页推荐对应的网吧赛事, 收到请求");
    if(msgSize < sizeof(msg::CoffRecommondFight))
    {
        LOG_DEBUG("请求首页推荐对应的网吧赛事, size错误");
        return;
    }

    const msg::CoffRecommondFight* msg = reinterpret_cast<const msg::CoffRecommondFight*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("请求首页推荐对应的网吧赛事, 未登录, userId={}", msg->userId);
        return;
    }

    Coffeenet::Ptr coff = CoffeenetManager::me().getCoffeenet(coffeenetId);
    if(!coff)
    {
        LOG_ERROR("请求首页推荐对应的网吧赛事, 找不到自己的网吧");
        return;
    }

    //自己能打的赛事有很多，省、市、国家范围内可以打的，以及自己网吧发布的赛事，而首页推荐的应该只有全国的
    Fight::Ptr sendPtr = nullptr;
    if(!sendPtr)
    {//全国的好找，直接找即可
        auto itAllFight = m_allFights.find(msg->fightId);
        if(itAllFight != m_allFights.end())
        {
            auto itFight = m_fights.find(*itAllFight);
            if(itFight == m_fights.end())
            {
                LOG_ERROR("请求首页推荐对应的网吧赛事, 内存错误, m_allFights中的赛事在管理中不存在");
                return;
            }
            sendPtr = itFight->second;
        }
    }
    if(!sendPtr)
    {//省范围内可打的赛事，首先自己的网吧在这个省内，然后遍历省内可打赛事（不会多）找出匹配或者parent递归匹配的可打赛事
        auto itProvince = m_provinceFights.find(coff->getProvince());
        if(itProvince != m_provinceFights.end())
        {
            for(auto& item : itProvince->second)
            {
                if(sendPtr)
                    break;
                uint64_t fightId = item;
                auto itPtr = m_fights.find(fightId);
                if(itPtr == m_fights.end())
                    continue;
                while(fightId != 0)
                {
                    auto it = m_fights.find(fightId);
                    if(it == m_fights.end())
                        break;
                    if(fightId == msg->fightId)
                    {
                        sendPtr = itPtr->second;
                        break;
                    }
                    //这里如果不是区域赛，那getParentId()得到0
                    fightId = it->second->getParentId();
                }
            }
        }
            
    }
    if(!sendPtr)
    {//市范围内可打的赛事，首先自己的网吧在这个市内，然后遍历省内可打赛事（不会多）找出匹配或者parent递归匹配的可打赛事
        auto itCity = m_cityFights.find(coff->getCity());
        if(itCity != m_cityFights.end())
        {
            for(auto& item : itCity->second)
            {
                if(sendPtr)
                    break;
                uint64_t fightId = item;
                auto itPtr = m_fights.find(fightId);
                if(itPtr == m_fights.end())
                    continue;
                while(fightId != 0)
                {
                    auto it = m_fights.find(fightId);
                    if(it == m_fights.end())
                        break;
                    if(fightId == msg->fightId)
                    {
                        sendPtr = itPtr->second;
                        break;
                    }
                    //这里如果不是区域赛，那getParentId()得到0
                    fightId = it->second->getParentId();
                }
            }
        }
            
    }
    if(!sendPtr)
    {//网吧范围内可打的赛事，首先自己的网吧在这个市内，然后遍历省内可打赛事（不会多）找出匹配或者parent递归匹配的可打赛事
        auto itCoff = m_coffFights.find(coff->getId());
        if(itCoff != m_coffFights.end())
        {
            for(auto& item : itCoff->second)
            {
                if(sendPtr)
                    break;
                uint64_t fightId = item;
                auto itPtr = m_fights.find(fightId);
                if(itPtr == m_fights.end())
                    continue;
                while(fightId != 0)
                {
                    auto it = m_fights.find(fightId);
                    if(it == m_fights.end())
                        break;
                    if(fightId == msg->fightId)
                    {
                        sendPtr = itPtr->second;
                        break;
                    }
                    //这里如果不是区域赛，那getParentId()得到0
                    fightId = it->second->getParentId();
                }
            }
        }
            
    }

    msg::RetCoffRecommondFight send;
    send.userId = msg->userId;
    if(sendPtr && !sendPtr->beStop() && !sendPtr->bePause())
    {
        send.fightData.fightId = sendPtr->getId();
        send.fightData.gameType = (uint16_t)sendPtr->getGameType();
        sendPtr->getName().copy(send.fightData.name, MAX_NAME_SIZE);
        uint64_t teamId = TeamManager::me().getTeamId(msg->userId, sendPtr->getGameType());
        if(sendPtr->inFight(teamId))
            send.fightData.inFight = 1;
        send.fightData.status = (uint16_t)sendPtr->getStatus();
        send.fightData.rule1 = (uint16_t)sendPtr->getRule1();
        send.fightData.rule2 = (uint16_t)sendPtr->getRule2();
        send.fightData.num = (uint16_t)sendPtr->getNum();
    }
    sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::coffRecommondFight, &send, sizeof(send));
}

void FightManager::fightMemData(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("赛事成员列表, 收到请求");
    if(msgSize < sizeof(msg::FightMemData))
    {
        LOG_DEBUG("赛事成员列表, size错误");
        return;
    }

    const msg::FightMemData* msg = reinterpret_cast<const msg::FightMemData*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("事成员列表, 未登录, userId={}", msg->userId);
        return;
    }

    auto it = m_fights.find(msg->fightId);
    if(it == m_fights.end())
    {
         LOG_DEBUG("赛事成员列表, 未找到对应的赛事, fightId={}", msg->fightId);
         return;
    }

    auto fight = it->second;

    msg::RetFightMemData send;
    uint32_t i = 0;
    const std::map<uint64_t, FightObjData>& fightObjData = it->second->getMemData();
    for(auto it = fightObjData.begin(); it != fightObjData.end(); ++it)
    {

        const BaseTeamData* teamData = TeamManager::me().getData(it->first);
        if(!teamData)
        {
            LOG_DEBUG("赛事成员列表, 为找到成员(战队)基础数据");
            continue;
        } 
        send.data[i].memId = it->first;
        teamData->name.copy(send.data[i].name, MAX_NAME_SIZE);
        send.data[i].value = it->second.value;
        send.data[i].position = it->second.position;
        i++;
        
        if(i == 5)
        {
            send.userId = msg->userId;
            send.total = fightObjData.size(); 
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::fightMemData, &send, sizeof(send));
            i = 0;
            memset(&send, 0, MAX_NAME_SIZE);
        }
    }
    if(i != 0 || i == fightObjData.size())
    {
        send.userId = msg->userId;
        send.total = fightObjData.size(); 
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::fightMemData, &send, sizeof(send));
    }

}

void FightManager::myCoffFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("所在网吧的赛事, 收到请求");
    if(msgSize < sizeof(msg::MyCoffFight))
    {
        LOG_DEBUG("所在网吧的赛事, 消息size错误");
        return;
    }

    const msg::MyCoffFight* msg = reinterpret_cast<const msg::MyCoffFight*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("所在网吧的赛事, 未登录, userId={}", msg->userId);
        return;
    }

    std::vector<Fight::Ptr> temp;
    //网吧对应的
    auto it = m_coffFights.find(user->getCoffeenetId());
    if(it != m_coffFights.end())
    {
        for(auto& item : it->second)
        {
            auto fight = m_fights.find(item);
            if(fight == m_fights.end())
            {
                LOG_ERROR("所在网吧的赛事, m_fights找不到对应赛事, item={}", item);
                break;
            }
            if(!fight->second->beStop() && !fight->second->bePause())
                temp.push_back(fight->second);
        }
    }
    Coffeenet::Ptr coff = CoffeenetManager::me().getCoffeenet(coffeenetId);
    if(!coff)
    {
        LOG_ERROR("请求首页推荐对应的网吧赛事, 找不到自己的网吧");
        return;
    }
    //省对应的
    auto itProvince = m_provinceFights.find(coff->getProvince());
    if(itProvince != m_provinceFights.end())
    {
        for(auto& item : itProvince->second)
        {
            auto fight = m_fights.find(item);
            if(fight == m_fights.end())
            {
                LOG_ERROR("所在省的赛事, m_fights找不到对应赛事, item={}", item);
                break;
            }
            if(!fight->second->beStop() && !fight->second->bePause())
            temp.push_back(fight->second);
        }
    }
    //市对应的
    auto itCity = m_cityFights.find(coff->getCity());
    if(itCity != m_cityFights.end())
    {
        for(auto& item : itCity->second)
        {
            auto fight = m_fights.find(item);
            if(fight == m_fights.end())
            {
                LOG_ERROR("所在省的赛事, m_fights找不到对应赛事, item={}", item);
                break;
            }
            if(!fight->second->beStop() && !fight->second->bePause())
            temp.push_back(fight->second);
        }
    }

    LOG_DEBUG("所在网吧的赛事, 回复数量:", temp.size());
    msg::RetMyCoffFight send;
    uint32_t i = 0;
    for(auto& item : temp)
    {
        send.fightData[i].fightId = item->getId();
        item->getName().copy(send.fightData[i].name, MAX_NAME_SIZE);
        send.fightData[i].gameType = (uint16_t)item->getGameType();
        item->getPic().copy(send.fightData[i].pic, MAX_PIC_SIZE);
        uint64_t teamId = TeamManager::me().getTeamId(msg->userId, item->getGameType());
        if(item->inFight(teamId))
            send.fightData[i].inFight = 1;

        send.fightData[i].status = (uint16_t)item->getStatus();
        send.fightData[i].rule1 = (uint16_t)item->getRule1();
        send.fightData[i].rule2 = (uint16_t)item->getRule2();
        send.fightData[i].num = (uint16_t)item->getNum();

        i++;
        if(i == 5)
        {
            send.total = temp.size();
            send.userId = msg->userId;
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::myCoffFight, &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == temp.size())
    {
        send.total = temp.size();
        send.userId = msg->userId;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::myCoffFight, &send, sizeof(send));
    }
}

void FightManager::signUp(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("报名参加赛事, 收到消息");
    if(msgSize < sizeof(msg::SignUp))
    {
        LOG_DEBUG("报名参加赛事, 消息size错误");
        return;
    }
    const msg::SignUp* msg = reinterpret_cast<const msg::SignUp*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("报名参加赛事, 用户未登录, id={}", msg->userId);
        return;
    }

    msg::RetSignUp send;
    send.userId = msg->userId;
    send.result = 1;//成功
    auto it = m_fights.find(msg->fightId);
    if(it == m_fights.end())
    {
        //无对应赛事
        LOG_DEBUG("报名参加赛事, 没找到对应的赛事");
        send.result = 2;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
        return;
    }

    //是否是报名状态
    if(!it->second->beApply())
    {
        LOG_DEBUG("报名参加赛事, 不是报名状态");
        send.result = 3;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
        return;
    }
    //是否实名认证
    if(!user->haveIdentify())
    {
        LOG_DEBUG("报名参加赛事, 没有实名认证");
        send.result = 4;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
        return;
    }
    uint64_t objId = 0;
    //if(!it->second->oneGame())
    {
        objId = TeamManager::me().getTeamId(msg->userId, it->second->getGameType());
        if(objId == 0)
        {
            LOG_DEBUG("报名参加赛事, 没有对应类型的战队，无法报名");
            send.result = 5;//不是队长或副队长，不能报名
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
            return;
        }
        if(!TeamManager::me().beLeader(msg->userId, objId))
        {
            LOG_DEBUG("报名参加赛事, 不是对应类型的战队的队长，无法报名");
            send.result = 6;//不是队长或副队长，不能报名
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
            return;
        }
    }
    //else
    {
        //objId = user->getId();
    }
    //判断对应的游戏
    if(!(user->getGameServer(it->second->getGameType()) == it->second->getGameServer()))
    {
        LOG_DEBUG("报名参加赛事, 个人游戏服与赛事游戏服不符");
        send.result = 7;//服不一致
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
        return;
    
    }
    //是否已经报名
    if(it->second->inFight(objId))
    {
        LOG_DEBUG("报名参加赛事, 已经报名了，无法再报名");
        send.result = 8;//没有对应游戏类型战队
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
        return;
    }

    //查询战队成员数量
    if(it->second->getGameType() == GameType::lol)
    {
        uint32_t num = TeamManager::me().getTeamerNum(objId);
        if(num < 5)
        {
            send.result = 9;
            LOG_DEBUG("报名参加赛事, 战队成员数量不足, teamId={}, num={}", objId, num);
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
            return;
        }
    }
    else if(it->second->getGameType() == GameType::dota2)
    {
        uint32_t num = TeamManager::me().getTeamerNum(objId);
        if(num < 5)
        {
            send.result = 9;
            LOG_DEBUG("报名参加赛事, 战队成员数量不足");
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
            return;
        }
    }
    else if(it->second->getGameType() == GameType::shouwangxianfeng)
    {
        uint32_t num = TeamManager::me().getTeamerNum(objId);
        if(num < 6)
        {
            send.result = 9;
            LOG_DEBUG("报名参加赛事, 战队成员数量不足");
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
            return;
        }
    }
    else if(it->second->getGameType() == GameType::lushi)
    {
    }


    //报名成功
    it->second->signUp(objId);
    m_objToFights[msg->userId].insert(msg->fightId);

    sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::signUpFight, &send, sizeof(send));
}

void FightManager::myFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::MyFight))
    {
        LOG_DEBUG("消息size错误");
        return;
    }

    const msg::MyFight* msg = reinterpret_cast<const msg::MyFight*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("未登录");
        return;
    }

    msg::RetMyFight send;
    send.userId = msg->userId;
    std::vector<uint64_t> teams = TeamManager::me().me().getAllTeam(msg->userId);

    std::vector<Fight::Ptr> temp;
    for(auto& item : teams)
    {
        auto it  = m_objToFights.find(item);
        if(it == m_objToFights.end())
        {
            LOG_DEBUG("自己没有对应赛事");
            continue;
        }

        for(auto& itemSet : it->second)
        {
            auto itFight = m_fights.find(itemSet);
            if(itFight != m_fights.end())
                temp.push_back(itFight->second);
        }
    }

    uint32_t i = 0;
    for(auto& item : temp)
    {
        send.fightData[i].fightId = item->getId();
        send.fightData[i].gameType = (uint16_t)item->getGameType();
        item->getName().copy(send.fightData[i].name, MAX_NAME_SIZE);
        item->getPic().copy(send.fightData[i].pic, MAX_PIC_SIZE);

        i++;
    
        if(i == 5)
        {
            send.userId = msg->userId;
            send.total = temp.size();
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::myFight, &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == temp.size())
    {
        send.userId = msg->userId;
        send.total = temp.size();
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::myFight, &send, sizeof(send));
    }
}

/*
void FightManager::myFightMem(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("自己的赛事数据, 收到请求");
    if(msgSize < sizeof(MyfightMem))
    {
        return; 
    }
    const msg::MyfightMem* msg = reinterpret_cast<const msg::MyfightMem*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("自己的赛事数据, 未登录, userId={}", msg->userId);
        return;
    }
}*/

void FightManager::ChallengerMem(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    LOG_DEBUG("挑战人员消息, 收到请求");
    if(msgSize < sizeof(msg::ChallengerMem))
    {
        return;
    }

    const msg::ChallengerMem* msg = reinterpret_cast<const msg::ChallengerMem*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("挑战人员消息, 未登录");
        return;
    }

    msg::RetChallengerMem send;
    send.userId = msg->userId;
    auto it = m_fights.find(msg->fightId);
    if(it == m_fights.end())
    {
        LOG_DEBUG("挑战人员, 赛事id错误, fightId={}", msg->fightId);
        send.result = 3;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
    }

    std::vector<User::Ptr> temp;
    for(int i = 0; i < 6; i++)
    {
        if(msg->mem[i] != 0)
        {
            User::Ptr mem = UserManager::me().getUser(msg->mem[i]);
            if(!mem)
            {
                LOG_DEBUG("挑战人员 有不在线的队员，不能创建房间, memId={}", msg->mem[i]);
                send.result = 2;    
                sendtoClient(mem->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
                return;
            }
            temp.push_back(mem);
        }
    }

    //找到战队
    uint64_t teamId = TeamManager::me().getTeamId(msg->userId, it->second->getGameType());
    if(teamId == 0)
    {
        LOG_DEBUG("挑战人员, 有的成员不存在对应战队, teamId={}", teamId);
        send.result = 4;    
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
        return;

    }

    //是否已经报名
    if(!it->second->haveSignUp(teamId))
    {
        LOG_DEBUG("挑战人员, 未报名, fightId={}", msg->fightId);
        send.result = 6;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
    }
    if(!TeamManager::me().beLeader(msg->userId, teamId))
    {
        LOG_DEBUG("挑战人员, 发起人不是队长, teamId={}, userId={}", teamId, msg->userId);
        send.result = 5;    
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
        return;
    }

    //验证成员是否在线，验证战队
    std::set<uint64_t> setT;//用于验证是否有重复
    //setT.insert(msg->userId);
    for(auto& item : temp)
    {
        if(!setT.insert(item->getId()).second)
        {
            LOG_DEBUG("挑战人员, 人员重复或者包含了队长, teamId={}, userId={}", teamId, item->getId());
            send.result = 7;    
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
            return;
            
        }
        if(!TeamManager::me().inTeam(item->getId(), teamId))
        {
            LOG_DEBUG("挑战人员, 有的成员不存在对应战队, teamId={}, userId={}", teamId, item->getId());
            send.result = 4;    
            sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
            return;
        }

    }

    //通知双方队员进入准备界面
    msg::HouseCreate sendC;
    sendC.fightId = it->second->getId();
    sendC.gameType = (uint16_t)it->second->getGameType();
    it->second->getName().copy(sendC.name, MAX_NAME_SIZE);
    //己方的成员
    int i = 0;
    for(auto& item : temp)
    {
        sendC.data[i].memId = item->getId();
        item->getName().copy(sendC.data[i].name, MAX_NAME_SIZE);
        item->getPic().copy(sendC.data[i].pic, MAX_PIC_SIZE);
        i++;
    }
    //对方的成员
    const FightObjData* comMem = it->second->getCompData(teamId);
    if(!comMem)
    {
        LOG_DEBUG("挑战人员, 轮空，客户端不改有此请求");
        return;
    }
    i = 0;
    for(auto& item : comMem->team)
    {
        const BaseUserData* userData = TeamManager::me().getUserData(item.m_userId);
        if(userData)
        {
            userData->name.copy(sendC.dataCom[i].name, MAX_NAME_SIZE);
            userData->pic.copy(sendC.dataCom[i].pic, MAX_PIC_SIZE);
        }
        sendC.dataCom[i].memId = item.m_userId;
        sendC.dataCom[i].status = item.m_status;
    }
    
    //
    std::vector<FightTeamData> housememId;
    FightTeamData dataLeader(0);
    for(auto& item : temp)
    {
        sendC.userId = item->getId();
        {
            sendtoClient(item->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::houseCreate, &sendC, sizeof(sendC)); 
            LOG_DEBUG("通知队员进入准备界面, userId={}", item->getId());
        }
        FightTeamData data(item->getId());
        if(item->getId() != msg->userId)
            housememId.push_back(data);
        else
            dataLeader = data;
    }
    housememId.push_back(dataLeader);

    //创建参赛个体中的成员小组（房间）
    it->second->createHouse(teamId, housememId);
    send.result = 1;    
    sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::challengeMem, &send, sizeof(send)); 
}

void FightManager::ready(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::ReadyFightToS))
    {
        LOG_DEBUG("准备, 消息size错误");
        return;
    }

    const msg::ReadyFightToS* msg = reinterpret_cast<const msg::ReadyFightToS*>(msgData);

    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("准备, 未登录");
        return;
    }
    msg::RetReadyFightToS send;
    send.userId = msg->userId;
    send.result = 1;
    auto it = m_fights.find(msg->fightId);
    if(it == m_fights.end())
    {
        LOG_DEBUG("准备, 没有赛事");
        send.result = 4;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToS, &send, sizeof(send)); 
        return;
    }

    //找到战队
    uint64_t teamId = TeamManager::me().getTeamId(msg->userId, it->second->getGameType());
    if(teamId == 0)
    {
        LOG_DEBUG("准备, 不存在对应战队, teamId={}", teamId);
        send.result = 4;    
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::readyFightToS, &send, sizeof(send)); 
        return;
    }
    //直接在赛事中执行准备，各类赛事相关的判断也在里面进行
    it->second->ready(teamId, user);
}

void FightManager::fightResult(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::ResultFightToS))
    {
        LOG_DEBUG("战斗结果, 消息size错误");
        return;
    }

    const msg::ResultFightToS* msg = reinterpret_cast<const msg::ResultFightToS*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("战斗结果, 未登录");
        return;
    }
    msg::RetResultFightToS send;
    send.userId = msg->userId;
    send.result = 1;
    auto it = m_fights.find(msg->fightId);
    if(it == m_fights.end())
    {
        LOG_DEBUG("战斗结果, 未找到对应赛事, fightId={}", msg->fightId);
        send.result = 2;
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send));
        return;
    }
    //找到战队
    int64_t teamId = TeamManager::me().getTeamId(msg->userId, it->second->getGameType());
    if(teamId == 0)
    {
        LOG_DEBUG("战斗结果, 不存在对应战队, teamId={}", teamId);
        send.result = 3;    
        sendtoClient(user->getCoffeenetId(), (water::process::MsgCode2)msg::FightMsgType::resultFightToS, &send, sizeof(send)); 
        return;
    }
    std::string pic(msg->resultPic, MAX_PIC_SIZE < sizeof(msg->resultPic)?MAX_PIC_SIZE:sizeof(msg->resultPic));
    it->second->fightResult(teamId, user, msg->result, pic);
}

void FightManager::sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize)
{
    water::process::MsgCode1 code1 = (water::process::MsgCode1)msg::MsgType::fight;
    Game::me().sendtoClient(coffeenetId, code1, code2, data, dataSize); 
}


}
