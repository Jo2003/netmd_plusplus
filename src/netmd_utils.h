/*
 * netmd_utils.h
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
#pragma once
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include "netmd_defines.h"

namespace netmd {

/// macro to throw an exception
#define mNetMdThrow(x_, ...) { std::ostringstream os_; os_ << __FUNCTION__ << "():" << __LINE__ << ": " << __VA_ARGS__; throw ThrownData{x_, os_.str()}; }

//--------------------------------------------------------------------------
//! @brief      format query for netmd exchange
//!
//! @param[in]  format  The format
//! @param[in]  params  The parameters
//! @param[out] query   The query
//!
//! @return     < 0 -> NetMdErr; else -> query size
//--------------------------------------------------------------------------
int formatQuery(const char* format, const NetMDParams& params, NetMDResp& query);

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
int scanQuery(const uint8_t data[], size_t size, const char* format, NetMDParams& params);

//--------------------------------------------------------------------------
//! @brief      Calculates the checksum.
//!
//! @param[in]  data     The data
//!
//! @return     The checksum.
//--------------------------------------------------------------------------
uint16_t calculateChecksum(const NetMDByteVector& data);

//--------------------------------------------------------------------------
//! @brief      format helper for TrackTime
//!
//! @param      o     ref. to ostream
//! @param[in]  tt    TrackTime
//!
//! @return     formatted TrackTime stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const TrackTime& tt);

//--------------------------------------------------------------------------
//! @brief      format helper for AudioEncoding
//!
//! @param      o     ref. to ostream
//! @param[in]  ae    AudioEncoding
//!
//! @return     formatted AudioEncoding stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const AudioEncoding& ae);

//--------------------------------------------------------------------------
//! @brief      format helper for TrackProtection
//!
//! @param      o     ref. to ostream
//! @param[in]  tp    TrackProtection
//!
//! @return     formatted TrackProtection stored in ostream
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& o, const TrackProtection& tp);

//------------------------------------------------------------------------------
//! @brief      swop bytes
//!
//! @param      val   The value
//!
//! @tparam     T     word, dword or qword
//!
//! @return     value
//------------------------------------------------------------------------------
template <typename T>
T& byteSwop(T& val)
{
    int sz  = sizeof(T);

    // check size ... up too 8 bytes are supported ...
    switch (sz)
    {
    case 1:
        // nothing to do ...
        break;

    case 2:
    case 4:
    case 8:
        {
            T   trg = 0;
            int i, j;

            // swab bytes ...
            for (i = sz - 1, j = 0; j < sz; i--, j++)
            {
                trg |= ((val >> (i * 8)) & 0xFF) << (j * 8);
            }

            val = trg;
        }
        break;

    default:
        // don't touch anything ...
        break;
    }

    return val;
}

//------------------------------------------------------------------------------
//! @brief      Determines if big endian.
//!
//! @return     True if big endian, False otherwise.
//------------------------------------------------------------------------------
inline bool is_big_endian()
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

//------------------------------------------------------------------------------
//! @brief      convert byte order for data coming from NetMD
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T fromLittleEndian(const T& val)
{
    T v = val;
    if (is_big_endian())
    {
        byteSwop(v);
    }
    return v;
}

//------------------------------------------------------------------------------
//! @brief      convert byte vector ro little endian integer type
//!
//! @param[in]  val   The value
//!
//! @tparam     T     integer type
//!
//! @return     converted integer
//------------------------------------------------------------------------------
template <typename T>
T fromLittleEndianByteVector(const NetMDByteVector& val)
{
    if(sizeof(T) == val.size())
    {
        uint8_t data[val.size()];
        for (size_t i = 0; i < val.size(); i++)
        {
            data[i] = val.at(i);
        }
        return fromLittleEndian(*reinterpret_cast<T*>(data));
    }
    else
    {
        return static_cast<T>(-1);
    }
}

//------------------------------------------------------------------------------
//! @brief      convert byte order for data sending to NetMD
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T toLittleEndian(const T& val)
{
    T v = val;
    if (is_big_endian())
    {
        byteSwop(v);
    }
    return v;
}

//------------------------------------------------------------------------------
//! @brief      convert integer type into little endian byte vector
//!
//! @param[in]  val   The value
//!
//! @tparam     T     integer type
//!
//! @return     The net md byte vector.
//------------------------------------------------------------------------------
template <typename T>
NetMDByteVector toLittleEndianByteVector(const T& val)
{
    NetMDByteVector ret;
    uint8_t data[sizeof(val)];
    *reinterpret_cast<T*>(data) = toLittleEndian(val);

    for (size_t i = 0; i < sizeof(T); i++)
    {
        ret.push_back(data[i]);
    }
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      convert byte order from big endian
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T fromBigEndian(const T& val)
{
    T v = val;
    if (!is_big_endian())
    {
        byteSwop(v);
    }
    return v;
}

//------------------------------------------------------------------------------
//! @brief      convert byte order to big endian
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T toBigEndian(const T& val)
{
    T v = val;
    if (!is_big_endian())
    {
        byteSwop(v);
    }
    return v;
}

//------------------------------------------------------------------------------
//! @brief      convert byte order from big endian
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T fromBigEndianArray(const uint8_t* val)
{
    T v = *reinterpret_cast<const T*>(val);
    return fromBigEndian(v);
}

//------------------------------------------------------------------------------
//! @brief      convert byte order from little endian
//!
//! @param[in]  val   The value
//!
//! @tparam     T     type to convert
//!
//! @return     converted data
//------------------------------------------------------------------------------
template <typename T>
T fromLittleEndianArray(const uint8_t* val)
{
    T v = *reinterpret_cast<const T*>(val);
    return fromLittleEndian(v);
}

//------------------------------------------------------------------------------
//! @brief      add bytes to byte vector
//!
//! @param      vec     The vector
//! @param[in]  data    The data
//! @param[in]  dataSz  The data size
//------------------------------------------------------------------------------
inline void addArrayData(NetMDByteVector& vec, const uint8_t* data, size_t dataSz)
{
    for (size_t i = 0; i < dataSz; i++)
    {
        vec.push_back(data[i]);
    }
}

//------------------------------------------------------------------------------
//! @brief      do strange netmd value handling
//!
//! @param[in]  value  The value
//!
//! @return     the converted value
//------------------------------------------------------------------------------
inline unsigned char proper_to_bcd_single(unsigned char value)
{
    unsigned char high, low;

    low = (value % 10) & 0xf;
    high = (((value / 10) % 10) * 0x10U) & 0xf0;

    return high | low;
}

//------------------------------------------------------------------------------
//! @brief      do strange netmd value handling
//!
//! @param[in]  value   The value
//! @param      target  The target
//! @param[in]  len     The length
//!
//! @return     the converted value
//------------------------------------------------------------------------------
inline unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len)
{
    while (value > 0 && len > 0) {
        target[len - 1] = proper_to_bcd_single(value & 0xff);
        value /= 100;
        len--;
    }

    return target;
}

//------------------------------------------------------------------------------
//! @brief      do strange netmd value handling
//!
//! @param[in]  value  The value
//!
//! @return     the converted value
//------------------------------------------------------------------------------
inline unsigned char bcd_to_proper_single(unsigned char value)
{
    unsigned char high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return ((high * 10U) + low) & 0xff;
}

//------------------------------------------------------------------------------
//! @brief      do strange netmd value handling
//!
//! @param[in]  value  The value
//! @param[in]  len    The length
//!
//! @return     the converted value
//------------------------------------------------------------------------------
inline unsigned int bcd_to_proper(unsigned char* value, size_t len)
{
    unsigned int result = 0;
    unsigned int nibble_value = 1;

    for (; len > 0; len--) {
        result += nibble_value * bcd_to_proper_single(value[len - 1]);

        nibble_value *= 100;
    }

    return result;
}

//------------------------------------------------------------------------------
//! @brief      parse netmd time
//!
//! @param      src   The source
//! @param      time  The time
//------------------------------------------------------------------------------
void parse_time(uint8_t* src, NetMdTime& time);


} // ~namespace
