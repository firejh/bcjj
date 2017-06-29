#ifndef FIGHT_TEAM_MANAGER_H
#define FIGHT_TEAM_MANAGER_H

//说明，manager是管理的意思，包括team相关操作

#include "team.h"
#include "fight_def.h"
#include "../coffeenet/def.h"

#include "process/rawmsg_manger.h"

#include <vector>
#include <set>
#include <map>
#include <string>

namespace game{

using namespace water;
using namespace process;

//游戏服数据
struct GameServerData
{
    uint16_t id;
    std::string name;
};

//游戏信息1
struct GameInfo
{
    uint16_t gameServer;
    std::string gameName;
};


/*所有的缓存，包括战队、角色、网吧的部分常用数据都在这里*/
//战队基础数据
struct BaseTeamData
{
    uint64_t id;
    std::string pic;
    std::string name;
    GameType gameType;
    uint16_t gameServer;
    uint64_t coffnetId;
    uint64_t leader;
    uint64_t viceLeader;
    std::set<uint64_t> allMems;
    std::set<uint64_t> waitMems;
};
//角色的基本信息，时间太紧，直接放到战队这里缓存角色基本信息
struct BaseUserData
{
    //基本
    uint64_t id;
    std::string pic;
    std::string tel;
    std::string name;

    //游戏信息
    std::map<GameType, GameInfo> gameInfo;
    //战队信息
    std::map<GameType, uint64_t> teams;
};
//网吧基础数据
struct CoffeenetBaseData
{
    uint32_t id;
    std::string name;
};

class TeamManager
{
public:
    static TeamManager& me();
    ~TeamManager(){};
    //消息注册
    void regMsgHandler();

    //服务器启动初始化
    void init();
    //插入战队
    void insertTeam(BaseTeamData& data);
    //插入角色缓存数据，实际上这部分应该在单独的一个缓存模块写，但是这里角色跟战队相互关联，为了方便就写在战队管理内了
    void insertMem(BaseUserData& data);
    //插入角色游戏信息
    void insertMemGame(uint64_t userId, GameType gameType, uint16_t gameServer, std::string gameName);
    //插入角色战队信息
    void insertMemTeam(uint64_t userId, uint64_t teamId, uint16_t status);
    //插入网吧基础信息
    void insertCoff(CoffeenetBaseData& data);

    //获取角色缓存数据
    const BaseUserData* getUserData(uint64_t userId);
    //添加角色缓存数据
    void addUser(BaseUserData data);

    //创建战队
    void createTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //获取战队信息
    void teamInfo(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //战队成员列表
    void TeamerList(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //申请加入战队c->s
    void JoinTeamToS(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //申请战队的处理结果
    void DealJoinTeamToS(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //网吧战队列表
    void coffTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //求对应类型的游戏服务器列表
    void allGameServer(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    void allGameServerCheck(uint64_t userId, std::vector<GameServerData>& datas);
    //请求搜索战队
    void findTeam(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);

    //获取战队缓存基础信息
    const BaseTeamData* getData(uint64_t teamId);
    //根据游戏类型获取战队id
    uint64_t getTeamId(uint64_t userId, GameType gameType);
    //获取战队成员数量
    uint32_t getTeamerNum(uint64_t teamId);
    //是否有某个游戏类型的战队
    bool haveTeam(uint64_t userId, GameType gameType);
    //是否是某个战队的队长
    bool beLeader(uint64_t userId, uint64_t teamId);
    //是否在战队内
    bool inTeam(uint64_t userId, uint64_t teamId);
    //获得角色的所有游戏类型的战队
    std::vector<uint64_t> getAllTeam(uint64_t userId);

private:
    TeamManager(){}
    //发送客户端信息
    void sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize);
    //是否有对应游戏服的游戏信息
    bool isGameServer(uint64_t userId, GameType gameType, uint16_t gameServer);
    //设置战队信息
    void setTeamId(uint64_t userId, GameType gameType, uint64_t teamId, uint16_t status);
    //产生一个战队id
    uint64_t createTeamId();

private:
    //所有的战队缓存
    std::map<uint64_t, BaseTeamData> m_teams;
    //所有的用户基础信息缓存
    std::map<uint64_t, BaseUserData> m_users;
    //网吧对应的战队
    std::map<uint32_t, std::set<uint64_t>> m_coffTeams;
    //网吧基础数据
    std::map<uint32_t, CoffeenetBaseData> m_coffs;
};
}
#endif
