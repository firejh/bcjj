/*
 * Author: JiangHeng
 *
 * Created: 2016-11-04 13:48 +0800
 *
 * Modified: 2016-11-04 13:48 +0800
 *
 * Description: 数据库mysql线程，只支持一单线程进单线程出操作，不可多进多出，因为内部使用无锁队列使用的高效队列只是但进单出安全的
 */
#ifndef BASE_DB_CONNECTION_H
#define BASE_DB_CONNECTION_H

#include "process_thread.h"

#include "../component/spinlock.h"
#include "../component/lock_free_circular_queue_ss.h"

#include "/usr/local/mysql/include/mysql.h"

#include <string>
#include <list>
#include <functional>
#include <vector>

namespace water{
namespace process{

class DbConnection final : public ProcessThread
{
public:
    using  EventHandler = std::function<void(MYSQL_RES* res, void* data)>;
    struct BackData
    {
        BackData(){}
        BackData(uint64_t data, EventHandler handle): m_data(data), m_handle(handle){}
        uint64_t m_data;          //回调的数据，暂时就简单的是一个64位数据，需要的话可以封装的更好些
        EventHandler m_handle;    //回调函数
    };

public:
    TYPEDEF_PTR(DbConnection)
    CREATE_FUN_MAKE(DbConnection)


    DbConnection(const std::string host, const std::string user, const std::string passwd, const std::string name, const uint16_t port);
    ~DbConnection();

    bool init();

    bool exec() override;
    bool callBackTimerExec();
    void exeStoreSql(std::string sql, EventHandler handerler, uint64_t data = 0);

private:
    std::string m_host;
    std::string m_user;
    std::string m_passwd;
    std::string m_name;
    uint16_t m_port;
    MYSQL m_mysql;

    component::LockFreeCircularQueueSPSC<std::pair<std::string, BackData>> m_callBackSqlIn;
    component::LockFreeCircularQueueSPSC<std::pair<MYSQL_RES*, BackData>> m_callBackSqlOut;
};

}}
#endif
