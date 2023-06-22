/*
 * CNetMdDev.hpp
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

/// announce Patch class
class CNetMdPatch;

/// announce Secure class
class CNetMdSecure;

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
        bool mPatchAble;        ///< is the device (maybe) patchable?
    };

    /// dev handle is a pointer to libusb_device_handle
    using netmd_dev_handle = libusb_device_handle*;

    /// NetMD device
    struct NetMDDevice
    {
        SKnownDevice mKnownDev;         ///< known device info
        std::string mName;              ///< name
        std::string mSerial;            ///< serial number
        netmd_dev_handle mDevHdl;       ///< device handle
    };

    /// map of known devices
    using KnownDevices     = std::map<uint32_t, SKnownDevice>;

    /// map with known / supported NetMD devices
    static const KnownDevices smKnownDevices;

    /// make friends
    friend CNetMdApi;
    friend CNetMdPatch;
    friend CNetMdSecure;

    /// 1000ms
    static constexpr unsigned int NETMD_POLL_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_SEND_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TIMEOUT = 1000;
    static constexpr unsigned int NETMD_RECV_TRIES   =  100;
    static constexpr unsigned int NETMD_SYNC_TRIES   =    5;
    static constexpr uint8_t    NETMD_STATUS_CONTROL = 0x00;

    /// reply retry interval
    static constexpr unsigned int NETMD_REPLY_SZ_INTERVAL_USEC     =    10'000;
    static constexpr unsigned int NETMD_MAX_REPLY_SZ_INTERVAL_USEC = 1'000'000;

    /// NetMD status
    enum NetMdStatus : uint8_t
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
    //! @brief      helper function to print netmd status return
    //!
    //! @param      os      ostream reference
    //! @param[in]  ntmdst  netmd status
    //!
    //! @return     ostream reference
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const NetMdStatus& ntmdst)
    {
#define mkCase(x_) case x_: os << #x_; break
        switch(ntmdst)
        {
            mkCase(NETMD_STATUS_NOT_IMPLEMENTED);
            mkCase(NETMD_STATUS_ACCEPTED       );
            mkCase(NETMD_STATUS_REJECTED       );
            mkCase(NETMD_STATUS_IN_TRANSITION  );
            mkCase(NETMD_STATUS_IMPLEMENTED    );
            mkCase(NETMD_STATUS_CHANGED        );
            mkCase(NETMD_STATUS_INTERIM        );
        }
        return os;
#undef mkCase
    }

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
    std::string getDeviceName() const;

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
    //! @brief      get response length
    //!
    //! @param[out] req request
    //!
    //! @return     < 0 -> NetMdErr; else number of bytes device wants to send
    //--------------------------------------------------------------------------
    int responseLength(uint8_t& req);

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
    //! @param[out] response  The response
    //!
    //! @return     The response size or NetMdErr.
    //--------------------------------------------------------------------------
    int getResponse(NetMDResp& response);

    //--------------------------------------------------------------------------
    //! @brief      exchange data with NetMD device
    //!
    //! @param[in]  cmd       The command
    //! @param[in]  cmdLen    The command length
    //! @param[out] response  The response pointer (optional)
    //! @param[in]  factory   if true, use factory mode (optional)
    //! @param[in]  expected  The expected status (optional)
    //!
    //! @return     The response size or NetMdErr.
    //--------------------------------------------------------------------------
    int exchange(unsigned char* cmd, size_t cmdLen, NetMDResp* response = nullptr,
                 bool factory = false, NetMdStatus expected =  NETMD_STATUS_ACCEPTED);

    //--------------------------------------------------------------------------
    //! @brief      do a bulk transfer
    //!
    //! @param      cmd      The command bytes
    //! @param[in]  cmdLen   The command length
    //! @param[in]  timeOut  The time out
    //!
    //! @return     The response size or NetMdErr.
    //--------------------------------------------------------------------------
    int bulkTransfer(unsigned char* cmd, size_t cmdLen, int timeOut);

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

    //--------------------------------------------------------------------------
    //! @brief      Gets the strings from the NetMD device
    //!
    //! @param[in]  descr  The descriptor
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int getStrings(const libusb_device_descriptor& descr);

    //--------------------------------------------------------------------------
    //! @brief      check for this device might be patchable
    //!
    //! @return     true if so
    //--------------------------------------------------------------------------
    bool isMaybePatchable() const;

    /// init marker
    bool mInitialized = false;

    /// NetMD device
    NetMDDevice mDevice = {{0, 0, nullptr, false, false}, "", "", nullptr};

    /// descriptor data
    static const DscrtData smDescrData;
};

} // /namespace netmd
