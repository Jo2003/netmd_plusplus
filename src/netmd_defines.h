/*
 * netmd_defines.h
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
#include <cstdint>
#include <map>
#include <vector>
#include <memory>
#include <variant>
#include <string>

namespace  netmd  {

#define mBYTE(x_) static_cast<uint8_t>(x_)
#define mWORD(x_) static_cast<uint16_t>(x_)
#define mDWORD(x_) static_cast<uint32_t>(x_)
#define mQWORD(x_) static_cast<uint64_t>(x_)

// ease memory management
using NetMDResp       = std::unique_ptr<uint8_t[]>;
using NetMDByteVector = std::vector<uint8_t>;
using NetMDParam      = std::variant<uint8_t, uint16_t, uint32_t, uint64_t, NetMDByteVector>;
using NetMDParams     = std::vector<NetMDParam>;

/// type definitaion stored in param
enum NetMDParamType
{
    UINT8_T,        ///< uint8_t
    UINT16_T,       ///< uint16_t
    UINT32_T,       ///< uint32_t
    UINT64_T,       ///< uint64_t
    BYTE_VECTOR     ///< @see NetMDByteVector
};

/// NetMD errors
enum NetMdErr
{
    NETMDERR_NO_ERROR      =  0,  ///< success
    NETMDERR_USB           = -1,  ///< general USB error
    NETMDERR_NOTREADY      = -2,  ///< player not ready for command
    NETMDERR_TIMEOUT       = -3,  ///< timeout while waiting for response
    NETMDERR_CMD_FAILED    = -4,  ///< minidisc responded with 08 response
    NETMDERR_CMD_INVALID   = -5,  ///< minidisc responded with 0A response
    NETMDERR_PARAM         = -6,  ///< parameter error
    NETMDERR_OTHER         = -7,  ///< any other error
    NETMDERR_NOT_SUPPORTED = -8,  ///< not supported
};

/// helper structure to throw an error with description
struct ThrownData
{
    int mErr;
    std::string mErrDescr;
};

} // ~namespace