/*
 * Author: JiangHeng
 *
 * Created: 2016-11-04 19:22 +0800
 *
 * Modified: 2016-11-04 19:22 +0800
 *
 * Description: 
 */
#ifndef PROCESSES_GAME_H
#define PROCESSES_GAME_H

#include "base/process/process.h"
#include "base/process/rawmsg_manger.h"

namespace game{

using namespace water;
using namespace process;

class Game : public water::process::Process
{
public:
    static Game* m_me;
    static void init(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir);
    static Game& me();
    ~Game(){};

    void init() override;

    //时间和消息注册
    void registerTcpMsgHandler();
    void registerTimerHandler();

    //连接管理
    void erasePublicConn(TcpConnectionManager::ClinetIdentity clientId);

    //消息处理
    void sendtoClient(TcpConnectionManager::ClinetIdentity clientId, MsgCode1 code1, MsgCode2 code2, const void* data, uint32_t dataSize);

public:
    //通用的一些时间注册，这里为了统一和方便，也可以加上一些整点的处理函数
    void registerTimer50ms(const component::TimePoint& now);
    void registerTimer500ms(const component::TimePoint& now);
    void registerTimer1s(const component::TimePoint& now);
    void registerTimer10s(const component::TimePoint& now);
    void registerTimer1min(const component::TimePoint& now);
    void registerTimer5min(const component::TimePoint& now);
private:
    Game(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir);

private:
};

}
#endif
