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
#include <vector>

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

#ifdef __GNUC__
    constexpr std::string_view method_name(const char* s)
    {
        std::string_view prettyFunction(s);
        size_t bracket = prettyFunction.rfind("(");
        size_t colon1  = prettyFunction.rfind("::", bracket);
        size_t space   = std::string::npos;
        if (colon1 != std::string::npos)
        {
            size_t colon2  = prettyFunction.rfind("::", colon1 - 1);
            if (colon2 != std::string::npos)
            {
                space = colon2 + 2;
            }
        }
        else
        {
            space = prettyFunction.rfind(" ", bracket) + 1;
        }
        return prettyFunction.substr(space, bracket-space);
    }
    #define __METHOD_NAME__ method_name(__PRETTY_FUNCTION__)
#else
    #define __METHOD_NAME__ __FUNCTION__
#endif

#define mLOG(x_) LOG(x_) << __METHOD_NAME__ << "():" << __LINE__ << ": "

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
		            *this << timeStamp();
        }

        if(LOGCFG.headers)
        {
            *this  << getLabel(type) << "|";
        }
    }

    ~LOG()
    {
        std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);
        if(opened)
        {
            *LOGCFG.sout << std::endl;
        }
        opened = false;
    }

    template<class T>
    LOG &operator<<(const T &msg)
    {
        std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);
        if(msglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << msg << std::flush;
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

    static std::string hexFormat(int sev, const std::vector<uint8_t>& data)
    {
        if (sev < LOGCFG.level)
        {
            return std::string{};
        }

        std::ostringstream oss;
        std::size_t i;
        std::size_t j = 0;
        std::size_t dataLen = data.size();
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

    static std::string getLabel(int type)
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

private:
    bool opened = false;
    int msglevel = DEBUG;
};

#define mFLOW(x_) Flow flow_(x_, std::string{__METHOD_NAME__})

//------------------------------------------------------------------------------
//! @brief      Helper class to indicate the program flow
//------------------------------------------------------------------------------
class Flow
{
public:
    Flow(int sev, const std::string& f) 
        : mMsglevel(sev), mFunc(f)
    {
        std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);

        if(LOGCFG.time && mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << LOG::timeStamp();
        }

        if(LOGCFG.headers && mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout  << LOG::getLabel(mMsglevel) << "|";
        }

        if(LOGCFG.headers && mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << mFunc << "() --> in" << std::endl;
        }
    }

    ~Flow()
    {
        std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);
        if(LOGCFG.time && mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << LOG::timeStamp();
        }

        if(LOGCFG.headers && mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout  << LOG::getLabel(mMsglevel) << "|";
        }

        if(mMsglevel >= LOGCFG.level)
        {
            *LOGCFG.sout << mFunc << "() <-- out" << std::endl;
        }
    }

private:
    int mMsglevel;
    std::string mFunc;
};
