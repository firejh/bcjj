#include "game.h"
#include "coffeenet/coffeenet_manager.h"
#include "coffeenet/coffeenet_checker.h"

#include "component/exception.h"
#include "component/scope_guard.h"
#include "component/logger.h"

#include "fight/fight_manager.h"
#include "fight/team_manager.h"

namespace game{

Game* Game::m_me = nullptr;

Game& Game::me()
{
    return *m_me;
}

void Game::init(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir)
{
    m_me = new Game(name, processId, configDir, logDir);
}

Game::Game(const std::string& name, const uint64_t processId, const std::string& configDir, const std::string& logDir): Process(name, processId, configDir, logDir)
{
}

void Game::init()
{
    Process::init();

    if(m_publicNetServer == nullptr)
    {
        EXCEPTION(component::ExceptionBase, "无公网监听，清查看配置")
    }
        
    using namespace std::placeholders;
    //注册公网新连接处理
    m_publicNetServer->e_newConn.reg(std::bind(&ClientChecker::addUncheckedConnection,
                                                   &ClientChecker::me(), _1));
    //注册公网连接检查处理
    ClientChecker::me().e_clientConfirmed.reg(std::bind(&TcpConnectionManager::addPublicConnection, &m_conns, _1, _2));
    //连接管理中的连接失效后处理
    m_conns.e_afterErasePublicConn.reg(std::bind(&CoffeenetManager::delCoffeenet, &CoffeenetManager::me(), _1));

    //消息和定时器注册
    registerTcpMsgHandler();
    registerTimerHandler();


    //业务初始化
    FightManager::me().init();
    TeamManager::me().init();
}

void Game::erasePublicConn(TcpConnectionManager::ClinetIdentity clientId)
{
    return m_conns.erasePublicConnection(clientId);
}

void Game::sendtoClient(TcpConnectionManager::ClinetIdentity clientId, MsgCode1 code1, MsgCode2 code2, const void* data, uint32_t dataSize)
{
    MsgCode code = RawmsgManager::me().getCode(code1, code2);
    const uint32_t bufSize = sizeof(MsgCode) + dataSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);
    RawMsg* msg = new(buf) RawMsg(code);
    memcpy(msg->data, data, dataSize);
    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(msg, bufSize);

    m_conns.sendPacketToPublic(clientId, packet);
    LOG_DEBUG("send msg to client, clientId={}, code1={}, code2={}, size={}", clientId, code1, code2, dataSize);
}

}
