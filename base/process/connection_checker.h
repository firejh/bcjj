/*
 * Author: JiangHeng
 *
 * Created: 2016-11-03 09:49 +0800
 *
 * Modified: 2016-11-03 09:49 +0800
 *
 * Description: 
 */

#ifndef WATER_PROCESS_CONNECTION_CHECKET_H 
#define WATER_PROCESS_CONNECTION_CHECKET_H 

#include "process_thread.h"
#include "process_config.h"

#include "../component/event.h"
#include "../net/packet_connection.h"

namespace water{
namespace process{


class ConnectionChecker : public ProcessThread
{
public:
    TYPEDEF_PTR(ConnectionChecker)

    ConnectionChecker() = default;
    virtual ~ConnectionChecker() = default;

    enum class ConnType {in, out};
    virtual void addUncheckedConnection(net::PacketConnection::Ptr conn, ConnType type) = 0;

public:
    component::Event<void (net::PacketConnection::Ptr, ProcessId processId)> e_connConfirmed;

protected:
    virtual void checkConn() = 0;

private:
    bool exec() override;
};

}}

#endif
