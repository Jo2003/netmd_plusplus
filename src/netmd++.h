/*
 * netmd++.h
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
#include <map>
#include <string>
#include <memory>


#include "log.h"

//------------------------------------------------------------------------------
//! @brief      This class describes a C++ NetMD access library
//------------------------------------------------------------------------------
class CNetMDpp
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

public:

    /// 1000ms
    static constexpr unsigned int NETMD_POLL_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_SEND_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TRIES   =   30;
    static constexpr unsigned int NETMD_SYNC_TRIES   =    5;


    // ease memory management
    using NetMDResp = std::unique_ptr<uint8_t[]>;

    /// NetMD errors
    enum NetMdErr
    {
        NETMDERR_NO_ERROR    =  0,  ///< success
        NETMDERR_USB         = -1,  ///< general USB error
        NETMDERR_NOTREADY    = -2,  ///< player not ready for command
        NETMDERR_TIMEOUT     = -3,  ///< timeout while waiting for response
        NETMDERR_CMD_FAILED  = -4,  ///< minidisc responded with 08 response
        NETMDERR_CMD_INVALID = -5,  ///< minidisc responded with 0A response
    };

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


    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //--------------------------------------------------------------------------
    CNetMDpp();

    //--------------------------------------------------------------------------
    //! @brief      Destroys the object.
    //--------------------------------------------------------------------------
    ~CNetMDpp();

    //--------------------------------------------------------------------------
    //! @brief      Initializes the device.
    //!
    //! @return     0 -> ok; < 0 -> error
    //--------------------------------------------------------------------------
    int initDevice();

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
    //! @brief      Gets the device name.
    //!
    //! @return     The device name.
    //--------------------------------------------------------------------------
    const std::string& getDeviceName();

    //--------------------------------------------------------------------------
    //! @brief      Sets the log level.
    //!
    //! @param[in]  severity  The severity
    //--------------------------------------------------------------------------
    static void setLogLevel(typelog severity);

    //--------------------------------------------------------------------------
    //! @brief      cache table of contents
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int cacheTOC();

    //--------------------------------------------------------------------------
    //! @brief      sync table of contents
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int syncTOC();

    //--------------------------------------------------------------------------
    //! @brief      request track count
    //!
    //! @return     < 0 -> NetMdErr; else -> track count
    //--------------------------------------------------------------------------
    int trackCount();

    //--------------------------------------------------------------------------
    //! @brief      request disc flags
    //!
    //! @return     < 0 -> NetMdErr; else -> flags
    //--------------------------------------------------------------------------
    int discFlags();

    //--------------------------------------------------------------------------
    //! @brief      erase MD
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int eraseDisc();

protected:

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
    //! @brief      Calculates the checksum.
    //!
    //! @param[in]  data     The data
    //! @param[in]  dataLen  The data length
    //!
    //! @return     The checksum.
    //--------------------------------------------------------------------------
    static unsigned int calculateChecksum(unsigned char* data, size_t dataLen);

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

private:
    /// init marker
    bool mInitialized = false;

    /// NetMD device
    NetMDDevice mDevice = {{0, 0, nullptr, false}, "", nullptr};
};