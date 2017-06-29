#include "user_db.h"
#include "user_manager.h"

#include "component/logger.h"


namespace game{

//用户
static const char* USER_SIGNIN_TEL_INFO = "call USER_SIGNIN_INFO(\'{}\',\'{}\',\'{}\',\'{}\',\'{}\')";
static const char* USER_TEL_LOGIN = "call USER_TEL_LOGIN(\'{}\')";

static const char* USER_SELECT_IDENTIFY = "call USER_SELECT_IDENTIFY({})";
static const char* USER_SELECT_GAME = "call USER_SELECT_GAME({})";

//实名验证
static const char* USER_IDENTIFY_DATA = "call USER_IDENTIFY_DATA({},\'{}\',\'{}\',\'{}\',\'{}\')";
static const char* USER_GAMEINFO_UPDATE = "call USER_GAMEINFO_UPDATE({},{},{},\'{}\')";

UserDb& UserDb::me()
{
    static UserDb me;
    return me;
}

void UserDb::signInTelUserInfo(std::string userName,
                        std::string userAddress,
                        std::string passwd,
                        std::string tel,
                        std::string qq,
                        std::string pic)
{
    using namespace std::placeholders;

    std::string sql = water::component::format(USER_SIGNIN_TEL_INFO, userName, passwd, qq, tel, pic);
    Game::me().execDbReadSql(sql, std::bind(&UserDb::signInTelUserInfoBack, this, _1));
    LOG_DEBUG("用户注册, 发起数据库请求, sql={}", sql);
}

void UserDb::signInTelUserInfoBack(MYSQL_RES* res)
{
    LOG_DEBUG("用户注册, 收到数据库回调");
    if(!res)
        return;

    MYSQL_ROW row = mysql_fetch_row(res);
    if(row)
    {
        UserManager::me().signInTelCheck(row[(uint32_t)SignInTelUserInfo::qq],
                                         atoi(row[(uint32_t)SignInTelUserInfo::result]));
    }
    
}

void UserDb::loginTelUser(const std::string tel)
{
    using namespace std::placeholders;

    std::string sql = water::component::format(USER_TEL_LOGIN, tel);
    Game::me().execDbReadSql(sql, std::bind(&UserDb::loginTelUserBack, this, _1));
    LOG_DEBUG("用户登录, 发起数据库请求, sql={}", sql);
}

void UserDb::loginTelUserBack(MYSQL_RES* res)
{
    LOG_DEBUG("用户登录, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row = mysql_fetch_row(res);
    if(row)
    {
        UserManager::me().loginTelCheck(atoll(row[(uint32_t)SelectUserInfo::userId]),
                                        row[(uint32_t)SelectUserInfo::userName],
                                        row[(uint32_t)SelectUserInfo::userAddress],
                                        row[(uint32_t)SelectUserInfo::passwd],
                                        row[(uint32_t)SelectUserInfo::qq],
                                        row[(uint32_t)SelectUserInfo::tel]);
    }
}

/*
void UserDb::setTeamId(const uint64_t userId, uint32_t gameType, uint64_t teamId)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(USER_SET_TEAMID, userId, gameType, teamId);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("用户战队设置, 发起数据库请求, sql={}", sql);
}
*/

void UserDb::selectIdentify(const uint64_t userId)
{
    std::string sql = water::component::format(USER_SELECT_IDENTIFY, userId);
    Game::me().execDbReadSql(sql, std::bind(&UserDb::selectIdentifyBack, this, _1)); 
    LOG_DEBUG("用户身份认证初始化, 发起数据库请求, sql={}", sql);
}

void UserDb::selectIdentifyBack(MYSQL_RES* res)
{
    LOG_DEBUG("用户身份认证初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row = mysql_fetch_row(res);
    if(row)
    {
        uint64_t userId = atoll(row[(uint32_t)SelectIdentifyDb::userId]);
        User::Ptr user = UserManager::me().getUser(userId);
        if(!user)
        {
            LOG_ERROR("用户身份认证初始化, 找不到对应的用户, userId={}", userId);
            return;
        } 
        user->initUserIdentifyCheck(row[(uint32_t)SelectIdentifyDb::identify],
                                    row[(uint32_t)SelectIdentifyDb::name],
                                    row[(uint32_t)SelectIdentifyDb::address],
                                    row[(uint32_t)SelectIdentifyDb::tel]);
        user->selectRet();
    }
    else
    {
        LOG_DEBUG("用户身份认证初始化, 未认证");
    }

    
}

void UserDb::selectUserGame(const uint64_t userId)
{
    std::string sql = water::component::format(USER_SELECT_GAME, userId);
    Game::me().execDbReadSql(sql, std::bind(&UserDb::selectUserGameBack, this, _1));
    LOG_DEBUG("用户游戏数据初始化, 发起数据库请求, sql={}", sql);
}

void UserDb::selectUserGameBack(MYSQL_RES* res)
{
    LOG_DEBUG("用户游戏数据初始化, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    User::Ptr user = nullptr;
    while((row = mysql_fetch_row(res)))
    {
        uint64_t userId = atoll(row[(uint32_t)SelectUserGameDb::userId]);
        if(!user)
        {
            user = UserManager::me().getUser(userId);
        }
        user->initUserGameCheck(atoi(row[(uint32_t)SelectUserGameDb::game_type]),
                                atoi(row[(uint32_t)SelectUserGameDb::game_server]),
                                row[(uint32_t)SelectUserGameDb::game_name]);
    }
    if(user)
        user->selectRet();
    
}

void UserDb::identifyData(uint64_t userId, 
              const std::string& identify, 
              const std::string& name, 
              const std::string& address, 
              const std::string& tel)
{
    std::string sql = water::component::format(USER_IDENTIFY_DATA, userId, identify, name, address, tel);
    Game::me().execDbReadSql(sql, std::bind(&UserDb::identifyDataBack, this, _1));
    LOG_DEBUG("用户实名认证, 发起数据库请求, sql=", sql);
}

void UserDb::identifyDataBack(MYSQL_RES* res)
{
    LOG_DEBUG("用户实名认证收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row = mysql_fetch_row(res);
    if(row)
    {
        uint64_t userId = atoll(row[(uint32_t)IdentifyDataDb::userId]);
        UserManager::me().identifyDataCheck(userId,
                                            row[(uint32_t)IdentifyDataDb::identify],
                                            row[(uint32_t)IdentifyDataDb::name],
                                            row[(uint32_t)IdentifyDataDb::address],
                                            row[(uint32_t)IdentifyDataDb::tel]);
    }

}

void UserDb::updateGameInfo(uint64_t userId, uint16_t gameType, uint16_t gameServer, std::string& name)
{
    std::string sql = water::component::format(USER_GAMEINFO_UPDATE, userId, gameType, gameServer, name);
    Game::me().execDbWriteSql(sql);
    LOG_DEBUG("户更新游戏信息, 发起数据库请求, sql={}", sql);
}

}
