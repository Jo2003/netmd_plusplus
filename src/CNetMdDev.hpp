/*
 * CNetMdDev.hpp
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
#include <cstdlib>
#include <libusb-1.0/libusb.h>
#include <cstdint>
#include <string>

#include "netmd_defines.h"
#include "log.h"

namespace netmd
{

/// announce API class
class CNetMdApi;

//------------------------------------------------------------------------------
//! @brief      This class describes a NetMd device
//------------------------------------------------------------------------------
class CNetMdDev
{
    /// describe a known NetMD device
    struct SKnownDevice
    {
        uint16_t mVendorID;     ///< vendor id
        uint16_t mDeviceID;     ///< device id
        const char* mModel;     ///< model name
        bool mOtfEncode;        ///< device supports on-the-fly LP encoding
    };

    /// dev handle is a pointer to libusb_device_handle
    using netmd_dev_handle = libusb_device_handle*;

    /// NetMD device
    struct NetMDDevice
    {
        SKnownDevice mKnownDev;         ///< known device info
        std::string mName;              ///< name
        netmd_dev_handle mDevHdl;       ///< device handle
    };

    /// map of known devices
    using KnownDevices     = std::map<uint32_t, SKnownDevice>;

    /// map with known / supported NetMD devices
    static const KnownDevices smKnownDevices;

    /// make friends
    friend CNetMdApi;

    /// 1000ms
    static constexpr unsigned int NETMD_POLL_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_SEND_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TRIES   =   30;
    static constexpr unsigned int NETMD_SYNC_TRIES   =    5;

    /// NetMD status
    enum NetMdStatus
    {
        NETMD_STATUS_NOT_IMPLEMENTED = 0x08,
        NETMD_STATUS_ACCEPTED        = 0x09,
        NETMD_STATUS_REJECTED        = 0x0a,
        NETMD_STATUS_IN_TRANSITION   = 0x0b,
        NETMD_STATUS_IMPLEMENTED     = 0x0c,
        NETMD_STATUS_CHANGED         = 0x0d,
        NETMD_STATUS_INTERIM         = 0x0f,
    };

    /// descriptor types
    enum class Descriptor : uint8_t
    {
        discTitleTD,
        audioUTOC1TD,
        audioUTOC4TD,
        DSITD,
        audioContentsTD,
        rootTD,

        discSubunitIndentifier,
        operatingStatusBlock,
    };

    /// descriptor actions
    enum class DscrtAction : uint8_t
    {
        openread  = 0x01,
        openwrite = 0x03,
        close     = 0x00,
    };


    /// a type for storing descriptor data
    using DscrtData = std::map<Descriptor, NetMDByteVector>;

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //--------------------------------------------------------------------------
    CNetMdDev();

    //--------------------------------------------------------------------------
    //! @brief      Destroys the object.
    //--------------------------------------------------------------------------
    ~CNetMdDev();

    //--------------------------------------------------------------------------
    //! @brief      Initializes the device.
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int initDevice();

    //--------------------------------------------------------------------------
    //! @brief      Gets the device name.
    //!
    //! @return     The device name.
    //--------------------------------------------------------------------------
    std::string getDeviceName();

    //--------------------------------------------------------------------------
    //! @brief      create unique id from vendor and device
    //!
    //! @param[in]  v     vendor id
    //! @param[in]  d     device id
    //!
    //! @return     uint32_t containing vendor and device
    //--------------------------------------------------------------------------
    static inline uint32_t vendorDev(uint16_t v, uint16_t d)
    {
        return static_cast<uint32_t>(v) << 16 | static_cast<uint32_t>(d);
    }


    //--------------------------------------------------------------------------
    //! @brief      polls to see if minidisc wants to send data
    //!
    //! @param      buf    The poll buffer buffer
    //! @param[in]  tries  The number of tries
    //!
    //! @return     < 0 -> NetMdErr; else number of bytes device wants to send
    //--------------------------------------------------------------------------
    int poll(uint8_t buf[4], int tries);

    //--------------------------------------------------------------------------
    //! @brief      Sends a standard command.
    //!
    //! @param      cmd      The new value
    //! @param[in]  cmdLen   The command length
    //! @param[in]  factory  if true, use factory mode
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sendCmd(unsigned char* cmd, size_t cmdLen, bool factory);

    //--------------------------------------------------------------------------
    //! @brief      Gets the response.
    //!
    //! @param      response  The response
    //!
    //! @return     The response size or NetMdErr.
    //--------------------------------------------------------------------------
    int getResponse(NetMDResp& response);

    //--------------------------------------------------------------------------
    //! @brief      excahnge data with NetMD device
    //!
    //! @param      cmd       The command
    //! @param[in]  cmdLen    The command length
    //! @param      response  The response
    //! @param[in]  factory   if true, use factory mode
    //!
    //! @return     The response size or NetMdErr.
    //--------------------------------------------------------------------------
    int exchange(unsigned char* cmd, size_t cmdLen, NetMDResp* response = nullptr, bool factory = false);

    //--------------------------------------------------------------------------
    //! @brief      wait for the device respond to a command
    //!
    //! @return     0 -> not synced; > 0 -> synced
    //--------------------------------------------------------------------------
    int waitForSync();

    //--------------------------------------------------------------------------
    //! @brief      aquire device (needed for Sharp)
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int aquireDev();

    //--------------------------------------------------------------------------
    //! @brief      Releases a device  (needed for Sharp)
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int releaseDev();

    //--------------------------------------------------------------------------
    //! @brief      change descriptor state
    //!
    //! @param[in]  d     descriptor
    //! @param[in]  a     action
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int changeDscrtState(Descriptor d, DscrtAction a);

    /// init marker
    bool mInitialized = false;

    /// NetMD device
    NetMDDevice mDevice = {{0, 0, nullptr, false}, "", nullptr};

    /// descriptor data
    static const DscrtData smDescrData;
};

} // /namespace netmd