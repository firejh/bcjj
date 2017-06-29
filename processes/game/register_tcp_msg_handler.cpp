/*
 * Author: JiangHeng
 *
 * Created: 2016-11-09 13:26 +0800
 *
 * Modified: 2016-11-09 13:26 +0800
 *
 * Description: 
 */

#include "game.h"
#include "coffeenet/coffeenet_manager.h"
#include "user/user_manager.h"
#include "fight/team_manager.h"
#include "fight/fight_manager.h"

namespace game{

void Game::registerTcpMsgHandler()
{
    CoffeenetManager::me().regMsgHandler();//网吧客户端消息处理
    UserManager::me().regMsgHandler();//用户消息处理

    TeamManager::me().regMsgHandler();//战队消息处理

    FightManager::me().regMsgHandler();
}

}
