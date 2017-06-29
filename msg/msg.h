#ifndef MSG_MSG_H
#define MSG_MSG_H

namespace msg{

enum class MsgType : uint16_t
{
    error = 0,          //错误
    coffeenet = 1,      //网吧
    user = 2,           //用户
    team = 3,           //战队
    fight = 4,          //赛事
};

}
#endif
