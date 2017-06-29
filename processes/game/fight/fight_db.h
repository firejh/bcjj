#ifndef FIGHT_FIGHT_DB_H
#define FIGHT_FIGHT_DB_H

#include "process/db_connection.h"

#include <string>

namespace game{

enum class InitAllUserDb: uint32_t
{
    id = 0,
    pic,
    name,
    tel,
};

enum class InitAllUserGameDb: uint32_t
{
    id,
    gameType,
    gameServer,
    gameName,
};

enum class InitAllUserTeamDb: uint32_t
{
    id,
    teamId,
    status,
};

enum class InitTeamUserInfoDb : uint32_t
{
    userId = 0,
    teamId = 1,
    teamStaus = 2,
    gameType = 3,
    leader = 4,
    num = 5,
};

enum class TeamInfoDb : uint32_t
{
    id = 0 ,
    pic,
    name,
    gameType,
    gameServer,
    coffeenetId,
    leader,
    leaderVice,
    createTime,
};

enum class TeamerListDb : uint32_t
{
    myId = 0,

    id = 1,
    userName = 2,
    userStatus = 3,
};

enum class JoinTeamToSDb : uint32_t
{
    userId = 0,
    teamId = 1,
    teamerNum = 2,
    leader = 3,
    leaderVice = 4,
    gameType = 5,
    gameServer = 6,
};

enum class CoffTeamDb : uint32_t
{
    userId = 0,
    teamId = 1,
    picId = 2,
    name = 3,
    gameType = 4,
    gameServer = 5,
};

//赛事相关
enum class InitAreaFight : uint32_t
{
    id = 0,
    gameType,
    fightType,//赛事类型
    gameServer,
    fightRule1,//赛事赛制，单败、双败、循环等
    fightRule2,//赛事规则，一局还是3局，还是5局
    startApply,
    stopApply,
    startFight,
    stopFight,
    name,
    simple_name,
    pic,
    fromType,
    from,
    status,
    createTime,
    instruction,
    stop,
    stopFlag,

    nextNum,
    parentId,
    area,
    
};

enum class InitAllFightingObj : uint32_t
{
    id = 0,
    fightId,
    status,
    value,
    valueScore,
    position,
    competitor,
};

enum class InitAllFightingChall : uint32_t
{
    fightId,
    objId1,
    objId2,
    result1,
    result2,
    pic1,
    pic2,
    reference,
};

class FightDb
{
public:
    ~FightDb(){}
    static FightDb& me();

    /*
    //初始化最大战队id
    void initMaxTeamId();
    void initMaxTeamIdBack(MYSQL_RES* res);

    void initUserTeamInfo(const uint64_t userId);
    void initUserTeamInfoBack(MYSQL_RES* res);
*/

    //初始化所有战队
    void initAllTeam();
    void initAllTeamBack(MYSQL_RES* res);
    void initAllUser();
    void initAllUserBack(MYSQL_RES* res);
    void initAllUserTeam();
    void initAllUserTeamBack(MYSQL_RES* res);
    void initAllUserGame();
    void initAllUserGameBack(MYSQL_RES* res);

    void initAllCoff();
    void initAllCoffBack(MYSQL_RES* res);

    //创建战队数据库操作
    void updateTeam(const uint64_t teamId,
                    const std::string& pic,
                    const std::string& name,
                    const uint32_t gameTye,
                    const uint16_t gameServer,
                    const uint64_t coffeenetId,
                    const uint64_t leaderId,
                    const uint64_t viceLeaderId);
    void updateTeamMem(const uint64_t userId, const uint64_t teamId, const uint16_t status);

    void refuseInTeam(const uint64_t userId, const uint64_t teamId);

    /*
    //获取战队列表（网吧内的）
    void coffTeam(const uint64_t userId, const uint64_t coffeenetId);
    void coffTeamBack(MYSQL_RES* res);
*/
    //初始化在进行的比赛
    void initAreaFight();
    void initAreaFightBack(MYSQL_RES* res);

    //查询推荐赛事
    void initRecommendFighting();
    void initRecommendFightingBack(MYSQL_RES* res);

    //更新赛事状态
    void updateFightStatus(uint64_t fightId, uint16_t status);

    //初始新的赛事
    void initNewAreaFight(uint64_t lastFightId);
    void initNewAreaFightBack(MYSQL_RES* res);

    //无就插入，有更新赛事参与个体数据
    void updateFightObj(uint64_t objId, uint64_t fightId, uint16_t status, uint32_t value, uint32_t valueScore, uint32_t position, uint64_t competitor);
    
    //查询所有赛事中的个体
    void initFightingObj();
    void initFightingObjBack(MYSQL_RES* res);

    //初始化赛事中的战斗
    void initFightingChall();
    void initFightingChallBack(MYSQL_RES* res);

    //更新或插入单次战斗
    void updateChallData(const uint64_t fightId,
                         const uint64_t objId1,
                         const uint64_t objId2,
                         const uint64_t result1,
                         const uint64_t result2,
                         std::string pic1,
                         std::string pic2);
    void eraseChallData(const uint64_t fightId, const uint64_t objId1, const uint16_t count);

    //插入战斗记录
    void insertChallRecord();

    void selectGameServer(uint64_t userId, uint16_t gameType);
    void selectGameServerBack(MYSQL_RES* res, void* data);
private:
    FightDb(){}
};

}

#endif
