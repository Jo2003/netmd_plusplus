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
#include "netmd_defines.h"

namespace netmd {

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
//! @param[in]  dataLen  The data length
//!
//! @return     The checksum.
//--------------------------------------------------------------------------
unsigned int calculateChecksum(unsigned char* data, size_t dataLen);

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
            // In case this is to cryptic, use algorithm below!
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
T fromNetMD(const T& val)
{
    T v = val;
    if (is_big_endian())
    {
        byteSwop(v);
    }
    return v;
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
T toNetMD(const T& val)
{
    T v = val;
    if (is_big_endian())
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

inline unsigned char proper_to_bcd_single(unsigned char value)
{
    unsigned char high, low;

    low = (value % 10) & 0xf;
    high = (((value / 10) % 10) * 0x10U) & 0xf0;

    return high | low;
}

inline unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len)
{
    while (value > 0 && len > 0) {
        target[len - 1] = proper_to_bcd_single(value & 0xff);
        value /= 100;
        len--;
    }

    return target;
}

inline unsigned char bcd_to_proper_single(unsigned char value)
{
    unsigned char high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return ((high * 10U) + low) & 0xff;
}

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


} // ~namespace