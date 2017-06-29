#include "game.h"

int main()
{
    game::Game::init("game", 3, "config", "log");
    game::Game::me().start();
    return 1;
}

