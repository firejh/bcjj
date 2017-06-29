#ifndef FIGHT_TEAM_H
#define FIGHT_TEAM_H

#include "common/def.h"
#include "fight_def.h"

#include "stdint.h"
#include <string>
#include <cstring>


static const uint32_t TEAMER_MAX_NUM = 20;

namespace game{

struct TeamerData
{
    uint64_t userId = 0;                //用户id
    uint16_t userStatus = 0;            //1：正式成员，2：待批准成员
    uint8_t online;
    std::string userName;               //用户名
    std::string tel;                    //用户昵称
    std::string gameName;               //游戏昵称
    //...
};

struct TeamData
{
    uint64_t teamId;                     //战队id
    std::string pic;                     //战队头像
    std::string name;                    //战队
    GameType gameType;                   //游戏类型
    uint16_t gameServer;                 //游戏类型对应的服id
    uint64_t coffeenetId;
    uint64_t leader;
    uint64_t viceLeader;
};

}
#endif
