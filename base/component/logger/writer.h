/*
 * Author: JiangHeng
 *
 * Created: 2016-10-31 18:20 +0800
 *
 * Modified: 2016-10-31 18:20 +0800
 *
 * Description: 志模块,写策略，基类
 */
#ifndef WATER_COMPONET_LOG_WRITER_HPP
#define WATER_COMPONET_LOG_WRITER_HPP
#include <stdint.h>

namespace water {
namespace component{

enum class WriterType
{
    stdOut = 1,
    fileOut = 2,
};

class Writer
{
public:
    Writer(const WriterType type):wtType(type)
    {
    }
    virtual ~Writer(){}
public:
    virtual void append(const char* msg, const uint32_t len) = 0;
    virtual void start() = 0; 
    virtual void stop() = 0;
public:
    WriterType getWriteType() const
    {
        return wtType;
    }
private:
    WriterType wtType;
};

}
}

#endif
