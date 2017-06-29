/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-10-09 17:42 +0800
 *
 * Description: 
 */

#ifndef WATER_COMPONET_LOG_HPP
#define WATER_COMPONET_LOG_HPP

#include "logger/logger.h"
#include "logger/file_writer.h"

#include <atomic>
#include <mutex>

namespace water{
namespace component{

class GlobalLogger : public Logger
{
public:
    ~GlobalLogger();
    static GlobalLogger* getMe();
private:
    GlobalLogger() = default;
};


}}

//加载与清除日志，std日志构造时自动加载，不用手动开启；file日志需手动开启；
#define LOG_ADD_FILE(filename) water::component::GlobalLogger::getMe()->setWriter(std::make_shared<water::component::FileWriter>(filename));
#define LOG_CLEAR_FILE water::component::GlobalLogger::getMe()->clearWriter(water::component::WriterType::fileOut);
#define LOG_CLEAR_STD water::component::GlobalLogger::getMe()->clearWriter(water::component::WriterType::stdOut);

#define LOG_DEBUG(args...) water::component::GlobalLogger::getMe()->debug(args)
#define LOG_TRACE(args...) water::component::GlobalLogger::getMe()->trace(args)
#define LOG_ERROR(args...) water::component::GlobalLogger::getMe()->error(args)

#endif
