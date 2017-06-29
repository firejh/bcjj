/*
 * Author: JiangHeng
 *
 * Created: 2016-11-03 13:18 +0800
 *
 * Modified: 2016-11-03 13:18 +0800
 *
 * Description: 
 */

#ifndef BASE_PROCESS_RAWMSG_MANAGER_H
#define BASE_PROCESS_RAWMSG_MANAGER_H

#include "../component/datetime.h"

#include <memory>
#include <unordered_map>
#include <cstring>

#pragma pack(1)
namespace water{
namespace process{

using MsgCode = uint32_t;
using MsgCode1 = uint16_t;
using MsgCode2 = uint16_t;
typedef std::function<void (const uint8_t*, uint32_t, uint64_t, const component::TimePoint&)> RawmsgHandler;
//using RawmsgHandler = std::function<void(const uint8_t*, uint32_t, uint64_t, const component::TimePoint&)>;

struct RawMsg
{
    RawMsg(){memset(this, 0, sizeof(*this));};
    RawMsg(MsgCode msgcode): code(msgcode){}

    MsgCode code;
    uint8_t data[0];
};

#pragma pack()

class RawmsgManager
{
public:
    static RawmsgManager& me();
    //注册处理msg的handler
    void regHandler(MsgCode1 code1, MsgCode2 code2, RawmsgHandler handler);
    void dealTcpMsg(RawMsg* rev, uint32_t recvSize, uint64_t senderId, component::TimePoint now);
    MsgCode getCode(MsgCode1 code1, MsgCode2 code2);

private:
    std::unordered_map<MsgCode, RawmsgHandler> m_handlers;
};

}}
#endif
