#ifndef USER_DB_H
#define USER_DB_H

#include "../game.h"
#include "process/db_connection.h"

#include <string>

namespace game{
using namespace std::placeholders; 

enum class SelectUserInfo
{
    //用户基本数据
    userId = 0,
    userName = 1,
    userAddress = 2,
    passwd = 3,
    qq = 4,
    tel = 5,
};

enum class SignInTelUserInfo
{
    qq = 0,
    result = 1,
};

enum class SelectIdentifyDb
{
    userId = 0,
    identify = 1,
    name = 2,
    address = 3,
    tel = 4,
};

enum class SelectUserGameDb
{
    userId = 0,
    game_type = 1,
    game_server = 2,
    game_name = 3,
};

enum class IdentifyDataDb
{
    userId = 0,
    identify = 1,
    name = 2,
    address = 3,
    tel = 4,
};

class UserDb
{
public:
    static UserDb& me();

    void signInTelUserInfo(std::string userName,
                    std::string userAddress,
                    std::string passwd,
                    std::string tel,
                    std::string qq,
                    std::string pic);
    void signInTelUserInfoBack(MYSQL_RES* res);

    //登录查询
    void loginTelUser(const std::string tel);
    void loginTelUserBack(MYSQL_RES* res);

    //void setTeamId(const uint64_t userId, uint32_t gameType, uint64_t teamId);

    //初始化角色身份认证数据
    void selectIdentify(const uint64_t userId);
    void selectIdentifyBack(MYSQL_RES* res);

    //初始化角色游戏数据
    void selectUserGame(const uint64_t userId);
    void selectUserGameBack(MYSQL_RES* res);

    //实名认证
    void identifyData(uint64_t userId,
                      const std::string& identify,
                      const std::string& name,
                      const std::string& address,
                      const std::string& tel);
    void identifyDataBack(MYSQL_RES* res);

    void updateGameInfo(uint64_t userId, uint16_t gameType, uint16_t gameServer, std::string& name);

};

}

#endif
