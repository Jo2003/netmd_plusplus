/* 
 * File:   Log.h
 * Author: Alberto Lepe <dev@alepe.com>
 *
 * Created on December 1, 2015, 6:00 PM
 */

#pragma once
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

enum typelog
{
    DEBUG,
    INFO,
    WARN,
    ERROR
};

struct structlog
{
    bool headers = false;
    typelog level = WARN;
    std::ostringstream* sout = nullptr;
};

extern structlog LOGCFG;

//------------------------------------------------------------------------------
//! @brief      This class describes a log helper
//------------------------------------------------------------------------------
class LOG
{
public:
    LOG() {}

    LOG(typelog type)
    {
        msglevel = type;
        if(LOGCFG.headers)
        {
            operator << ("["+getLabel(type)+"]");
        }
    }

    ~LOG()
    {
        if(opened)
        {
            if (LOGCFG.sout != nullptr)
            {
                *LOGCFG.sout << std::endl;
            }
            else
            {
                std::cerr << std::endl;
            }
        }
        opened = false;
    }

    template<class T>
    LOG &operator<<(const T &msg)
    {
        if(msglevel >= LOGCFG.level)
        {
            if (LOGCFG.sout != nullptr)
            {
                *LOGCFG.sout << msg;
            }
            else
            {
                std::cerr << msg;
            }
            opened = true;
        }
        return *this;
    }

    static std::string hexFormat(typelog sev, unsigned char* data, std::size_t dataLen)
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

private:

    inline std::string getLabel(typelog type)
    {
        std::string label;
        switch(type)
        {
            case DEBUG: label = "DEBUG"; break;
            case INFO:  label = "INFO "; break;
            case WARN:  label = "WARN "; break;
            case ERROR: label = "ERROR"; break;
        }
        return label;
    }

    bool opened = false;
    typelog msglevel = DEBUG;
};
