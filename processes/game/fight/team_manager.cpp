#include "team_manager.h"
#include "fight_db.h"

#include "component/logger.h"
#include "msg/team_msg.h"
#include "../user/user_manager.h"
#include "../game.h"

namespace game{

TeamManager& TeamManager::me()
{
    static TeamManager me;
    return me;
}

void TeamManager::init()
{
    FightDb::me().initAllTeam();//战队加载完后加载user，最后加载user的战队信息
    FightDb::me().initAllCoff();//网吧加载
}

void TeamManager::regMsgHandler()
{
    using namespace std::placeholders;
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::createTeam, std::bind(&TeamManager::createTeam, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::TeamInfo, std::bind(&TeamManager::teamInfo, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::JoinTeamToS, std::bind(&TeamManager::JoinTeamToS, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::DealJoinTeamToS, std::bind(&TeamManager::DealJoinTeamToS, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::TeamerList, std::bind(&TeamManager::TeamerList, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::CoffTeam, std::bind(&TeamManager::coffTeam, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::allGameServer, std::bind(&TeamManager::allGameServer, this, _1, _2, _3));
    RawmsgManager::me().regHandler((MsgCode1)msg::MsgType::team, (MsgCode2)msg::TeamMsgType::findTeam, std::bind(&TeamManager::findTeam, this, _1, _2, _3));

}

std::vector<uint64_t> TeamManager::getAllTeam(uint64_t userId)
{
    std::vector<uint64_t> ret;
    auto it = m_users.find(userId);
    if(it == m_users.end())
        return ret;

    for(auto itTeam = it->second.teams.begin(); itTeam != it->second.teams.end(); itTeam++)
    {
        ret.push_back(itTeam->second);
    }
    return ret;
}

void TeamManager::insertTeam(BaseTeamData& data)
{
    m_teams[data.id] = data; 
    auto& teams = m_coffTeams[data.coffnetId];
    teams.insert(data.id);
}

void TeamManager::insertMem(BaseUserData& data)
{
    m_users[data.id] = data;
}

void TeamManager::insertCoff(CoffeenetBaseData& data)
{
    auto it = m_coffs.find(data.id);
    if(it != m_coffs.end())
        m_coffs[data.id] = data;
}

void TeamManager::insertMemGame(uint64_t userId, GameType gameType, uint16_t gameServer, std::string gameName)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
    {
        LOG_ERROR("角色游戏信息加载错误, 内存中没有对应的角色");
        return;
    }
    GameInfo data;
    data.gameServer = gameServer;
    data.gameName = gameName;
    it->second.gameInfo[gameType] = data;
}

void TeamManager::insertMemTeam(uint64_t userId, uint64_t teamId, uint16_t status)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
    {
        LOG_DEBUG("角色战队信息加载错误, 内存中没有对应的角色");
        return;
    }
    auto itTeam = m_teams.find(teamId);
    if(itTeam == m_teams.end())
    {
        return;
    }

    it->second.teams[itTeam->second.gameType] = teamId;
    if(status == 1)
        itTeam->second.allMems.insert(userId);
    else
        itTeam->second.waitMems.insert(userId);
}

bool TeamManager::isGameServer(uint64_t userId, GameType gameType, uint16_t gameServer)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
        return false;
    auto itGame = it->second.gameInfo.find(gameType);
    if(itGame == it->second.gameInfo.end())
        return false;
    if(gameServer != itGame->second.gameServer)
        return false;
    return true;
}

bool TeamManager::haveTeam(uint64_t userId, GameType gameType)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
        return false;
    auto itTeam = it->second.teams.find(gameType);
    if(itTeam == it->second.teams.end())
        return false;
    return true;
}

uint64_t TeamManager::createTeamId()
{
    if(m_teams.empty())
        return 1;
    return m_teams.rbegin()->second.id + 1;
}

void TeamManager::setTeamId(uint64_t userId, GameType gameType, uint64_t teamId, uint16_t status)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
    {
        LOG_DEBUG("加入战队, 内存错误, 找不到对应的user, userId={}", userId);
        return;
    }
    auto itTeam = m_teams.find(teamId);
    if(itTeam == m_teams.end())
    {
        LOG_DEBUG("加入战队, 内存错误, 找不到对应的team, teamId={}", teamId);
        return;
    }

    if(status == 1)
    {
        it->second.teams[gameType] = teamId;
        itTeam->second.allMems.insert(userId);
        itTeam->second.waitMems.erase(userId);
    }
    else
    {
        itTeam->second.waitMems.insert(userId);
    }
}

void TeamManager::createTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::CreateTeam))
    {
        LOG_DEBUG("创建战队, 消息size错误, msgSize={}, needSize={}", msgSize, sizeof(msg::CreateTeam));
        return;
    }
    const msg::CreateTeam* msg = reinterpret_cast<const msg::CreateTeam*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId); 
    if(!user)
    {
        LOG_DEBUG("创建战队, 未登录的角色, userId={}", msg->userId);
        return;
    }

    //准备返回的数据
    msg::RetCreateTeam send;
    send.userId = user->getId();
    send.result = 1;

    //如果没有实名认证返回提示
    if(!user->haveIdentify())
    {
        send.result = 3;//未实名
        LOG_DEBUG("创建战队, 实名认证服务器不符或未实名认证, id={}", msg->userId);
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::createTeam, &send, sizeof(send));
        return;
    }

    //检查是否对应自己的游戏信息
    if(!isGameServer(msg->userId, (GameType)msg->gameType, msg->gameServer))
    {
        send.result = 4;//游戏信息不匹配
        LOG_DEBUG("创建战队, 游戏信息与个人的不匹配, gameTye={}, gameServer={}", msg->gameType, msg->gameServer);
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::createTeam, &send, sizeof(send));
        return;
    }

    //安全的得到客户端传来的数据
    std::string teamName(msg->teamName, MAX_NAME_SIZE<strlen(msg->teamName)?MAX_NAME_SIZE:strlen(msg->teamName));
    std::string pic(msg->pic, MAX_NAME_SIZE<strlen(msg->pic)?MAX_NAME_SIZE:strlen(msg->pic));

    //检查是否已经加入该类型游戏队伍
    if(haveTeam(msg->userId, (GameType)msg->gameType)) 
    {
        LOG_DEBUG("创建战队, 已经存在 {} 游戏的战队, 不能创建", msg->gameType);
        send.result = 2;
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::createTeam, &send, sizeof(send));
        return;
    }

    //内存生成
    BaseTeamData teamData;
    teamData.id = createTeamId();
    teamData.name = teamName;
    teamData.gameType = (GameType)msg->gameType;
    teamData.gameServer = msg->gameServer;
    teamData.coffnetId = msg->coffeenetId;
    teamData.leader = msg->userId;
    teamData.viceLeader = 0;
    teamData.allMems.insert(msg->userId);
    m_teams[teamData.id] = teamData;

    setTeamId(msg->userId, teamData.gameType, teamData.id, 1);

    //通知user发送同步主数据
    user->sendUserData();

    //插入数据库
    FightDb::me().updateTeam(teamData.id, msg->pic, msg->teamName, msg->gameType, msg->gameServer, msg->coffeenetId, msg->userId, 0);
    FightDb::me().updateTeamMem(msg->userId, teamData.id, 1);

    LOG_DEBUG("创建战队验证, 成功, teamId={}", teamData.id);
    send.userId = msg->userId;
    send.result = 1;
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::createTeam, &send, sizeof(send));
}

void TeamManager::teamInfo(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::TeamInfo))
    {
        LOG_DEBUG("获取战队信息, 消息size错误, msgSize={}, needSize={}", msgSize, sizeof(msg::TeamInfo));
        return;
    }

    const msg::TeamInfo* msg = reinterpret_cast<const msg::TeamInfo*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("获取战队信息, 用户未登录, userId={}", msg->userId); 
        return;
    }

    auto it = m_teams.find(msg->teamId);
    if(it == m_teams.end())
    {
        LOG_DEBUG("获取战队信息, 战队不存在, teamId={}", msg->teamId);
        msg::RetTeamInfo send;
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::TeamInfo, &send, sizeof(send));
        return;
    }

    msg::RetTeamInfo send;
    send.userId = msg->userId;
    send.teamId = it->second.id;
    it->second.pic.copy(send.pic, MAX_PIC_SIZE);
    it->second.name.copy(send.teamName, MAX_NAME_SIZE);
    send.gameType = (uint16_t)it->second.gameType;
    send.gameServer = it->second.gameServer;
    std::string coffName = "暂时没有";
    coffName.copy(send.coffeenetName, MAX_NAME_SIZE);
    auto itCoff = m_coffs.find(it->second.coffnetId);
    if(itCoff != m_coffs.end())
    {
        itCoff->second.name.copy(send.coffeenetName, MAX_NAME_SIZE);
    }
    auto itUser = m_users.find(it->second.leader);
    if(itUser != m_users.end())
    {
        itUser->second.name.copy(send.leaderName, MAX_NAME_SIZE);
        itUser->second.tel.copy(send.tel, MAX_TEL_SIZE);
    }
    send.leader = it->second.leader;
    send.viceLeader = it->second.viceLeader;

    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::TeamInfo, &send, sizeof(send));
}

void TeamManager::TeamerList(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::TeamerList))
    {
        LOG_ERROR("获取战队成员列表, 错误的信息size, msgSize={}, needSize={}", msgSize, sizeof(msg::TeamerList));
        return;
    }
    const msg::TeamerList* msg = reinterpret_cast<const msg::TeamerList*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("获取战队成员列表, 用户未登录, userId={}", msg->userId); 
        return;
    }

    auto it = m_teams.find(msg->teamId);
    if(it == m_teams.end())
    {
        LOG_DEBUG("获取战队成员列表, 战队不存在, teamId={}", msg->teamId);
        msg::RetTeamerList send;
        send.userId = msg->userId;
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::TeamerList, &send, sizeof(send));
        return;
    }

    std::vector<TeamerData> teamerList;
    for(auto& item : it->second.allMems)
    {
        auto itUser = m_users.find(item);
        if(itUser == m_users.end())
        {
            LOG_ERROR("user缓存内存错误, 找不到对应的user, userId={}", item);
            continue;
        }
        TeamerData data;
        data.userId = item;
        data.userName = itUser->second.name;
        data.userStatus = 1;
        User::Ptr online = UserManager::me().getUser(item);
        if(online)
            data.online = 2;
    
        data.tel = itUser->second.tel;
        auto itGameInfo = itUser->second.gameInfo.find(it->second.gameType);
        if(itGameInfo != itUser->second.gameInfo.end())
            data.gameName = itGameInfo->second.gameName;
        teamerList.push_back(data);
    }
    for(auto& item : it->second.waitMems)
    {
        auto itUser = m_users.find(item);
        if(itUser == m_users.end())
        {
            LOG_ERROR("user缓存内存错误, 找不到对应的user, userId={}", item);
            continue;
        }
        TeamerData data;
        data.userId = item;
        data.userName = itUser->second.name;
        data.userStatus = 2;
        data.tel = itUser->second.tel;
        auto itGameInfo = itUser->second.gameInfo.find(it->second.gameType);
        if(itGameInfo != itUser->second.gameInfo.end())
            data.gameName = itGameInfo->second.gameName;
        User::Ptr online = UserManager::me().getUser(item);
        if(online)
            data.online = 2;
        teamerList.push_back(data);
    }

    msg::RetTeamerList send;
    uint32_t i = 0;
    for(auto& item : teamerList)
    {
        send.data[i].userId = item.userId;
        item.userName.copy(send.data[i].userName, MAX_NAME_SIZE);
        send.data[i].userStatus = item.userStatus;
        send.data[i].online = item.online;
        item.tel.copy(send.data[i].tel, MAX_TEL_SIZE);
        item.gameName.copy(send.data[i].gameName, MAX_NAME_SIZE);
        User::Ptr online = UserManager::me().getUser(item.userId);
        if(online)
            send.data[i].online = 1;
        i++;
        if(i == 5)
        {
            send.userId = msg->userId;
            send.totalSize = teamerList.size();
            sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::TeamerList, &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || i == teamerList.size())
    {
        send.userId = msg->userId;
        send.totalSize = teamerList.size();
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::TeamerList, &send, sizeof(send));
    }
}

void TeamManager::JoinTeamToS(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::JoinTeamToS))
    {
        LOG_DEBUG("申请加入战队c->s, 消息size错误");
        return;
    }
    
    const msg::JoinTeamToS* msg = reinterpret_cast<const msg::JoinTeamToS*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("申请加入战队c->s, 用户未登录, userId={}", msg->userId); 
        return;
    }

    msg::RetJoinTeamToS send;
    send.userId = msg->userId;

    if(!user->haveIdentify())
    {
        LOG_DEBUG("请加入战队c->s验证, 未实名验证, userId={}", msg->userId);
        send.result = 4;//未实名
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JoinTeamToS, &send, sizeof(send));
        return;
    }

    auto it = m_teams.find(msg->teamId);
    if(it == m_teams.end())
    {
        LOG_DEBUG("申请加入战队c->s, 战队在内存中未找到, teamId={}", msg->teamId);
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JoinTeamToS, &send, sizeof(send));
        return;
    }
    //检查是否对应自己的游戏信息
    if(!isGameServer(msg->userId, it->second.gameType, it->second.gameServer))
    {
        LOG_DEBUG("加入战队c->s验证, 游戏信息不匹配");
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JoinTeamToS, &send, sizeof(send));
        return;
    }
    if(haveTeam(msg->userId, it->second.gameType))
    {
        LOG_DEBUG("申请加入战队c->s验证, 已经存在对应类型的游戏, 不能申请, 流程结束");
        send.result = 3;//已经有对应战队
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JoinTeamToS, &send, sizeof(send));
        return;
    }
    if(it->second.allMems.size() >= TEAMER_MAX_NUM)
    {
        LOG_DEBUG("申请加入战队c->s验证, 人员已满");
        send.result = 2;//人数已满
    }
    else
    {
        //设置申请
        FightDb::me().updateTeamMem(msg->userId, it->second.id, 2);
        it->second.waitMems.insert(msg->userId);
        LOG_DEBUG("申请加入战队c->s验证, 申请已接受");
        send.result = 1;//成功
    }
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JoinTeamToS, &send, sizeof(send));

    msg::JointTeamToC sendC;
    //通知队长
    User::Ptr leaderP = UserManager::me().getUser(it->second.leader);
    if(leaderP)
    {
        sendC.userId = it->second.leader;
        sendtoClient(leaderP->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JointTeamToC, &sendC, sizeof(sendC));
    }
    //通知副队长
    User::Ptr leaderViceP = UserManager::me().getUser(it->second.viceLeader);
    if(leaderViceP)
    {
        sendC.userId = it->second.viceLeader;
        sendtoClient(leaderViceP->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::JointTeamToC, &sendC, sizeof(sendC));
    }

}

void TeamManager::DealJoinTeamToS(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::DealJoinTeamToS))
    {
        LOG_DEBUG("战队申请处理, 消息size错误, msgSize={}, needSize={}", msgSize, sizeof(msg::DealJoinTeamToS));
        return;
    }
    const msg::DealJoinTeamToS* msg = reinterpret_cast<const msg::DealJoinTeamToS*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("战队申请处理, 用户未登录");
        return;
    }

    auto it = m_teams.find(msg->teamId);
    if(it == m_teams.end())
    {
        LOG_DEBUG("战队申请处理, 战队在内存中未找到, teamId={}", msg->teamId);
        return;
    }
    if(it->second.allMems.size() >= TEAMER_MAX_NUM)
    {
        LOG_DEBUG("战队申请处理, 人员已满");
        return;
    }

    msg::RetDealJoinTeamToS send;
    send.userId = msg->userId;
    send.teamId = msg->teamId;
    send.dealUserId = msg->dealUserId;
    send.result = 1;

    if(msg->type == (uint8_t)1)
    {
        LOG_DEBUG("战队申请处理, 同意");
        setTeamId(msg->dealUserId, it->second.gameType, msg->teamId, 1);
        FightDb::me().updateTeamMem(msg->dealUserId, msg->teamId, 1);
        it->second.allMems.insert(msg->dealUserId);
        User::Ptr dealUser = UserManager::me().getUser(msg->dealUserId);
        if(dealUser)
        {
            //通知被处理人
            msg::DealJoinTeamToC sendC;
            sendC.userId = msg->dealUserId;
            sendC.TeamId = msg->teamId;
            dealUser->sendUserData();
            sendtoClient(dealUser->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::DealJoinTeamToC, &sendC, sizeof(sendC));
        } 
    }
    else
    {
        send.result = 2;
        LOG_DEBUG("战队申请处理, 拒绝");
        FightDb::me().refuseInTeam(msg->dealUserId, msg->teamId);
        //拒绝不需要通知被处理人
    }
    it->second.waitMems.erase(msg->dealUserId);
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::DealJoinTeamToS, &send, sizeof(send));
}

void TeamManager::findTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::FindTeam))
    {
        LOG_DEBUG("搜索战队, 消息size 错误");
        return;
    }

    const msg::FindTeam* msg = reinterpret_cast<const msg::FindTeam*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("搜索战队, 用户未登录");
        return;
    }

    msg::RetFindTeam send;
    send.userId = msg->userId;

    auto it = m_teams.find(msg->teamId);
    if(it == m_teams.end())
    {
        LOG_DEBUG("搜索战队, 战队id不存在, teamId={}", msg->teamId);
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::findTeam, &send, sizeof(send));
        return;
    }

    send.teamId = msg->teamId;
    it->second.name.copy(send.name, MAX_NAME_SIZE);
    sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::findTeam, &send, sizeof(send));
}

void TeamManager::coffTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::CoffTeam))
    {
        LOG_DEBUG("网吧战队列表, 消息size错误");
        return;
    }

    const msg::CoffTeam* msg = reinterpret_cast<const msg::CoffTeam*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("网吧战队列表, 用户未登录");
        return;
    }

    std::vector<TeamData> teamDatas;
    auto it = m_coffTeams.find(msg->coffeenetId);
    if(it != m_coffTeams.end())
    {
        for(auto& item : it->second)
        {
            auto itTeam = m_teams.find(item);
            if(itTeam != m_teams.end())
            {
                TeamData data;
                data.teamId = itTeam->second.id;
                data.pic = itTeam->second.pic;
                data.name = itTeam->second.name;
                data.gameType = itTeam->second.gameType;
                data.gameServer = itTeam->second.gameServer;
                teamDatas.push_back(data);
            }
        }
    }

    msg::RetCoffTeam send;
    int i = 0;
    for(auto& item : teamDatas)
    {
        send.teamData[i].teamId = item.teamId;
        item.pic.copy(send.teamData[i].pic, MAX_PIC_SIZE);
        item.name.copy(send.teamData[i].name, MAX_NAME_SIZE);
        send.teamData[i].gameType = (uint16_t)item.gameType;
        send.teamData[i].gameServer = item.gameServer;

        i++;
        if(i == 5)
        {
            send.userId = msg->userId;
            send.total = teamDatas.size();
            sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::CoffTeam, &send, sizeof(send));
            memset(&send, 0, sizeof(send));
            i = 0;
        }
    }
    if(i != 0 || teamDatas.empty())
    {
        send.userId = msg->userId;
        send.total = teamDatas.size();
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::CoffTeam, &send, sizeof(send));
    }
}

void TeamManager::allGameServer(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId)
{
    if(msgSize < sizeof(msg::AllGameServer))
    {
        LOG_DEBUG("游戏服务器列表, 消息size错误");
        return;
    }

    const msg::AllGameServer* msg = reinterpret_cast<const msg::AllGameServer*>(msgData);
    User::Ptr user = UserManager::me().getUser(msg->userId);
    if(!user)
    {
        LOG_DEBUG("戏服务器列表, 用户未登录");
        return;
    }
    
    FightDb::me().selectGameServer(msg->userId, msg->gameType);
}

void TeamManager::allGameServerCheck(uint64_t userId, std::vector<GameServerData>& datas)
{
    User::Ptr user = UserManager::me().getUser(userId);
    if(!user)
    {
        LOG_DEBUG("戏服务器列表, 用户未登录");
        return;
    }
    msg::RetAllGameServer send;
    uint32_t i = 0;
    for(auto& item : datas)
    {
        send.data[i].id = item.id;
        item.name.copy(send.data[i].serverName, MAX_NAME_SIZE);
        i++;

        if(i == 5)
        {
            send.userId = userId;
            send.total = datas.size();
            sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::allGameServer, &send, sizeof(send));
            i = 0;
            memset(&send, 0, sizeof(send));
        }
    }
    if(i != 0 || datas.empty())
    {
        send.userId = userId;
        send.total = datas.size();
        sendtoClient(user->getCoffeenetId(), (MsgCode2)msg::TeamMsgType::allGameServer, &send, sizeof(send));
    }
}

const BaseTeamData* TeamManager::getData(uint64_t teamId)
{
    auto it = m_teams.find(teamId);
    if(it == m_teams.end())
        return nullptr;
    return &it->second;
}

const BaseUserData* TeamManager::getUserData(uint64_t userId)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
        return nullptr;
    return &it->second;
}

uint64_t TeamManager::getTeamId(uint64_t userId, GameType gameType)
{
    auto it = m_users.find(userId);
    if(it == m_users.end())
        return 0;
    auto itTeam = it->second.teams.find(gameType);
    if(itTeam != it->second.teams.end())
        return itTeam->second;
    return 0;
}

bool TeamManager::beLeader(uint64_t userId, uint64_t teamId)
{
    auto it = m_teams.find(teamId);
    if(it == m_teams.end())
        return false;
    if(it->second.leader == userId)
        return true;
    return false;
}

bool TeamManager::inTeam(uint64_t userId, uint64_t teamId)
{
    auto it = m_teams.find(teamId);
    if(it == m_teams.end())
        return false;
    auto itMem = it->second.allMems.find(userId);
    if(itMem == it->second.allMems.end())
        return false;
    return true;
}

uint32_t TeamManager::getTeamerNum(uint64_t teamId)
{
    auto it = m_teams.find(teamId);
    if(it == m_teams.end())
        return 0;
    return it->second.allMems.size();
}

void TeamManager::addUser(BaseUserData data)
{
    auto it = m_users.find(data.id);
    if(it == m_users.end())
        m_users[data.id] = data;
}

void TeamManager::sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize)
{
    MsgCode1 code1 = (MsgCode1)msg::MsgType::team;
    Game::me().sendtoClient(coffeenetId, code1, code2, data, dataSize);
}

}
