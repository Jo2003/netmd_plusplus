/*
 * netmd_defines.h
 *
 * This file is part of netmd++, a library for accessing NetMD devices.
 *
 * It makes use of knowledge / code collected by Marc Britten and
 * Alexander Sulfrian for the Linux Minidisc project.
 *
 * Asivery helped to make this possible!
 * Sir68k discovered the Sony FW exploit!
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
enum NetMdErr : int
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
    NETMDERR_INTERIM       = -9,  ///< interim
};

/// disk format
enum DiskFormat : uint8_t
{
    NETMD_DISKFORMAT_LP4       =   0,
    NETMD_DISKFORMAT_LP2       =   2,
    NETMD_DISKFORMAT_SP_MONO   =   4,
    NETMD_DISKFORMAT_SP_STEREO =   6,
    NO_ONTHEFLY_CONVERSION     = 0xf
};

/// track times
struct TrackTime
{
    int mMinutes;
    int mSeconds;
    int mTenthSecs;
};

/// type safe protection flags
enum class TrackProtection : uint8_t
{
    UNPROTECTED = 0x00,
    PROTECTED   = 0x03,
    UNKNOWN     = 0xFF
};

/// type safe encoding flags
enum class AudioEncoding : uint8_t
{
    SP      = 0x90,
    LP2     = 0x92,
    LP4     = 0x93,
    UNKNOWN = 0xff
};

/// helper structure to throw an error with description
struct ThrownData
{
    int mErr;
    std::string mErrDescr;
};

/// NetMD time
struct NetMdTime
{
    uint16_t hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  frame;
};

/// Structure to hold the capacity information of a disc.
struct DiscCapacity
{
    /// Time allready recorded on the disc.
    NetMdTime recorded;

    /// Total time, that could be recorded on the disc. This depends on the
    /// current recording settings.
    NetMdTime total;

    /// Time that is available on the disc. This depends on the current
    /// recording settings.
    NetMdTime available;
};

//-----------------------------------------------------------------------------
//! @brief      track group
//-----------------------------------------------------------------------------
struct Group
{
    int      mGid;      //!< group id
    int16_t  mFirst;    //!< first track
    int16_t  mLast;     //!< last track
    std::string mName;  //!< group name
};

using Groups = std::vector<Group>;

constexpr uint8_t NETMD_CHANNELS_MONO   = 0x01;
constexpr uint8_t NETMD_CHANNELS_STEREO = 0x00;

} // ~namespace