#include "fight_db.h"
#include "team.h"

#include "component/logger.h"
#include "team_manager.h"
#include "../game.h"
#include "../user/user_manager.h"
#include "fight_manager.h"

#include <vector>

namespace game{

//加载所有战队
static const char* FIGHT_TEAM_INIT_ALL = "call FIGHT_TEAM_INIT_ALL()";
static const char* FIGHT_USER_INIT_ALL = "call FIGHT_USER_INIT_ALL()";
static const char* FIGHT_USER_INIT_ALL_GAME_INFO = "call FIGHT_USER_INIT_ALL_GAME_INFO()";
static const char* FIGHT_USER_INIT_ALL_TEAM = "call FIGHT_USER_INIT_ALL_TEAM()";
static const char* FIGHT_COFF_INIT_ALL = "call FIGHT_COFF_INIT_ALL()";

//战队更新
static const char* FIGHT_UPDATE_TEAM = "call FIGHT_TEAM_UPDATE({},\'{}\',\'{}\',{},{},{},{},{})";
//战队成员更新
static const char* FIGHT_UPDATE_TEAMER = "call FIGHT_TEAMER_UPDATE({},{},{})";

//拒绝加入战队
static const char* FIGHT_TEAM_REFUSE_JOININ = "call FIGHT_TEAM_REFUSE_JOININ({},{})";
//请求网吧战队列表
//static const char* FIGHT_TEAM_COFF_TEAM = "call FIGHT_TEAM_COFF_TEAM({},{})";


//初始化进行的赛事
//区域赛
static const char* FIGHT_INIT_AREA_FIGHT = "call FIGHT_INIT_AREA_FIGHT()";
static const char* FIGHT_INIT_NEW_AREA_FIGHT = "call FIGHT_INIT_NEW_AREA_FIGHT({})";

//推荐的赛事
static const char* FIGHT_INIT_RECOOMOND_FIGHTING = "call FIGHT_INIT_RECOOMOND_FIGHTING()";

//赛事状态更新
static const char* FIGHT_UPDATE_STATUS = "call FIGHT_UPDATE_STATUS({},{})";

//更新赛事参与个体数据
static const char* FIGHT_UPDATE_FIGHT_OBJ = "call FIGHT_UPDATE_FIGHT_OBJ({},{},{},{},{},{},{})";
static const char* FIGHT_INIT_FIGHTINGOBJ = "call FIGHT_INIT_FIGHTINGOBJ()";

//更新战斗数据
static const char* FIGHT_UPDATE_FIGHT_CHALL = "call FIGHT_UPDATE_FIGHT_CHALL({},{},{},{},{},\'{}\',\'{}\')";
static const char* FIGHT_ERASE_FIGHT_CHALL = "call FIGHT_ERASE_FIGHT_CHALL({},{})";
static const char* FIGHT_INIT_FIGHTINGCHALL = "call FIGHT_INIT_FIGHTINGCHALL()";

static const char* FIGHT_TEAM_ALL_GAME_SERVER = "call FIGHT_TEAM_ALL_GAME_SERVER({})";

FightDb& FightDb::me()
{
    static FightDb me;
    return me;
}

void FightDb::initAllTeam()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_TEAM_INIT_ALL);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAllTeamBack, this, _1));
    //LOG_DEBUG("$$$$$$$$$$$$$$$$$$战队初始化, 发起数据库请求, sql={}", sql);
}

void FightDb::initAllTeamBack(MYSQL_RES* res)
{
    //LOG_DEBUG("&&&&&&&&&&&&&&&&&战队初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        BaseTeamData data;
        data.id = atoll(row[(uint32_t)TeamInfoDb::id]);
        data.pic = row[(uint32_t)TeamInfoDb::pic];
        data.name = row[(uint32_t)TeamInfoDb::name];
        data.gameType = (GameType)atoi(row[(uint32_t)TeamInfoDb::gameType]);
        data.gameServer = (uint16_t)atoi(row[(uint32_t)TeamInfoDb::gameServer]);
        data.coffnetId = atoi(row[(uint32_t)TeamInfoDb::coffeenetId]);
        data.leader = atoll(row[(uint32_t)TeamInfoDb::leader]);
        data.viceLeader = atoll(row[(uint32_t)TeamInfoDb::leaderVice]);
        TeamManager::me().insertTeam(data);
    }

    initAllUser();
}

void FightDb::initAllCoff()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_COFF_INIT_ALL);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAllCoffBack, this, _1));
    //LOG_DEBUG("网吧初始化, 发起数据库请求, sql={}", sql);
}

void FightDb::initAllCoffBack(MYSQL_RES* res)
{
    LOG_DEBUG("网吧初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        CoffeenetBaseData data;
        data.id = atoi(row[(uint32_t)0]);
        data.name = row[1];
        TeamManager::me().insertCoff(data);
    }
}

void FightDb::initAllUser()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_USER_INIT_ALL);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAllUserBack, this, _1));
    //LOG_DEBUG("角色初始化, 发起数据库请求, sql={}", sql);
}

void FightDb::initAllUserBack(MYSQL_RES* res)
{
    LOG_DEBUG("角色初始化, 收到数据库回调");
    if(!res)
        return;
    //auto size = mysql_num_fields(res);
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        BaseUserData data;
        data.id = atoll(row[(uint32_t)InitAllUserDb::id]);
        data.name = row[(uint32_t)InitAllUserDb::name];
        data.pic = row[(uint32_t)InitAllUserDb::pic];
        data.tel = row[(uint32_t)InitAllUserDb::tel];
        TeamManager::me().insertMem(data);
    }
    initAllUserGame(); 
}

void FightDb::initAllUserGame()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_USER_INIT_ALL_GAME_INFO);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAllUserGameBack, this, _1));
    LOG_DEBUG("角色游戏信息初始化, 发起数据库请求, sql={}", sql);
}

void FightDb::initAllUserGameBack(MYSQL_RES* res)
{
    LOG_DEBUG("角色初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        TeamManager::me().insertMemGame(atoll(row[(uint32_t)InitAllUserGameDb::id]), (GameType)atoll(row[(uint32_t)InitAllUserGameDb::gameType]), atoll(row[(uint32_t)InitAllUserGameDb::gameServer]), row[(uint32_t)InitAllUserGameDb::gameName]);
    }
    initAllUserTeam(); 
}

void FightDb::initAllUserTeam()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_USER_INIT_ALL_TEAM);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAllUserTeamBack, this, _1));
    LOG_DEBUG("角色战队信息初始化, 发起数据库请求, sql={}", sql);
}

void FightDb::initAllUserTeamBack(MYSQL_RES* res)
{
    LOG_DEBUG("角色战队信息初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        TeamManager::me().insertMemTeam(atoll(row[(uint32_t)InitAllUserTeamDb::id]), atoll(row[(uint32_t)InitAllUserTeamDb::teamId]), atoll(row[(uint32_t)InitAllUserTeamDb::status]));
    }
}

void FightDb::updateTeam(const uint64_t teamId,
                         const std::string& pic,
                         const std::string& name,
                         const uint32_t gameTye,
                         const uint16_t gameServer,
                         const uint64_t coffeenetId,
                         const uint64_t leaderId,
                         const uint64_t viceLeaderId)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_UPDATE_TEAM, teamId, pic, name, gameTye, gameServer, coffeenetId, leaderId, viceLeaderId);
    Game::me().execDbReadSql(sql, nullptr);
    LOG_DEBUG("更新战队, 发起数据库请求, sql={}, sql");
}

void FightDb::updateTeamMem(const uint64_t userId, const uint64_t teamId, const uint16_t status)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_UPDATE_TEAMER, userId, teamId, status);
    Game::me().execDbReadSql(sql, nullptr);
    LOG_DEBUG("更新战队成员, 发起数据库请求, sql={}, sql");
}

void FightDb::refuseInTeam(const uint64_t userId, const uint64_t teamId)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_TEAM_REFUSE_JOININ, userId, teamId);
    Game::me().execDbReadSql(sql, nullptr);
    LOG_DEBUG("战队申请处理, 拒绝, 发起数据库请求");
};

void FightDb::initAreaFight()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_INIT_AREA_FIGHT);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initAreaFightBack, this, _1));
    LOG_DEBUG("初始化区域赛事, 发起数据库请求, sql={}", sql);
}

void FightDb::initAreaFightBack(MYSQL_RES* res)
{
    LOG_DEBUG("收到区域赛事数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        FightData data;
        data.m_id = atoll(row[(uint32_t)InitAreaFight::id]);
        data.m_gameType = (GameType)atoi(row[(uint32_t)InitAreaFight::gameType]);
        data.m_fightType = (FightType)atoi(row[(uint32_t)InitAreaFight::fightType]);
        data.m_gameServer = atoi(row[(uint32_t)InitAreaFight::gameServer]);
        data.m_rule1 = (FightRule1)atoi(row[(uint32_t)InitAreaFight::fightRule1]);
        data.m_rule2 = (FightRule2)atoi(row[(uint32_t)InitAreaFight::fightRule2]);
        data.m_startApplyTime = atoll(row[(uint32_t)InitAreaFight::startApply]);
        data.m_stopApplyTime = atoll(row[(uint32_t)InitAreaFight::stopApply]);
        data.m_startFightTime = atoll(row[(uint32_t)InitAreaFight::startFight]);
        data.m_stopFightTime = atoll(row[(uint32_t)InitAreaFight::stopFight]);
        data.m_name = row[(uint32_t)InitAreaFight::name];
        data.m_pic = row[(uint32_t)InitAreaFight::pic];
        data.m_userTye = (UserType)atoi((row[(uint32_t)InitAreaFight::fromType]));
        data.m_from = atoi(row[(uint32_t)InitAreaFight::from]);
        data.m_status = (FightStatus)atoi(row[(uint32_t)InitAreaFight::status]);
        data.m_createTime = atoll(row[(uint32_t)InitAreaFight::createTime]);
        data.m_instruction = row[(uint32_t)InitAreaFight::instruction];
        data.m_stop = atoi(row[(uint32_t)InitAreaFight::stop]);
        data.m_stopFlag = atoi(row[(uint32_t)InitAreaFight::stopFlag]);

        FightManager::me().initAreaFightCheck(data, 
                                              atoi(row[(uint32_t)InitAreaFight::nextNum]),
                                              atoll(row[(uint32_t)InitAreaFight::parentId]), 
                                              row[(uint32_t)InitAreaFight::area]);
    }

    initFightingObj();
}

void FightDb::initRecommendFighting()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_INIT_RECOOMOND_FIGHTING);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initRecommendFightingBack, this, _1));
    LOG_DEBUG("更新推荐赛事, 发起数据库请求, sql={}", sql);
}

void FightDb::initRecommendFightingBack(MYSQL_RES* res)
{
    LOG_DEBUG("更新推荐赛事, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<uint64_t> temp;
    while((row = mysql_fetch_row(res)))
    {
        uint64_t id = atoll(row[0]);
        if(id != 0)
            temp.push_back(id);
    }
    FightManager::me().initRecommendFightingCheck(temp);
}

void FightDb::initNewAreaFight(uint64_t lastFightId)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_INIT_NEW_AREA_FIGHT, lastFightId);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initNewAreaFightBack, this, _1));
}

void FightDb::initNewAreaFightBack(MYSQL_RES* res)
{
    LOG_DEBUG("初始化新的的赛事, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        FightData data;
        data.m_id = atoll(row[(uint32_t)InitAreaFight::id]);
        data.m_gameType = (GameType)atoi(row[(uint32_t)InitAreaFight::gameType]);
        data.m_fightType = (FightType)atoi(row[(uint32_t)InitAreaFight::fightType]);
        data.m_gameServer = atoi(row[(uint32_t)InitAreaFight::gameServer]);
        data.m_rule1 = (FightRule1)atoi(row[(uint32_t)InitAreaFight::fightRule1]);
        data.m_rule2 = (FightRule2)atoi(row[(uint32_t)InitAreaFight::fightRule2]);
        data.m_startApplyTime = atoll(row[(uint32_t)InitAreaFight::startApply]);
        data.m_stopApplyTime = atoll(row[(uint32_t)InitAreaFight::stopApply]);
        data.m_startFightTime = atoll(row[(uint32_t)InitAreaFight::startFight]);
        data.m_stopFightTime = atoll(row[(uint32_t)InitAreaFight::stopFight]);
        data.m_name = row[(uint32_t)InitAreaFight::name];
        data.m_pic = row[(uint32_t)InitAreaFight::pic];
        data.m_userTye = (UserType)atoi((row[(uint32_t)InitAreaFight::fromType]));
        data.m_from = atoi(row[(uint32_t)InitAreaFight::from]);
        data.m_status = (FightStatus)atoi(row[(uint32_t)InitAreaFight::status]);
        data.m_createTime = atoll(row[(uint32_t)InitAreaFight::createTime]);
        data.m_instruction = row[(uint32_t)InitAreaFight::instruction];
        data.m_stop = atoi(row[(uint32_t)InitAreaFight::stop]);
        data.m_stopFlag = atoi(row[(uint32_t)InitAreaFight::stopFlag]);

        FightManager::me().initAreaFightCheck(data, 
                                              atoi(row[(uint32_t)InitAreaFight::nextNum]),
                                              atoll(row[(uint32_t)InitAreaFight::parentId]), 
                                              row[(uint32_t)InitAreaFight::area]);

    };

}

void FightDb::updateFightObj(uint64_t objId, uint64_t fightId, uint16_t status, uint32_t value, uint32_t valueScore, uint32_t position, uint64_t competitor)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_UPDATE_FIGHT_OBJ, objId, fightId, status, value, valueScore, position, competitor);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("更新赛事个体, 发起数据库请求, sql={}", sql);
}

void FightDb::initFightingObj()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_INIT_FIGHTINGOBJ);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initFightingObjBack, this, _1));
    LOG_DEBUG("初始化在参赛的个体, 发起数据库请求, sql={}", sql);
}

void FightDb::initFightingObjBack(MYSQL_RES* res)
{
    LOG_DEBUG("始化在参赛的个体, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        FightObjData data;
        data.id = atoll(row[(uint32_t)InitAllFightingObj::id]);
        data.fightId = atoll(row[(uint32_t)InitAllFightingObj::fightId]); 
        data.status = atoi(row[(uint32_t)InitAllFightingObj::status]);
        data.value = atoi(row[(uint32_t)InitAllFightingObj::value]);
        data.valueScore = atoi(row[(uint32_t)InitAllFightingObj::valueScore]);
        data.position = atoi(row[(uint32_t)InitAllFightingObj::position]);
        data.competitor = atoll(row[(uint32_t)InitAllFightingObj::competitor]); 

        FightManager::me().initFightObj(data);
    }
    initFightingChall();
}

void FightDb::initFightingChall()
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_INIT_FIGHTINGCHALL);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::initFightingChallBack, this, _1));
    LOG_DEBUG("初始化在参赛的战斗, 发起数据库请求, sql={}", sql);
}

void FightDb::initFightingChallBack(MYSQL_RES* res)
{
    LOG_DEBUG("始化在参赛的战斗, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res)))
    {
        ChallengeData data;
        uint64_t fightId = atoll(row[(uint32_t)InitAllFightingChall::fightId]);
        data.objId1 = atoll(row[(uint32_t)InitAllFightingChall::objId1]);
        data.objId2 = atoll(row[(uint32_t)InitAllFightingChall::objId2]);
        data.result1 = atoi(row[(uint32_t)InitAllFightingChall::result1]);
        data.result2 = atoi(row[(uint32_t)InitAllFightingChall::result2]);
        data.pic1 = atoi(row[(uint32_t)InitAllFightingChall::pic1]);
        data.pic2 = atoi(row[(uint32_t)InitAllFightingChall::pic2]);
        data.reference = atoi(row[(uint32_t)InitAllFightingChall::reference]);
        FightManager::me().initFightChall(fightId, data);
    }

    FightManager::me().initOver();
}

void FightDb::updateChallData(const uint64_t fightId,
                     const uint64_t objId1,
                     const uint64_t objId2,
                     const uint64_t result1,
                     const uint64_t result2,
                     std::string pic1,
                     std::string pic2)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_UPDATE_FIGHT_CHALL, fightId, objId1, objId2, result1, result2, pic1, pic2);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("更新战斗结果, 发起数据库请求, sql={}", sql);
}

void FightDb::updateFightStatus(uint64_t fightId, uint16_t status)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_UPDATE_STATUS, fightId, status);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("更新赛事状态, 发起数据库请求, sql={}", sql);
}

void FightDb::eraseChallData(const uint64_t fightId, const uint64_t objId1, const uint16_t count)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_ERASE_FIGHT_CHALL, fightId, objId1);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("删除战斗结果, 发起数据库请求, sql={}", sql);
}

void FightDb::insertChallRecord()
{
    LOG_DEBUG("插入战斗记录");
}

void FightDb::selectGameServer(uint64_t userId, uint16_t gameType)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(FIGHT_TEAM_ALL_GAME_SERVER, userId, gameType);
    Game::me().execDbReadSql(sql, std::bind(&FightDb::selectGameServerBack, this, _1, _2), userId);
    LOG_DEBUG("请求对应游戏服务器列表, 发起数据库请求, sql={}", sql);
}

void FightDb::selectGameServerBack(MYSQL_RES* res, void* data)
{
    uint64_t userId = *(uint64_t*)data;
    LOG_DEBUG("请求对应游戏服务器列表, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<GameServerData> temp;
    while((row = mysql_fetch_row(res)))
    {
        GameServerData data;
        data.id = atoi(row[0]);
        data.name = row[1];
        temp.push_back(data);
    }
    TeamManager::me().allGameServerCheck(userId, temp);

}

}
