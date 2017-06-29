#ifndef GAME_COFFEENET_MANAGER_H
#define GAME_COFFEENET_MANAGER_H

#include "coffeenet.h"

#include "process/rawmsg_manger.h"
#include "component/spinlock.h"

#include <unordered_map>
#include <list>
#include <vector>

namespace game{

struct AreaData
{
    std::string code;
    std::string name;
};

using namespace water;
using namespace process;
class CoffeenetManager
{
public:
    static CoffeenetManager& me();
    void regMsgHandler();

    Coffeenet::Ptr getCoffeenet(CoffeenetId coffeenetId);

    //上线，登录成功后调用
    void online(Coffeenet::Ptr coffeenet);
    void offline(CoffeenetId coffeenetId);
    void offlineIfExist(CoffeenetId coffeenetId);

    //连接异常调用，不会再处理连接
    void delCoffeenet(CoffeenetId coffeenetId);

    //请求对应区域的所有网吧
    void areaCoffeenet(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void areaCoffeenetBack(const uint64_t userId, std::vector<CoffeenetData>& coffDatas);

    void province(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void city(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void area(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
    void provinceBack(uint64_t userId, std::vector<AreaData>& data);
    void cityBack(uint64_t userId, std::vector<AreaData>& data);
    void areaBack(uint64_t userId, std::vector<AreaData>& data);

    void sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize);


//msg
public:
    void logoff(const uint8_t* msgData, uint32_t msgSize, CoffeenetId coffeenetId);
private:
    CoffeenetManager(){}
    ~CoffeenetManager(){}

private:

    component::Spinlock m_delconnectCoffenetsLock;//
    std::list<CoffeenetId> m_delconnectCoffenets;//连接关闭的放入此容器由本线程做删除处理，否则异步操作代价太大
    std::unordered_map<CoffeenetId, Coffeenet::Ptr> m_coffeenets;//所有网吧
};
}
#endif
