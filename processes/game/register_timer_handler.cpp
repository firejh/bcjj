#include "game.h"

#include "coffeenet/coffeenet_checker.h"
#include "fight/fight_manager.h"

#include <functional>

namespace game{

void Game::registerTimerHandler()
{
    using namespace std::placeholders;
    Game::me().regTimer(std::chrono::milliseconds(50), std::bind(&Game::registerTimer50ms, this, _1));
    Game::me().regTimer(std::chrono::milliseconds(500), std::bind(&Game::registerTimer500ms, this, _1));
    Game::me().regTimer(std::chrono::milliseconds(1000), std::bind(&Game::registerTimer1s, this, _1));
    Game::me().regTimer(std::chrono::milliseconds(10000), std::bind(&Game::registerTimer10s, this, _1));
    Game::me().regTimer(std::chrono::milliseconds(60000), std::bind(&Game::registerTimer1min, this, _1));
    Game::me().regTimer(std::chrono::milliseconds(300000), std::bind(&Game::registerTimer5min, this, _1));
    
}

void Game::registerTimer50ms(const component::TimePoint& now)
{
    ClientChecker::me().timerExec(now);//客户端登录连接处理，包括超时处理
}

void Game::registerTimer500ms(const component::TimePoint& now)
{
    
}

void Game::registerTimer1s(const component::TimePoint& now)
{

}

void Game::registerTimer10s(const component::TimePoint& now)
{

}

void Game::registerTimer1min(const component::TimePoint& now)
{
    FightManager::me().initNewFighting();//更新新的赛事到内存
    FightManager::me().initRecommendFighting();//更新首页的推荐赛事
    FightManager::me().contrl();
}

void Game::registerTimer5min(const component::TimePoint& now)
{
}
}
