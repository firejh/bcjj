#ifndef COFFEENET_DB_H
#define COFFEENET_DB_H

#include "coffeenet_checker.h"
#include "../game.h"

#include "process/db_connection.h"

#include <string>

namespace game{
enum class SelectCoffInfo : int32_t
{
    coffeenetId = 0,
    account = 1,
    passwd = 2,
    name = 3,
    simpleName = 4,
    ownerName = 5,
    ownerTel = 6,
    managerTel = 7,
    competitivePlatporm = 8,
    pcNum = 9,
    competitiveClub = 10,
    vip = 11,
    instruction = 12
};

class CoffeenetDb
{
public:
    static CoffeenetDb& me();

    void selectCoffeenetInfo(std::string account);
    void selectCoffeenetInfoBack(MYSQL_RES* res);

    void signInCoffeenetInfo(std::string account,
                             std::string passwd,
                             std::string name,
                             std::string simpleName,
                             std::string ownerName,
                             std::string ownerTel,
                             std::string managerTel,
                             const uint8_t competitivePlatporm,
                             const uint32_t pcNum,
                             std::string competitiveClub,
                             std::string instruction);
    void signInCoffeenetInfoBack(MYSQL_RES* res);

    //请求对应区域的所有网吧
    void areaCoffeenet(uint64_t userId, std::string province,
                       std::string city,
                       std::string area);
    void areaCoffeenetBack(MYSQL_RES* res);


    void province(uint64_t userId);
    void city(uint64_t userId, std::string code);
    void area(uint64_t userId, std::string code);
    void provinceBACK(MYSQL_RES* res);
    void cityBACK(MYSQL_RES* res);
    void areaBACK(MYSQL_RES* res);
};

}
#endif
