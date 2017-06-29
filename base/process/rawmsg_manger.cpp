#include "rawmsg_manger.h"

#include "../component/logger.h"
#include <stdio.h>

#include "string.h"
namespace water{
namespace process{

RawmsgManager& RawmsgManager::me()
{
    static RawmsgManager me;
    return me;
}

void RawmsgManager::regHandler(MsgCode1 code1, MsgCode2 code2, RawmsgHandler handler)
{
     MsgCode msgCode = getCode(code1, code2);
     auto retInsert = m_handlers.insert({msgCode, handler});
     if(!retInsert.second)
     {
        LOG_ERROR("regHandler error, repeat code, code1={}, code={}", code1, code2);
     }
}

void RawmsgManager::dealTcpMsg(RawMsg* rev, uint32_t recvSize, uint64_t senderId, component::TimePoint now)
{
    if(!rev)
    {
        LOG_DEBUG("handle msg no code");
        return;
    }
    auto iterToHandler = m_handlers.find(rev->code);
    if(iterToHandler == m_handlers.end())
    {   
        struct Code
        {
            MsgCode1 code1;
            MsgCode2 code2;
        };
        Code* p = reinterpret_cast<Code*>(&(rev->code));
        LOG_ERROR("handle msg error, missing rawmsg handler, code = {}, code1={}, code2={}", rev->code, p->code1, p->code2);
        return;
    }   

    iterToHandler->second(rev->data, recvSize - sizeof(MsgCode), senderId, now);
    return;
}

MsgCode RawmsgManager::getCode(MsgCode1 code1, MsgCode2 code2)
{
    struct{
        MsgCode1 code1;
        MsgCode2 code2;
    }code;
    code.code1 = code1;
    code.code2 = code2;

    const MsgCode* ret = reinterpret_cast<MsgCode*>(&code);
    return *ret;
}

}}
