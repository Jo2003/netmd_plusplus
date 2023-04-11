/* 
 * File:   Log.h
 * Author: Alberto Lepe <dev@alepe.com>
 *
 * Created on December 1, 2015, 6:00 PM
 */

#pragma once
#include <cstddef>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <mutex>

enum typelog
{
    DEBUG,
    INFO,
    WARN,
    CRITICAL,
    CAPTURE //!< needed for log parcing!
};

struct structlog
{
    bool headers = false;
    bool time = false;
    int level = WARN;
    std::ostream* sout = &std::cerr;
    std::mutex mtxLog;
};

extern structlog LOGCFG;

#define mLOG(x_) LOG(x_) << __FUNCTION__ << "():" << __LINE__ << ": "

//------------------------------------------------------------------------------
//! @brief      This class describes a log helper
//------------------------------------------------------------------------------
class LOG
{
public:
    LOG() {}

    LOG(int type)
    {
        msglevel = type;

        if(LOGCFG.time)
        {
            operator << (timeStamp());
        }

        if(LOGCFG.headers)
        {
            operator << (getLabel(type) +"|");
        }
    }

    ~LOG()
    {
        std::unique_lock lck(LOGCFG.mtxLog);
        if(opened)
        {
            *LOGCFG.sout << std::endl;
        }
        opened = false;
    }

    template<class T>
    LOG &operator<<(const T &msg)
    {
        std::unique_lock lck(LOGCFG.mtxLog);
        if(msglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << msg;
            opened = true;
        }
        return *this;
    }

    static std::string hexFormat(int sev, const unsigned char* data, std::size_t dataLen)
    {
        if (sev < LOGCFG.level)
        {
            return std::string{};
        }

        std::ostringstream oss;
        std::size_t i;
        std::size_t j = 0;
        int breakpoint = 0;

        oss << std::endl;

        for (i = 0; i < dataLen; i++)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]) << std::dec << " ";

            breakpoint++;

            if(!((i + 1) % 16) && i)
            {
                oss << "\t\t";

                for(j = ((i + 1) - 16); j < ((i + 1) / 16) * 16; j++)
                {
                    if(data[j] < 30)
                    {
                        oss << ".";
                    }
                    else
                    {
                        oss << data[j];
                    }
                }
                oss << std::endl;
                breakpoint = 0;
            }
        }

        if(breakpoint == 16)
        {
            return oss.str();
        }

        for(; breakpoint < 16; breakpoint++)
        {
            oss << "   ";
        }

        oss << "\t\t";

        for(j = dataLen - (dataLen % 16); j < dataLen; j++)
        {
            if(data[j] < 30)
            {
                oss << ".";
            }
            else
            {
                oss << data[j];
            }
        }

        return oss.str();
    }

    static std::string timeStamp()
    {
        std::ostringstream oss;
        auto time = std::time(nullptr);

        // ISO 8601: %Y-%m-%d %H:%M:%S, e.g. 2017-07-31 00:42:00+0200.
        oss <<  std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S|");
        return oss.str();
    }

private:

    inline std::string getLabel(int type)
    {
        std::string label;
        switch(type)
        {
            case DEBUG:    label = "DEBUG"   ; break;
            case INFO:     label = "INFO"    ; break;
            case WARN:     label = "WARN"    ; break;
            case CRITICAL: label = "CRITICAL"; break;
            case CAPTURE:  label = "CAPTURE" ; break;
        }
        return label;
    }

    bool opened = false;
    int msglevel = DEBUG;
};
