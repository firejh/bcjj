#ifndef COFFEENET_DEF_H
#define COFFEENET_DEF_H

#include "stdint.h"
#include <string>

namespace game{

using CoffeenetId = uint64_t;

struct CoffeenetData
{
    CoffeenetId coffeenetId;    //网吧id
    std::string name;           //网吧名称
};

}

#endif
