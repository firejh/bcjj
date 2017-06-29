#include "db_connection.h"

#include "../component/logger.h"
#include "../component/scope_guard.h"

#include <thread>

namespace water{
namespace process{

DbConnection::DbConnection(const std::string host, const std::string user, const std::string passwd, const std::string name, const uint16_t port):
  m_host(host), m_user(user), m_passwd(passwd), m_name(name), m_port(port)
{
}

DbConnection::~DbConnection()
{
    mysql_close(&m_mysql);
    mysql_library_end();
}

bool DbConnection::init()
{
    LOG_DEBUG("DbConnection, mysql init");
    //初始化
    mysql_init(&m_mysql);

    //设置重连
    char value = 1;
    mysql_options(&m_mysql, MYSQL_OPT_RECONNECT, &value);

    mysql_options(&m_mysql, MYSQL_SET_CHARSET_NAME, "utf8");
    //连接
    if(!mysql_real_connect(&m_mysql, m_host.c_str(), m_user.c_str(), m_passwd.c_str(), m_name.c_str(), m_port, NULL, 0))
    {
        LOG_ERROR("DbConnection, mysql连接错误, m_host={}, m_user={}, m_passwd={}, m_name={}, m_port={}",
              m_host, m_user, m_passwd, m_name, m_port);
        return false;
    }
    LOG_TRACE("DbConnection, 新的mysql连接建立, m_host={}, m_user={}, m_passwd={}, m_name={}, m_port={}",
              m_host, m_user, m_passwd, m_name, m_port);
    return true;
}

bool DbConnection::exec()
{
    while(checkSwitch())
    {
        while(!m_callBackSqlIn.empty())
        {
            //获取sql
            std::pair<std::string, BackData> temp;
            if(!m_callBackSqlIn.pop(&temp))
            {
                LOG_ERROR("DbConnection, 存储队列非空但是pop失败");
                return false;
            } 

            //执行sql
            if(mysql_real_query(&m_mysql, temp.first.data(), temp.first.size()) != 0)
            {
                std::string error = mysql_error(&m_mysql);
                LOG_ERROR("DbConnection, sql error: {}", error);
                //continue;
            }

            MYSQL_RES *rs = NULL;
            do
            {
                //这里不支持一次执行多次查询操作
                MYSQL_RES *temp = mysql_store_result(&m_mysql);
                if(!temp)
                    continue;
                rs = temp;

            }while(!mysql_next_result(&m_mysql));

            //加入回调队列
            if(temp.second.m_handle != nullptr)
                m_callBackSqlOut.push({rs, temp.second});
            //LOG_DEBUG("DbConnection, sql执行并生成回调队列, sql={}", temp.first);
        }

        //一定要sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}

bool DbConnection::callBackTimerExec()
{
    while(!m_callBackSqlOut.empty())
    {
        //取出回调
        std::pair<MYSQL_RES*, BackData> temp;
        if(!m_callBackSqlOut.pop(&temp))
        {
            LOG_ERROR("DbConnection, 回调队列非空但是pop失败");
            return false;//出现的话就乱了，不改出现
        }

        //本次循环结束自动化释放MYSQL_RES*
        ON_EXIT_SCOPE_DO(mysql_free_result(temp.first));

        //LOG_DEBUG("DbConnection, 回调执行");
        //无效的回调，即无回调
        if(temp.second.m_handle == nullptr)
            continue;

        //执行回调
        temp.second.m_handle(temp.first, &temp.second.m_data);
    }
    return true;
}

void DbConnection::exeStoreSql(std::string sql, EventHandler handler, uint64_t data)
{
    //LOG_DEBUG("DbConnection, 收到sql, sql={}", sql);

    BackData backData(data, handler);

    if(!m_callBackSqlIn.push({sql, backData}))
    {
        LOG_ERROR("DbConnection, 存储循环队列满");
    }
}

}}
