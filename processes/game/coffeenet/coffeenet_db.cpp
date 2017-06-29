#include "coffeenet_db.h"
#include "coffeenet_checker.h"

#include "component/logger.h"

#include <functional>

namespace game{

//放在这里是不想再开个文件，麻烦！而放在头文件的话被别的单元引入后会产生defined but not used警告，优化后是错误
static const char* COFF_SELECT_INFO = "call COFF_SELECT_INFO(\'{}\')";//查询网吧账户信息，准备登录
static const char* COFF_SIGNIN_INFO = "call COFF_SIGNIN_INFO(\'{}\',\'{}\',\'{}\',\'{}\',\'{}\',\'{}\',\'{}\',{},{},\'{}\',\'{}\')";//注册网吧，存储过程会验证是否已经存在

static const char* COFF_AREA_COFF = "call COFF_AREA_COFF({},\'{}\',\'{}\',\'{}\')";

static const char* COFF_AREA_PROVINCE = "call COFF_AREA_PROVINCE({})";
static const char* COFF_AREA_CITY = "call COFF_AREA_CITY({},\'{}\')";
static const char* COFF_AREA_AREA = "call COFF_AREA_AREA({},\'{}\')";

CoffeenetDb& CoffeenetDb::me()
{
    static CoffeenetDb me;
    return me;
}

void CoffeenetDb::selectCoffeenetInfo(std::string account)
{
    using namespace std::placeholders;

    std::string sql = water::component::format(COFF_SELECT_INFO, account);
    Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::selectCoffeenetInfoBack, this, _1));
    LOG_DEBUG("客户端登录验证, 发起数据库请求, sql={}", sql);
}

void CoffeenetDb::selectCoffeenetInfoBack(MYSQL_RES* res)
{
    LOG_DEBUG("客户端登录验证, 收到数据库回调");
    if(!res)
        return;

    MYSQL_ROW row = mysql_fetch_row(res);
    uint16_t vip = 0;
    if(!row[(int32_t)SelectCoffInfo::vip])
        vip = atoi(row[(int32_t)SelectCoffInfo::pcNum]);
    if(row)
    {
        ClientChecker::me().loginCheck(atoll(row[(int32_t)SelectCoffInfo::coffeenetId]), 
                                       row[(int32_t)SelectCoffInfo::account], 
                                       row[(int32_t)SelectCoffInfo::passwd],
                                       row[(int32_t)SelectCoffInfo::name],
                                       row[(int32_t)SelectCoffInfo::simpleName],
                                       row[(int32_t)SelectCoffInfo::ownerName],
                                       row[(int32_t)SelectCoffInfo::ownerTel],
                                       row[(int32_t)SelectCoffInfo::managerTel],
                                       atoi(row[(int32_t)SelectCoffInfo::competitivePlatporm]),
                                       atoi(row[(int32_t)SelectCoffInfo::pcNum]),
                                       row[(int32_t)SelectCoffInfo::competitiveClub],
                                       vip,
                                       row[(int32_t)SelectCoffInfo::instruction]);
    }
}

void CoffeenetDb::signInCoffeenetInfo(std::string account,
                         std::string passwd,
                         std::string name,
                         std::string simpleName,
                         std::string ownerName,
                         std::string ownerTel,
                         std::string managerTel,
                         const uint8_t competitivePlatporm,
                         const uint32_t pcNum,
                         std::string competitiveClub,
                         std::string instruction)
{
    using namespace std::placeholders;
    std::string sql = water::component::format(COFF_SIGNIN_INFO, 
                                               account, 
                                               passwd, 
                                               name, 
                                               simpleName, 
                                               ownerName, 
                                               ownerTel,
                                               managerTel,
                                               competitivePlatporm,
                                               pcNum,
                                               competitiveClub,
                                               instruction);//注意uint8_t c++中不是unsigned char, 所有这里不会把competitivePlatporm当做一个字符，不会导致sql语句本该是8位数值变为一个字符符号，导致错误
    LOG_DEBUG("客户端注册验证, 发起数据库请求, sql={}", sql);
    Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::signInCoffeenetInfoBack, this, _1));
}

void CoffeenetDb::signInCoffeenetInfoBack(MYSQL_RES* res)
{
    LOG_DEBUG("客户端注册验证, 收到数据库回调");
    if(!res)
        return;

    MYSQL_ROW row = mysql_fetch_row(res);
    if(row)
    {
        ClientChecker::me().signInCheck(row[0], *(uint8_t*)row[1]);
    }
    else
    {
        LOG_ERROR("客户端注册验证, 出现同步注册同一账号，客户端收不到返回信息");
    }
}

void CoffeenetDb::areaCoffeenet(uint64_t userId, std::string province,
                               std::string city,
                               std::string area)
{
   using namespace std::placeholders;
   std::string sql = water::component::format(COFF_AREA_COFF, userId, province, city, area); 
   Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::areaCoffeenetBack, this, _1));
   LOG_DEBUG("区域网吧查询, 发起数据库请求, sql={}", sql);
}

void CoffeenetDb::areaCoffeenetBack(MYSQL_RES* res)
{
    LOG_DEBUG("区域网吧查询, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<CoffeenetData> vec;
    uint64_t userId = 0;
    while((row = mysql_fetch_row(res)))
    {
        userId = atoll(row[0]);
        CoffeenetData data;
        data.coffeenetId = atoll(row[1]);
        data.name = row[2];
        vec.push_back(data);
    }
    CoffeenetManager::me().areaCoffeenetBack(userId, vec);
}

void CoffeenetDb::province(uint64_t userId)
{
   using namespace std::placeholders;
   std::string sql = water::component::format(COFF_AREA_PROVINCE, userId); 
   Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::provinceBACK, this, _1));
   LOG_DEBUG("区域查询, 发起数据库请求, sql={}", sql);
}

void CoffeenetDb::provinceBACK(MYSQL_RES* res)
{
    LOG_DEBUG("区域查询, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<AreaData> vec;
    uint64_t userId = 0;
    while((row = mysql_fetch_row(res)))
    {
        userId = atoll(row[0]);
        AreaData data;
        data.code = row[1];
        data.name = row[2];
        vec.push_back(data);
    }
    CoffeenetManager::me().provinceBack(userId, vec);
}

void CoffeenetDb::city(uint64_t userId, std::string code)
{
   using namespace std::placeholders;
   std::string sql = water::component::format(COFF_AREA_CITY, userId, code); 
   Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::cityBACK, this, _1));
   LOG_DEBUG("区域查询, 发起数据库请求, sql={}", sql);
}

void CoffeenetDb::cityBACK(MYSQL_RES* res)
{
    LOG_DEBUG("区域查询, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<AreaData> vec;
    uint64_t userId = 0;
    while((row = mysql_fetch_row(res)))
    {
        userId = atoll(row[0]);
        AreaData data;
        data.code = row[1];
        data.name = row[2];
        vec.push_back(data);
    }
    CoffeenetManager::me().cityBack(userId, vec);
}

void CoffeenetDb::area(uint64_t userId, std::string code)
{
   using namespace std::placeholders;
   std::string sql = water::component::format(COFF_AREA_AREA, userId, code); 
   Game::me().execDbReadSql(sql, std::bind(&CoffeenetDb::areaBACK, this, _1));
   LOG_DEBUG("区域查询, 发起数据库请求, sql={}", sql);
}

void CoffeenetDb::areaBACK(MYSQL_RES* res)
{
    LOG_DEBUG("区域查询, 收到数据库回调");
    if(!res)
        return;
    MYSQL_ROW row;
    std::vector<AreaData> vec;
    uint64_t userId = 0;
    while((row = mysql_fetch_row(res)))
    {
        userId = atoll(row[0]);
        AreaData data;
        data.code = row[1];
        data.name = row[2];
        vec.push_back(data);
    }
    CoffeenetManager::me().areaBack(userId, vec);
}
}
