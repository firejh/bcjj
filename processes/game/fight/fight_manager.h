/*
 * Author: JiangHeng
 *
 * Created: 2016-11-30 16:24 +0800
 *
 * Modified: 2016-11-30 16:24 +0800
 *
 * Description:所有的赛事，不论类型，统一管理，因为比赛的接口基本统一
 */
#ifndef FIGHT_MOUDEL_H
#define FIGHT_MOUDEL_H

#include "fight.h"
#include "area_fight.h"

#include "../coffeenet/def.h"
#include "common/def.h"
#include "process/rawmsg_manger.h"

#include <map>
#include <set>

namespace game{

class FightManager
{
public:
    static FightManager& me();
    ~FightManager(){}

    //赛事的启动服务器初始化
    void init();
    //初始化结束
    void initOver();
    // 消息注册
    void regMsgHandler();
    //赛事的控制，循环实现
    void contrl();

    //根据赛事id获得赛事对象指针
    Fight::Ptr getFight(uint64_t fightId);
    //从数据库初始化赛事
    void initFightObj(FightObjData& objs);
    //从数据库初始化赛事的当前的战斗
    void initFightChall(uint64_t fightId, ChallengeData& challs);

    //加载推荐赛事
    void initRecommendFighting();
    void initRecommendFightingCheck(std::vector<uint64_t>& recommonFighting);
    //加载在战斗的赛事
    void initAreaFight();
    void initAreaFightCheck(FightData& data, uint32_t nextNum, uint64_t parentId, std::string area);
    //找出新的即id大于内存中最大的，这里是网站新发布的赛事，暂时没有给服务器发消息，服务器从库内定时检测是否有新赛事
    void initNewFighting();

    //msg，正常的通过战队参与比赛
    //首页推荐赛事，首页是后台发布的推荐的赛事
    void recommondFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //根据推荐找到对应的网吧赛事
    void coffFightByrRecommond(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //请求赛事成员列表
    void fightMemData(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //自己网吧的对应游戏的赛事列表
    void myCoffFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //报名
    void signUp(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //自己的赛事，报名的未结束的都显示
    void myFight(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //进入比赛选人，即请求自己战队的人
    //teamerlist
    //队长选人
    void ChallengerMem(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //void ChallengerMem(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //队员准备
    void ready(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);
    //比赛结果，结果提交了之后，不冲突直接生成比赛记录,进入下一轮，不一致待仲裁，如果仲裁没结束，上级获取排名信息时得到提示做延迟处理
    void fightResult(const uint8_t* msgData, uint32_t msgSize, const CoffeenetId coffeenetId);


    //临时组队参与比赛

    void sendtoClient(CoffeenetId coffeenetId, water::process::MsgCode2 code2, void* data, uint32_t dataSize);
private:
    FightManager(){}
private:

    bool m_intitOver = false;

    //所有赛事，根据赛事id索引，定时从库内取出创建时间在定时器范围内的新的赛事
    std::map<uint64_t, Fight::Ptr> m_fights;

    //用户可打的比赛，根据条件索引
    //网吧赛事，根据网吧id索引
    std::map<CoffeenetId, std::set<uint64_t>> m_coffFights;
    //市级赛事，根据市索引
    std::map<std::string, std::set<uint64_t>> m_cityFights;
    //省级赛事，根据省索引
    std::map<std::string, std::set<uint64_t>> m_provinceFights;
    //所有用户可打的赛事
    std::set<uint64_t> m_allFights;
    //联盟内可打的赛事

    //后台推荐赛事，五分钟从库内更新一次即可，注意赛事管理中不存在的赛事不推荐
    std::vector<uint64_t> m_recommendFights;

    //在参与赛事的人
    std::map<uint64_t, std::set<uint64_t>> m_objToFights;
};

}
#endif
