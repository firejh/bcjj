#ifndef GAME_COFFEENET_H
#define GAME_COFFEENET_H

#include "def.h"

#include "../user/user.h"

#include "component/class_helper.h"
#include "process/rawmsg_manger.h"

#include <map>

namespace game{

class Coffeenet
{
public:
    TYPEDEF_PTR(Coffeenet)
    CREATE_FUN_NEW(Coffeenet)
    Coffeenet(CoffeenetId coffeenetId, 
              const char* account,
              const char* name, 
              const char* simpleName,
              const char* ownerName,
              const char* ownerTel,
              const char* managerTel, 
              const uint8_t competitivePlatporm,
              const uint32_t pcNum, 
              const char* competitiveClub,
              const uint8_t vip,
              const char* instruction);
    ~Coffeenet(){}

    //上线
    void online();

    //下线
    void offline();

    //基础信息获取
    CoffeenetId getId();
    std::string getAccount();
    const std::string& getProvince() const;
    const std::string& getCity() const;

    //被动添加与删除角色
    void addUser(User::Ptr user);
    void delUser(uint64_t userId);

    //消息发送
    void sendtoMe(water::process::MsgCode1 code1, water::process::MsgCode2 code2, const void* data, uint32_t dataSize);

private:
    //网吧基础信息
    CoffeenetId m_id;
    std::string m_account;
    std::string m_name;
    std::string m_simpleName;                                                                               
    std::string m_ownerName;                                                                                
    std::string m_ownerTel;
    std::string m_managerTel;
    const uint8_t m_competitivePlatporm;
    const uint32_t m_pcNum;
    std::string m_competitiveClub;
    const uint8_t m_vip;
    std::string m_instruction;
    std::string m_province;
    std::string m_city;

    //角色
    std::map<uint64_t, User::Ptr> m_users;

};

}
#endif
