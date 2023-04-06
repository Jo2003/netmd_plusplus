/*
 * netmd_utils.cpp
 *
 * This file is part of netmd++, a library for accessing NetMD devices.
 *
 * It makes use of knowledge / code collected by Marc Britten and
 * Alexander Sulfrian for the Linux Minidisc project.
 *
 * Copyright (C) 2023 Jo2003 (olenka.joerg@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include "netmd_utils.h"
#include "log.h"

namespace netmd {

//--------------------------------------------------------------------------
//! @brief      Calculates the checksum.
//!
//! @param[in]  data     The data
//!
//! @return     The checksum.
//--------------------------------------------------------------------------
unsigned int calculateChecksum(const NetMDByteVector& data)
{
    unsigned int crc  = 0;
    unsigned int temp = data.size();

    for (size_t i = 0; i < data.size(); i++)
    {
        temp = (temp & 0xffff0000) | data[i];
        crc ^= temp;

        for (int j = 0; j < 16; j++)
        {
            unsigned int ts = crc & 0x8000;
            crc <<= 1;
            if (ts)
            {
                crc ^= 0x1021;
            }
        }
    }

    return (crc & 0xffff);
}

//--------------------------------------------------------------------------
//! @brief      format query for netmd exchange
//!
//! @param[in]  format  The format
//! @param[in]  params  The parameters
//! @param[out] query   The query
//!
//! @return     < 0 -> NetMdErr; else -> query size
//--------------------------------------------------------------------------
int formatQuery(const char* format, const NetMDParams& params, NetMDResp& query)
{
    int ret = 0;
    NetMDByteVector queryBuffer;

    uint8_t b;
    char   tok[3]     = {'\0',};
    size_t tokIdx     = 0;
    size_t argno      = 0;
    int    esc        = 0;

    // endianess
    bool bigE = false;

    uint8_t wordBuff[sizeof(uint64_t)] = {0,};

    // remove spaces
    while (*format != '\0')
    {
        // add some kind of sanity check
        if (argno > params.size())
        {
            mLOG(CRITICAL) << "Error sanity check while creating query!";
            return NETMDERR_PARAM;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;

            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    b = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        queryBuffer.push_back(b);
                    }
                    else
                    {
                        // can't convert char* to number
                        mLOG(CRITICAL) << "Can't convert token '" << tok << "' into hex number";
                        return NETMDERR_PARAM;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case 'b':
                if (params.at(argno).index() == UINT8_T)
                {
                    queryBuffer.push_back(std::get<uint8_t>(params.at(argno++)));
                    esc = 0;
                    bigE = false;
                }
                else
                {
                    mLOG(CRITICAL) << "Stored parameter isn't of type BYTE!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'w':
                if (params.at(argno).index() == UINT16_T)
                {
                    auto f = bigE ? toBigEndian<uint16_t> : toLittleEndian<uint16_t>;
                    *reinterpret_cast<uint16_t*>(wordBuff) = f(std::get<uint16_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint16_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                    bigE = false;
                }
                else
                {
                    mLOG(CRITICAL) << "Stored parameter isn't of type WORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'd':
                if (params.at(argno).index() == UINT32_T)
                {
                    auto f = bigE ? toBigEndian<uint32_t> : toLittleEndian<uint32_t>;
                    *reinterpret_cast<uint32_t*>(wordBuff) = f(std::get<uint32_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint32_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                    bigE = false;
                }
                else
                {
                    mLOG(CRITICAL) << "Stored parameter isn't of type DWORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'q':
                if (params.at(argno).index() == UINT64_T)
                {
                    auto f = bigE ? toBigEndian<uint64_t> : toLittleEndian<uint64_t>;
                    *reinterpret_cast<uint64_t*>(wordBuff) = f(std::get<uint64_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint64_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                    bigE = false;
                }
                else
                {
                    mLOG(CRITICAL) << "Stored parameter isn't of type QWORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case '*':
                if (params.at(argno).index() == BYTE_VECTOR)
                {
                    NetMDByteVector ba = std::get<NetMDByteVector>(params.at(argno++));
                    for (const auto& v : ba)
                    {
                        queryBuffer.push_back(v);
                    }
                    esc = 0;
                    bigE = false;
                }
                else
                {
                    mLOG(CRITICAL) << "Stored parameter isn't of type NetMDByteVector!";
                    return NETMDERR_PARAM;
                }
                break;

            case '<':
                // little endian is standard
                break;

            case '>':
                bigE = true;
                break;

            default:
                mLOG(CRITICAL) << "Unsupported format option '"
                           << static_cast<char>(c) << "' used query format!";
                return NETMDERR_PARAM;
                break;
            }
        }
        format++;
    }

    if (!queryBuffer.empty())
    {
        ret   = static_cast<int>(queryBuffer.size());
        query = NetMDResp(new uint8_t[ret]);

        for(int i = 0; i < ret; i++)
        {
            query[i] = queryBuffer.at(i);
        }
    }

    return ret;

}

//--------------------------------------------------------------------------
//! @brief      capture data from netmd response
//!
//! @param[in]  data    The response data
//! @param[in]  size    The data size
//! @param[in]  format  The capture format string
//! @param[out] params  The buffer for captured parameters
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int scanQuery(const uint8_t data[], size_t size, const char* format, NetMDParams& params)
{
    params.clear();
    int     ret                 = NETMDERR_PARAM;
    int     esc                 =  0;
    char    tok[3]              = {'\0',};
    size_t  tokIdx              =  0;
    size_t  dataIdx             = 0;
    uint8_t cmp                 = 0;

    // endianess
    bool bigE = false;

    // remove spaces
    while (*format != '\0')
    {
        if (dataIdx >= size)
        {
            mLOG(CRITICAL) << "Error sanity check scanning response!";
            return ret;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;
            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    cmp = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        if (cmp != data[dataIdx++])
                        {
                            mLOG(CRITICAL) << "Error! Got: " << std::hex
                                       << static_cast<int>(data[dataIdx - 1])
                                       << " expected: " << std::hex << static_cast<int>(cmp) << std::dec;
                            return ret;
                        }
                    }
                    else
                    {
                        // can't convert char* to number
                        mLOG(CRITICAL) << "Can't convert token '" << tok << "' into hex number!";
                        return ret;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case '?':
                esc = 0;
                bigE = false;
                dataIdx ++;
                break;

            case 'b':
                // capture byte
                params.push_back(data[dataIdx++]);
                esc = 0;
                bigE = false;
                break;

            case 'w':
                {
                    // capture word
                    auto f = bigE ? fromBigEndian<uint16_t> : fromLittleEndian<uint16_t>;
                    params.push_back(f(*reinterpret_cast<const uint16_t*>(&data[dataIdx])));
                    dataIdx += sizeof(uint16_t);
                    esc = 0;
                    bigE = false;
                }
                break;

            case 'd':
                {
                    // capture dword
                    auto f = bigE ? fromBigEndian<uint32_t> : fromLittleEndian<uint32_t>;
                    params.push_back(f(*reinterpret_cast<const uint32_t*>(&data[dataIdx])));
                    dataIdx += sizeof(uint32_t);
                    esc = 0;
                    bigE = false;
                }
                break;

            case 'q':
                {
                    // capture qword
                    auto f = bigE ? fromBigEndian<uint64_t> : fromLittleEndian<uint64_t>;
                    params.push_back(f(*reinterpret_cast<const uint64_t*>(&data[dataIdx])));
                    dataIdx += sizeof(uint64_t);
                    esc = 0;
                    bigE = false;
                }
                break;

            case '*':
                // capture byte array
                {
                    NetMDByteVector ba;
                    while (dataIdx < size)
                    {
                        ba.push_back(data[dataIdx++]);
                    }
                    params.push_back(ba);
                    esc = 0;
                    bigE = false;
                }
                break;

            case '<':
                // little endian is standard
                break;

            case '>':
                bigE = true;
                break;

            default:
                mLOG(CRITICAL) << "Unsupported format option '"
                           << static_cast<char>(c) << "' used query format!";
                return ret;
                break;
            }
        }

        format ++;
    }

    if (!params.empty())
    {
        ret = NETMDERR_NO_ERROR;
    }


    return ret;
}

//--------------------------------------------------------------------------
//! @brief      format helper for TrackTime
//!
//! @param      o     ref. to ostream
//! @param[in]  tt    TrackTime
//!
//! @return     formatted TrackTime stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const TrackTime& tt)
{
    o << std::dec
      << std::setw(2) << std::setfill('0') << tt.mMinutes << ":"
      << std::setw(2) << std::setfill('0') << tt.mSeconds << "."
      << std::setw(2) << std::setfill('0') << tt.mTenthSecs;

    return o;
}

//--------------------------------------------------------------------------
//! @brief      format helper for TrackProtection
//!
//! @param      o     ref. to ostream
//! @param[in]  tp    TrackProtection
//!
//! @return     formatted TrackProtection stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const TrackProtection& tp)
{
    switch (tp)
    {
    case TrackProtection::UNPROTECTED:
        o << "UnPROT";
        break;
    case TrackProtection::PROTECTED:
        o << "TrPROT";
        break;
    default:
        o << "N/A";
        break;
    }
    return o;
}

//--------------------------------------------------------------------------
//! @brief      format helper for AudioEncoding
//!
//! @param      o     ref. to ostream
//! @param[in]  ae    AudioEncoding
//!
//! @return     formatted AudioEncoding stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const AudioEncoding& ae)
{
    switch (ae)
    {
    case AudioEncoding::SP:
        o << "SP";
        break;
    case AudioEncoding::LP2:
        o << "LP2";
        break;
    case AudioEncoding::LP4:
        o << "LP4";
        break;
    default:
        o << "N/A";
        break;
    }
    return o;
}



} // ~namespace