/*
 * CNetMdDev.cpp
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

#include "CNetMdDev.hpp"
#include "log.h"
#include "netmd_defines.h"
#include "netmd_utils.h"
#include <libusb-1.0/libusb.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace netmd {

#define mkDevEntry(a_, b_, c_, d_, e_, f_) CNetMdDev::vendorDev(a_, b_), {a_, b_, c_, d_, e_, f_}

/// map with known / supported NetMD devices
const CNetMdDev::KnownDevices CNetMdDev::smKnownDevices = {
    // Sony / Aiwa
    { mkDevEntry(0x054c, 0x0034, "Sony PCLK-XX"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x0036, "Sony NetMD Walkman"              , false, false, false) },
    { mkDevEntry(0x054c, 0x006F, "Sony NW-E7"                      , false, false, false) },
    { mkDevEntry(0x054c, 0x0075, "Sony MZ-N1"                      , false, true , false) },
    { mkDevEntry(0x054c, 0x007c, "Sony NetMD Walkman"              , false, false, false) },
    { mkDevEntry(0x054c, 0x0080, "Sony LAM-1"                      , false, false, false) },
    { mkDevEntry(0x054c, 0x0081, "Sony MDS-JB980/MDS-NT1/MDS-JE780", true , false, true ) },
    { mkDevEntry(0x054c, 0x0084, "Sony MZ-N505"                    , false, true , false) },
    { mkDevEntry(0x054c, 0x0085, "Sony MZ-S1"                      , false, true , false) },
    { mkDevEntry(0x054c, 0x0086, "Sony MZ-N707"                    , false, true , false) },
    { mkDevEntry(0x054c, 0x008e, "Sony CMT-C7NT"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x0097, "Sony PCGA-MDN1"                  , false, false, false) },
    { mkDevEntry(0x054c, 0x00ad, "Sony CMT-L7HD"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x00c6, "Sony MZ-N10"                     , false, true , false) },
    { mkDevEntry(0x054c, 0x00c7, "Sony MZ-N910"                    , false, true , false) },
    { mkDevEntry(0x054c, 0x00c8, "Sony MZ-N710/NE810/NF810"        , false, true , false) },
    { mkDevEntry(0x054c, 0x00c9, "Sony MZ-N510/NF610"              , false, true , false) },
    { mkDevEntry(0x054c, 0x00ca, "Sony MZ-NE410/DN430/NF520"       , false, true , false) },
    { mkDevEntry(0x054c, 0x00e7, "Sony CMT-M333NT/M373NT"          , false, false, false) },
    { mkDevEntry(0x054c, 0x00eb, "Sony MZ-NE810/NE910"             , false, true , false) },
    { mkDevEntry(0x054c, 0x0101, "Sony LAM"                        , false, false, false) },
    { mkDevEntry(0x054c, 0x0113, "Aiwa AM-NX1"                     , false, true , false) },
    { mkDevEntry(0x054c, 0x011a, "Sony CMT-SE7"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x0119, "Sony CMT-SE9"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x013f, "Sony MDS-S500"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x0148, "Sony MDS-A1"                     , false, false, false) },
    { mkDevEntry(0x054c, 0x014c, "Aiwa AM-NX9"                     , false, true , false) },
    { mkDevEntry(0x054c, 0x017e, "Sony MZ-NH1"                     , false, false, false) },
    { mkDevEntry(0x054c, 0x0180, "Sony MZ-NH3D"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x0182, "Sony MZ-NH900"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x0184, "Sony MZ-NH700/800"               , false, false, false) },
    { mkDevEntry(0x054c, 0x0186, "Sony MZ-NH600"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x0187, "Sony MZ-NH600D"                  , false, false, false) },
    { mkDevEntry(0x054c, 0x0188, "Sony MZ-N920"                    , false, true , false) },
    { mkDevEntry(0x054c, 0x018a, "Sony LAM-3"                      , false, false, false) },
    { mkDevEntry(0x054c, 0x01e9, "Sony MZ-DH10P"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x0219, "Sony MZ-RH10"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x021b, "Sony MZ-RH910"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x021d, "Sony CMT-AH10"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x022c, "Sony CMT-AH10"                   , false, false, false) },
    { mkDevEntry(0x054c, 0x023c, "Sony DS-HMD1"                    , false, false, false) },
    { mkDevEntry(0x054c, 0x0286, "Sony MZ-RH1"                     , false, false, false) },
    
    // Sharp
    { mkDevEntry(0x04dd, 0x7202, "Sharp IM-MT880H/MT899H"          , false, false, false) },
    { mkDevEntry(0x04dd, 0x9013, "Sharp IM-DR400/DR410"            , true , false, true ) },
    { mkDevEntry(0x04dd, 0x9014, "Sharp IM-DR80/DR420/DR580"       , true , false, false) },
    
    // Panasonic
    { mkDevEntry(0x04da, 0x23b3, "Panasonic SJ-MR250"              , false, false, true ) },
    { mkDevEntry(0x04da, 0x23b6, "Panasonic SJ-MR270"              , false, false, true ) },
    
    // Kenwood
    { mkDevEntry(0x0b28, 0x1004, "Kenwood MDX-J9"                  , false, false, false) },
};

#undef mkDevEntry

/// descriptor data
const CNetMdDev::DscrtData CNetMdDev::smDescrData = {
    {CNetMdDev::Descriptor::discTitleTD           , {0x10, 0x18, 0x01}},
    {CNetMdDev::Descriptor::audioUTOC1TD          , {0x10, 0x18, 0x02}},
    {CNetMdDev::Descriptor::audioUTOC4TD          , {0x10, 0x18, 0x03}},
    {CNetMdDev::Descriptor::DSITD                 , {0x10, 0x18, 0x04}},
    {CNetMdDev::Descriptor::audioContentsTD       , {0x10, 0x10, 0x01}},
    {CNetMdDev::Descriptor::rootTD                , {0x10, 0x10, 0x00}},
    {CNetMdDev::Descriptor::discSubunitIndentifier, {0x00}            },
    {CNetMdDev::Descriptor::operatingStatusBlock  , {0x80, 0x00}      },
};

const CNetMdDev::NetMDDevice CNetMdDev::UNINIT_DEV = {SKnownDevice{0, 0, nullptr, false, false, false}, "", nullptr, nullptr, SDI_UNKNOWN, false};

//--------------------------------------------------------------------------
//! @brief      print helper for SonyDevInfo
//!
//! @param[in, out] os ref. to ostream
//! @param[in]      dinfo device info
//!
//! @returns       ref. to os
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const CNetMdDev::SonyDevInfo& dinfo)
{
    switch(dinfo)
    {
        case CNetMdDev::SonyDevInfo::SDI_R1000     : os << "SDI_R1000"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_R1100     : os << "SDI_R1100"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_R1200     : os << "SDI_R1200"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_R1300     : os << "SDI_R1300"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_R1400     : os << "SDI_R1400"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1000     : os << "SDI_S1000"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1100     : os << "SDI_S1100"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1200     : os << "SDI_S1200"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1300     : os << "SDI_S1300"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1400     : os << "SDI_S1400"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1500     : os << "SDI_S1500"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_S1600     : os << "SDI_S1600"     ; break;
        case CNetMdDev::SonyDevInfo::SDI_NO_SUPPORT: os << "SDI_NO_SUPPORT"; break;
        case CNetMdDev::SonyDevInfo::SDI_UNKNOWN   : os << "SDI_UNKNOWN"   ; break;
        default                                    : os << "SDI_UNKNOWN"   ; break;
    }
    return os;
}

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//--------------------------------------------------------------------------
CNetMdDev::CNetMdDev()
    :mhdHPAdd(-1), mhdHPRmv(-1), 
    mDevApiCallback(nullptr), mDoPoll(false), mbHotPlug(false)
{
    mInitialized = libusb_init(NULL) == 0;
    mLOG(INFO) << "Init: " << mInitialized;
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdDev::~CNetMdDev()
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (hotplugSupported())
    {
        libusb_hotplug_deregister_callback(NULL, mhdHPAdd);
        libusb_hotplug_deregister_callback(NULL, mhdHPRmv);
    }
    else
    {
        if (mPollThread.joinable())
        {
            mDoPoll = false;
            mPollThread.join();
        }
    }

    if (mDevice.mDevHdl != nullptr)
    {
        if (libusb_release_interface(mDevice.mDevHdl, 0) == 0)
        {
            libusb_close(mDevice.mDevHdl);
            mDevice.mDevHdl = nullptr;
        }
    }

    if (mInitialized)
    {
        libusb_exit(NULL);
    }
}

//--------------------------------------------------------------------------
//! @brief      init libusb hotplug (native or emulation)
//
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::initHotPlug()
{
    mFLOW(INFO);
    if (mInitialized)
    {
        mbHotPlug = true;

        // get a valid status
        initDevice();

        if (hotplugSupported())
        {
            mLOG(INFO) << "Hotplug supported!";
            libusb_hotplug_register_callback(
                NULL,
                LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                LIBUSB_HOTPLUG_NO_FLAGS,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                &CNetMdDev::deviceAdded,
                static_cast<void*>(this),
                &mhdHPAdd);

            libusb_hotplug_register_callback(
                NULL,
                LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                LIBUSB_HOTPLUG_NO_FLAGS,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                LIBUSB_HOTPLUG_MATCH_ANY,
                &CNetMdDev::deviceRemoved,
                static_cast<void*>(this),
                &mhdHPRmv);
        }
        else
        {
            mLOG(INFO) << "Hotplug emulated!";
            mDoPoll = true;
            mPollThread = std::thread(std::bind(&CNetMdDev::pollThread, this));
        }
    }

    return mInitialized ? NETMDERR_NO_ERROR : NETMDERR_USB;
}

//--------------------------------------------------------------------------
//! @brief      Initializes the device.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::initDevice()
{
    mFLOW(INFO);
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    int ret = NETMDERR_USB;

    if (mInitialized)
    {
        if (mDevice.mDevHdl != nullptr)
        {
            if (mbHotPlug)
            {
                // after being initialized the first time, all other device
                // handling has to go through hotplug
                return NETMDERR_NO_ERROR;
            }
            else
            {
                // in case hotplug wasn't enabled, we do a complete device recognition
                libusb_close(mDevice.mDevHdl);
                mDevice = UNINIT_DEV;
                if (mDevApiCallback)
                {
                    mDevApiCallback(false);
                }
            }
        }

        libusb_device **devs = nullptr;
        libusb_device_descriptor descr;

        ssize_t cnt = libusb_get_device_list(NULL, &devs);

        if (cnt > -1)
        {
            for (ssize_t i = 0; ((i < cnt) && (ret != NETMDERR_NO_ERROR)); i++)
            {
                if (libusb_get_device_descriptor(devs[i], &descr) == 0)
                {
                    ret = openDevice(devs[i], &descr);
                }
            }
        }

        if (devs != nullptr)
        {
            libusb_free_device_list(devs, 1);
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      open a supported NetMD device
//
//! @param[in]  dev        The USB device
//! @param[in]  desc       The device description
//
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::openDevice(libusb_device* dev, libusb_device_descriptor* desc)
{
    mFLOW(INFO);
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);
    int ret = NETMDERR_USB;

    mLOG(DEBUG) << "Checking device: " << std::hex << desc->idVendor << ":" << desc->idProduct 
        << ", device class: " << std::dec << static_cast<int>(desc->bDeviceClass);

    if (mDevice.mDevHdl != nullptr)
    {
        mLOG(DEBUG) << "A NetMd device is already in use!";
        return NETMDERR_NOTREADY;
    }

    KnownDevices::const_iterator cit;

    if ((cit = smKnownDevices.find(vendorDev(desc->idVendor, desc->idProduct))) != smKnownDevices.cend())
    {
        mLOG(DEBUG) << "Found supported device: " << cit->second.mModel << " @ " << dev;
        mDevice.mKnownDev = cit->second;

        int cycle    = 5;
        bool success = false;
        do
        {
            if (libusb_open(dev, &mDevice.mDevHdl) == 0)
            {
                if ((ret = libusb_reset_device(mDevice.mDevHdl)) == 0)
                {
                    if (libusb_claim_interface(mDevice.mDevHdl, 0) == 0)
                    {
                        success = true;
                        ret     = NETMDERR_NO_ERROR;
                        break;
                    }
                }
                else if (ret == LIBUSB_ERROR_NOT_FOUND)
                {
                    mLOG(DEBUG) << "Can't reset " << cit->second.mModel;
                }
            }

            if (!success && mDevice.mDevHdl)
            {
                libusb_close(mDevice.mDevHdl);
                mDevice.mDevHdl = nullptr;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while(!success && cycle--);

        /*
        bool success = false;

        if (libusb_open(dev, &mDevice.mDevHdl) == 0)
        {
            int cycle = 5;
            do
            {
                if (libusb_claim_interface(mDevice.mDevHdl, 0) == 0)
                {
                    success = true;
                    ret     = NETMDERR_NO_ERROR;
                    break;
                }
                else
                {
                    libusb_reset_device(mDevice.mDevHdl);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    mLOG(DEBUG) << "Can't claim interface " << cit->second.mModel;
                }
            }
            while(!success && cycle--);
        }
*/
        if (success)
        {
            mDevice.mDevPtr = dev;
            static_cast<void>(waitForSync());
            static_cast<void>(getStrings(*desc));
            static_cast<void>(sonyDevCode());
            mLOG(INFO) << "Product name: " << mDevice.mName << " @ " << mDevice.mDevPtr;
        }
        else
        {
            if (mDevice.mDevHdl != nullptr)
            {
                libusb_close(mDevice.mDevHdl);
            }
            mDevice = UNINIT_DEV;
            ret = NETMDERR_USB;
            mLOG(CRITICAL) << "Can't init usb device!";
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      static hotplug device left callback
//
//! @param[in]   ctx    pointer to libusb_context
//! @param[in]   device pointer to libusb_device
//! @param[in]   event  event type
//! @param[in]   pointer to user data
//
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::deviceRemoved(libusb_context*, libusb_device *device, libusb_hotplug_event, void *userData)
{
    if (CNetMdDev* pDev = static_cast<CNetMdDev*>(userData))
    {
        std::unique_lock<std::recursive_mutex> lock(pDev->mMtxDevAcc);

        //! @note Since we can't get a description from a removed device,
        //!       we use the device pointer to the libusb_device is 
        //!       the unique key.
        if (pDev->mDevice.mDevHdl && (device == pDev->mDevice.mDevPtr)) 
        {
            mLOG(INFO) << "Device " << pDev->mDevice.mKnownDev.mModel  << " @ " << device << " removed.";
            libusb_close(pDev->mDevice.mDevHdl);
            pDev->mDevice = UNINIT_DEV;
            if (pDev->mDevApiCallback)
            {
                pDev->mDevApiCallback(false);
            }
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      static hotplug device added callback
//
//! @param[in]   ctx    pointer to libusb_context
//! @param[in]   device pointer to libusb_device
//! @param[in]   event  event type
//! @param[in]   pointer to user data
//
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::deviceAdded(libusb_context*, libusb_device *device, libusb_hotplug_event, void *userData)
{
    if (CNetMdDev* pDev = static_cast<CNetMdDev*>(userData))
    {
        std::unique_lock<std::recursive_mutex> lock(pDev->mMtxDevAcc);

        if (!pDev->mDevice.mDevHdl)
        {
            libusb_device_descriptor desc;
            libusb_get_device_descriptor(device, &desc);

            if (pDev->openDevice(device, &desc) == NETMDERR_NO_ERROR) 
            {
                if (pDev->mDevApiCallback)
                {
                    pDev->mDevApiCallback(true);
                }
            }
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief poll for device add / remove (hotplug emulation)
//
//! @return NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::pollThread()
{
    std::map<uint64_t, libusb_device*> lastDevices, currDevices;

    while (mDoPoll)
    {
        libusb_device **devs = nullptr;
        libusb_device_descriptor descr;

        currDevices.clear();

        ssize_t cnt = libusb_get_device_list(NULL, &devs);

        if (cnt > -1)
        {
            for (ssize_t i = 0; i < cnt; i++)
            {
                if (libusb_get_device_descriptor(devs[i], &descr) == 0)
                {
                    uint64_t devId = (static_cast<uint64_t>(descr.idVendor) << 32) | static_cast<uint64_t>(descr.idProduct);
                    currDevices[devId] = devs[i];
                }
            }
        }

        // removed
        for (const auto &[key, val] : lastDevices)
        {
            const auto it = currDevices.find(key);

            if (it == currDevices.cend())
            {
                deviceRemoved(nullptr, val, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, this);
            }
        }

        // added
        for (const auto &[key, val] : currDevices)
        {
            const auto it = lastDevices.find(key);

            if (it == lastDevices.cend())
            {
                deviceAdded(nullptr, val, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, this);
            }
        }

        if (devs != nullptr)
        {
            libusb_free_device_list(devs, 1);
        }

        //! @note  All libusb_device pointers are now invalid. The only exception is
        //!        the pointer we created a netmd device handle from. This stands valid and 
        //!        will be used to check if our NetMD device was removed.

        lastDevices = currDevices;

        if (mDoPoll)
        {
            // wait for 250ms
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Gets the strings from the NetMD device
//!
//! @param[in]  descr  The descriptor
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::getStrings(const libusb_device_descriptor& descr)
{
    mFLOW(DEBUG);
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    uint8_t buff[255] = {0,};
    int ret = NETMDERR_NO_ERROR;
    int sz;

    mDevice.mName   = "";

    if ((sz = libusb_get_string_descriptor_ascii(mDevice.mDevHdl, descr.iProduct, buff, 255)) < 0)
    {
        mLOG(DEBUG) << "Can't read product name: " << libusb_strerror(static_cast<libusb_error>(sz));
        ret = NETMDERR_USB;
    }
    else
    {
        for(int j = 0; j < sz; j++)
        {
            mDevice.mName.push_back(static_cast<char>(buff[j]));
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Gets the device name.
//!
//! @return     The device name.
//--------------------------------------------------------------------------
std::string CNetMdDev::getDeviceName()
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);
    return {mDevice.mKnownDev.mModel == nullptr ? "" : mDevice.mKnownDev.mModel};
}

//--------------------------------------------------------------------------
//! @brief      get response length
//!
//! @param[out] req request
//!
//! @return     < 0 -> NetMdErr; else number of bytes device wants to send
//--------------------------------------------------------------------------
int CNetMdDev::responseLength(uint8_t& req)
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    uint8_t pollbuf[] = {0, 0, 0, 0};
    static constexpr uint8_t REQ_TYPE = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE;
    int ret = 0;

    if ((ret = libusb_control_transfer(mDevice.mDevHdl, REQ_TYPE, 0x01, 0, 0, pollbuf, 4, NETMD_POLL_TIMEOUT * 2)) > 0)
    {
        if (pollbuf[0] != 0)
        {
            req = pollbuf[1];
            return (static_cast<int>(pollbuf[3]) << 8) | static_cast<int>(pollbuf[2]);
        }
    }
    else if (ret < 0)
    {
        mLOG(DEBUG) << "Error while polling for response: " << libusb_strerror(static_cast<libusb_error>(ret));
        return -1;
    }

    return 0;
}

//--------------------------------------------------------------------------
//! @brief      read any garbage which might still be in send queue of
//!             the NetMD device
//--------------------------------------------------------------------------
void CNetMdDev::cleanupRespQueue()
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    uint8_t req = 0;
    int ret = responseLength(req);

    if (ret > 0)
    {
        std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

        NetMDResp response = NetMDResp(new unsigned char[ret]);

        constexpr uint8_t REQ_TYPE = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE;

        // receive data
        if (libusb_control_transfer(mDevice.mDevHdl, REQ_TYPE, req, 0, 0, response.get(), ret, NETMD_RECV_TIMEOUT) > 0)
        {
            mLOG(DEBUG) << "Read garbage: " << LOG::hexFormat(DEBUG, response.get(), ret);
        }
    }
}

//--------------------------------------------------------------------------
//! @brief      Sends a standard command.
//!
//! @param      cmd     The new value
//! @param[in]  cmdLen  The command length
//! @param[in]  factory  if true, use factory mode
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::sendCmd(unsigned char* cmd, size_t cmdLen, bool factory)
{
    mFLOW(DEBUG);
    // send data
    mLOG(DEBUG) << (factory ? "factory " : "") << "command:" << LOG::hexFormat(DEBUG, cmd, cmdLen);

    // read any data still in response queue
    // of the NetMD device
    cleanupRespQueue();

    int ret;

    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if ((ret = libusb_control_transfer(mDevice.mDevHdl,
                                       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                       factory ? 0xff : 0x80, 0, 0, cmd, cmdLen,
                                       NETMD_SEND_TIMEOUT)) < 0)
    {
        mLOG(CRITICAL) << "libusb_control_transfer failed! " << libusb_strerror(static_cast<libusb_error>(ret));
        return NETMDERR_USB;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Gets the response.
//!
//! @param[out] response           The response
//! @param[in]  overrideRespLength override response length
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMdDev::getResponse(NetMDResp& response, int overrideRespLength)
{
    // poll for data that minidisc wants to send
    uint8_t req = 0;
    int ret;
    uint32_t i = 0;
    int tmOut = NETMD_RECV_TIMEOUT;

    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (overrideRespLength != -1)
    {
        ret   = overrideRespLength;
        req   = 0x81; // 0xff for factory command
        tmOut = 20'000;
    }
    else
    {
        while ((ret = responseLength(req)) <= 0)
        {
            // a second chance
            if (ret < 0)
            {
                mLOG(DEBUG) << "try again ...";
                return NETMDERR_AGAIN;
            }

            // we shouldn't try forever ...
            if (i == NETMD_RECV_TRIES)
            {
                mLOG(CRITICAL) << "Timeout while waiting for response length!";
                return NETMDERR_TIMEOUT;
            }

            // Double wait time every 10 attempts up to 1 sec
            uint32_t sleep = std::min<uint32_t>(NETMD_REPLY_SZ_INTERVAL_USEC * pow(2, static_cast<uint32_t>(i / 10)),
                                                NETMD_MAX_REPLY_SZ_INTERVAL_USEC);

            if (!(i % 10))
            {
                mLOG(DEBUG) << "still polling ... (" << i << " / " << NETMD_RECV_TRIES << " / " << sleep / 1000 << " ms)";
            }

            std::this_thread::sleep_for(std::chrono::microseconds(sleep));
            i++;
        }
    }

    response = NetMDResp(new unsigned char[ret]);

    // receive data
    if ((ret = libusb_control_transfer(mDevice.mDevHdl,
                                       LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                       req, 0, 0, response.get(), ret,
                                       tmOut)) < 0)
    {
        mLOG(CRITICAL) << "libusb_control_transfer failed! " << libusb_strerror(static_cast<libusb_error>(ret));
        return NETMDERR_USB;
    }

    mLOG(DEBUG)  << "Response: 0x" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(response[0]) << " / " << static_cast<NetMdStatus>(response[0])
                 << std::dec << LOG::hexFormat(DEBUG, response.get(), ret);

    // return length
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      exchange data with NetMD device
//!
//! @param[in]  cmd                The command
//! @param[in]  cmdLen             The command length
//! @param[out] response           The response pointer (optional)
//! @param[in]  factory            if true, use factory mode (optional)
//! @param[in]  expected           The expected status (optional)
//! @param[in]  overrideRespLength override response length (optional)
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMdDev::exchange(unsigned char* cmd, size_t cmdLen, NetMDResp* response,
                        bool factory, NetMdStatus expected, int overrideRespLength)
{
    mFLOW(DEBUG);
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    int ret = 0;
    NetMDResp tmpRsp;
    NetMDResp* pResp = (response == nullptr) ? &tmpRsp : response;
    int redo = 2;

    do
    {
        if ((ret = sendCmd(cmd, cmdLen, factory)) == NETMDERR_NO_ERROR)
        {
            if ((ret = getResponse(*pResp, overrideRespLength)) == NETMDERR_AGAIN)
            {
                redo --;
                continue;
            }

            if (*pResp == nullptr)
            {
                ret = NETMDERR_CMD_FAILED;
            }
            else if (((*pResp)[0] == NETMD_STATUS_INTERIM) && (expected != NETMD_STATUS_INTERIM))
            {
                mLOG(DEBUG) << "Re-read ...!";
                (*pResp) = nullptr;
                ret = getResponse(*pResp, overrideRespLength);

                if (*pResp == nullptr)
                {
                    ret = NETMDERR_USB;
                }
            }
            else if (((*pResp)[0] == NETMD_STATUS_INTERIM) && (expected == NETMD_STATUS_INTERIM))
            {
                mLOG(DEBUG) << "Expected INTERIM return value: 0x" << std::hex << std::setw(2)
                               << std::setfill('0') << static_cast<int>((*pResp)[0]) << std::dec;
            }
            else if (((*pResp)[0] == NETMD_STATUS_NOT_IMPLEMENTED) && (expected == NETMD_STATUS_NOT_IMPLEMENTED))
            {
                mLOG(DEBUG) << "Expected status 'NOT IMPLEMENTED' return value: 0x" << std::hex << std::setw(2)
                               << std::setfill('0') << static_cast<int>((*pResp)[0]) << std::dec;
            }
            else if ((*pResp)[0] != NETMD_STATUS_ACCEPTED)
            {
                ret = NETMDERR_CMD_FAILED;
            }
        }
        redo = 0;
    }
    while(redo > 0);

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      do a bulk transfer
//!
//! @param      cmd      The command bytes
//! @param[in]  cmdLen   The command length
//! @param[in]  timeOut  The time out
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMdDev::bulkTransfer(unsigned char* cmd, size_t cmdLen, int timeOut)
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    int bytesDone = 0, sent, err = 0;

    do
    {
        sent = 0;
        err  = libusb_bulk_transfer(mDevice.mDevHdl,
                                    LIBUSB_RECIPIENT_ENDPOINT,
                                    cmd + bytesDone, cmdLen - bytesDone,
                                    &sent, timeOut);

        bytesDone += sent;

        if (bytesDone < static_cast<int>(cmdLen))
        {
            if ((err != LIBUSB_ERROR_INTERRUPTED) && (err != LIBUSB_SUCCESS))
            {
                mLOG(CRITICAL) << "USB transfer error while transffering "
                               << cmdLen << " bytes: " << libusb_strerror(static_cast<libusb_error>(err));
                return NETMDERR_USB;
            }
        }
    }
    while(bytesDone < static_cast<int>(cmdLen));

    return bytesDone;
}

//--------------------------------------------------------------------------
//! @brief      wait for the device respond to a command
//!
//! @return     0 -> not synced; > 0 -> synced
//--------------------------------------------------------------------------
int CNetMdDev::waitForSync()
{
    mFLOW(DEBUG);
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    unsigned char syncmsg[4];
    int tries = NETMD_SYNC_TRIES;
    int ret;
    bool success = false;

    do
    {
        ret = libusb_control_transfer(mDevice.mDevHdl,
                                      LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                      0x01, 0, 0,
                                      syncmsg, 0x04,
                                      NETMD_POLL_TIMEOUT * 5);
        tries --;
        if (ret < 0)
        {
            mLOG(DEBUG) << "libusb_control_transfer failed! " << libusb_strerror(static_cast<libusb_error>(ret));
        }
        else if (ret != 4)
        {
            mLOG(DEBUG) << "control transfer returned " << ret << " bytes instead of the expected 4!";
        }
        else if (memcmp(syncmsg, "\0\0\0\0", 4) == 0)
        {
            // When the device returns 00 00 00 00 we are done.
            success = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    while (tries);

    if (!success)
    {
        mLOG(WARN) << "no sync response from device!";
    }
    else
    {
        mLOG(DEBUG) << "device successfully synced!";
    }

    return success ? 1 : 0;
}

//--------------------------------------------------------------------------
//! @brief      aquire device (needed for Sharp)
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::aquireDev()
{
    mFLOW(DEBUG);
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (exchange(request, sizeof(request)) > 0)
    {
        return NETMDERR_NO_ERROR;
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      Releases a device  (needed for Sharp)
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::releaseDev()
{
    mFLOW(DEBUG);
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);
    
    if (exchange(request, sizeof(request)) > 0)
    {
        return NETMDERR_NO_ERROR;
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      change descriptor state
//!
//! @param[in]  d     descriptor
//! @param[in]  a     action
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::changeDscrtState(Descriptor d, DscrtAction a)
{
    mFLOW(DEBUG);
    int ret = NETMDERR_OTHER;
    DscrtData::const_iterator cit = smDescrData.find(d);

    if (cit != smDescrData.cend())
    {
        NetMDResp query;
        if (((ret = formatQuery("00 1808 %* %b 00", {{cit->second}, {mBYTE(a)}}, query)) > 0)
            && (query != nullptr))
        {
            std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

            if ((ret = exchange(query.get(), ret)) > 0)
            {
                ret = NETMDERR_NO_ERROR;
            }
        }
    }
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      check for this device might be patchable
//!
//! @return     true if so
//--------------------------------------------------------------------------
bool CNetMdDev::isMaybePatchable()
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);
    return mDevice.mKnownDev.mPatchAble;
}

//--------------------------------------------------------------------------
//! @brief      Reads metadata peripheral.
//!
//! @param[in]  sector  The sector
//! @param[in]  offset  The offset
//! @param[in]  length  The length
//!
//! @return     NetMDByteVector
//--------------------------------------------------------------------------
NetMDByteVector CNetMdDev::readMetadataPeripheral(uint16_t sector, uint16_t offset, uint8_t length)
{
    int ret;
    NetMDResp query, resp;
    NetMDParams params;

    // mLOG(INFO) << "Read sector " << sector << ", offset " << offset << ", lebgth: " << static_cast<int>(length);

    if ((ret = formatQuery("00 1824 ff %<w %<w %b 00", {{sector}, {offset}, {length}}, query)) == 10)
    {
        if ((ret = exchange(query.get(), ret, &resp, true)) > 8)
        {
            if (scanQuery(resp.get(), ret, "%? 1824 00 %?%?%?%? %? %*", params) == NETMDERR_NO_ERROR)
            {
                if ((params.size() == 1) && (params.at(0).index() == BYTE_VECTOR))
                {
                    return simple_get<NetMDByteVector>(params.at(0));
                }
            }
        }
    }
    return NetMDByteVector{};
}

//--------------------------------------------------------------------------
//! @brief      Writes metadata peripheral.
//!
//! @param[in]  sector  The sector
//! @param[in]  offset  The offset
//! @param[in]  data    The data
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::writeMetadataPeripheral(uint16_t sector, uint16_t offset, const NetMDByteVector& data)
{
    int ret;
    NetMDResp query;
    if ((ret = formatQuery("00 1825 ff %<w %<w %b %*", {{sector}, {offset}, {mBYTE(data.size())}, {data}}, query)) >= 8)
    {
        ret = exchange(query.get(), ret, nullptr, true);
        return (ret >= 0) ? NETMDERR_NO_ERROR : ret;
    }
    return NETMDERR_PARAM;
}

//------------------------------------------------------------------------------
//! @brief      open for read, read, close
//!
//! @param[in]  addr     address
//! @param[in]  sz       size of data to read
//! @param[out] data     read data
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdDev::cleanRead(uint32_t addr, uint8_t sz, NetMDByteVector& data)
{
    mFLOW(DEBUG);
    int ret;
    static_cast<void>(changeMemState(addr, sz, MemAcc::NETMD_MEM_READ));
    ret = patchRead(addr, sz, data);
    static_cast<void>(changeMemState(addr, sz, MemAcc::NETMD_MEM_CLOSE));
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      open for write, write, close
//!
//! @param[in]  addr     address
//! @param[in]  data     data to write
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdDev::cleanWrite(uint32_t addr, const NetMDByteVector& data)
{
    mFLOW(DEBUG);
    int ret;
    static_cast<void>(changeMemState(addr, data.size(), MemAcc::NETMD_MEM_WRITE));
    ret = patchWrite(addr, data);
    static_cast<void>(changeMemState(addr, data.size(), MemAcc::NETMD_MEM_CLOSE));
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      write patch data
//!
//! @param[in]  addr      address
//! @param[in]  data      data to write
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdDev::patchWrite(uint32_t addr, const NetMDByteVector& data)
{
    mFLOW(DEBUG);
    int ret;
    const char* format = "00 1822 ff 00 %<d %b 0000 %* %<w";

    NetMDResp query;

    if (((ret = formatQuery(format, {{addr}, {mBYTE(data.size())}, {data}, calculateChecksum(data)}, query)) >= 15)
        && (query != nullptr))
    {
        if ((ret = exchange(query.get(), ret, nullptr, true)) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      read patch data
//!
//! @param[in]  addr      address
//! @param[in]  size      size to read
//! @param[in]  data      read data
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdDev::patchRead(uint32_t addr, uint8_t size, NetMDByteVector& data)
{
    mFLOW(DEBUG);
    int ret;
    const char* format  = "00 1821 ff 00 %<d %b";
    const char* capture = "%? 1821 00 %? %?%?%?%? %? %?%? %*";

    NetMDResp query, response;
    NetMDParams params;

    if (((ret = formatQuery(format, {{addr}, {size}}, query)) == 10) && (query != nullptr))
    {
        if (((ret = exchange(query.get(), ret, &response, true)) > 0) && (response != nullptr))
        {
            if ((ret = scanQuery(response.get(), ret, capture, params)) == NETMDERR_NO_ERROR)
            {
                if (params.at(0).index() == BYTE_VECTOR)
                {
                    data = simple_get<NetMDByteVector>(params.at(0));

                    // ignore last two bytes (checksum)
                    data.pop_back();
                    data.pop_back();

                    ret = NETMDERR_NO_ERROR;
                }
            }
        }
    }

    if (ret != NETMDERR_NO_ERROR)
    {
        ret = NETMDERR_OTHER;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      change memory access state
//!
//! @param[in]  addr  The address
//! @param[in]  size  The size
//! @param[in]  acc   The memory access acc
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::changeMemState(uint32_t addr, uint8_t size, MemAcc acc)
{
    mFLOW(DEBUG);
    int ret;
    const char* format = "00 1820 ff 00 %<d %b %b 00";

    NetMDResp query;

    if (((ret = formatQuery(format, {{addr}, {size}, {mBYTE(acc)}}, query)) == 12)
        && (query != nullptr))
    {
        if ((ret = exchange(query.get(), ret, nullptr, true)) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }
    }

    if (ret != NETMDERR_NO_ERROR)
    {
        ret = NETMDERR_OTHER;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get Sony device code
//!
//! @return     SonyDevInfo
//! @see        SonyDevInfo
//--------------------------------------------------------------------------
CNetMdDev::SonyDevInfo CNetMdDev::sonyDevCode()
{
    std::unique_lock<std::recursive_mutex> lock(mMtxDevAcc);

    if (mDevice.mDevInfo == SDI_UNKNOWN)
    {
        if (!isMaybePatchable())
        {
            mDevice.mDevInfo = SDI_NO_SUPPORT;
        }
        else
        {
            mFLOW(INFO);
            uint8_t query[] = {0x00, 0x18, 0x12, 0xff};
            uint8_t chip    = 255, hwid = 255, version = 255, subversion = 255;

            std::ostringstream code;

            NetMDResp respone;
            int       rpSz   = 0;
            int       chkCnt = 0;

            // in case we don't get extended device info
            // try to enable factory mode
            do
            {
                rpSz = exchange(query, sizeof(query), &respone, true);
                if ((rpSz < 8) || (respone == nullptr))
                {
                    enableFactory(mDevice.mFactoryMode);
                }
            }
            while ((++chkCnt < 1) && ((rpSz < 8) || (respone == nullptr)));

            if ((rpSz >= 8) && (respone != nullptr))
            {
                mDevice.mFactoryMode = true;

                chip       = respone[4];
                hwid       = respone[5];
                subversion = respone[6];
                version    = respone[7];

                if ((chip != 255) || (hwid != 255) || (version != 255) || (subversion != 255))
                {
                    switch (chip)
                    {
                    case 0x20:
                        code << "R";
                        break;
                    case 0x21:
                        code << "S";
                        break;
                    case 0x22:
                        code << "Hn";
                        break;
                    case 0x24:
                        code << "Hr";
                        break;
                    case 0x25:
                        code << "Hx";
                        break;
                    default:
                        code << "0x" << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(chip) << std::dec;
                        break;
                    }

                    code << static_cast<int>(version >> 4) << "." << static_cast<int>(version & 0xf)
                        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(subversion);

                    mLOG(INFO) << "Found device info: " << code.str();

                    if (code.str() == "R1.000")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_R1000;
                    }
                    else if (code.str() == "R1.100")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_R1100;
                    }
                    else if (code.str() == "R1.200")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_R1200;
                    }
                    else if (code.str() == "R1.300")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_R1300;
                    }
                    else if (code.str() == "R1.400")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_R1400;
                    }
                    else if (code.str() == "S1.000")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1000;
                    }
                    else if (code.str() == "S1.100")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1100;
                    }
                    else if (code.str() == "S1.200")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1200;
                    }
                    else if (code.str() == "S1.300")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1300;
                    }
                    else if (code.str() == "S1.400")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1400;
                    }
                    else if (code.str() == "S1.500")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1500;
                    }
                    else if (code.str() == "S1.600")
                    {
                        mDevice.mDevInfo = SonyDevInfo::SDI_S1600;
                    }
                }
            }
        }
    }

    return mDevice.mDevInfo;
}

//--------------------------------------------------------------------------
//! @brief      Enables the factory mode.
//
//! @param[in,out]  marker   get / store factory state
//
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::enableFactory(bool& marker)
{
    mFLOW(INFO);
    int ret = NETMDERR_NO_ERROR;

    if (!marker)
    {
        mLOG(DEBUG) << "enable factory ...";
        uint8_t p1[]  = {0x00, 0x18, 0x09, 0x00, 0xff, 0x00, 0x00, 0x00,
                        0x00, 0x00};

        uint8_t p2[]  = {0x00, 0x18, 0x01, 0xff, 0x0e, 0x4e, 0x65, 0x74,
                        0x20, 0x4d, 0x44, 0x20, 0x57, 0x61, 0x6c, 0x6b,
                        0x6d, 0x61, 0x6e};

        if (changeDscrtState(Descriptor::discSubunitIndentifier,
                            DscrtAction::openread) != NETMDERR_NO_ERROR)
        {
            ret = NETMDERR_USB;
        }

        if (exchange(p1, sizeof(p1)) <= 0)
        {
            ret = NETMDERR_USB;
        }

        if (exchange(p2, sizeof(p2), nullptr, true) <= 0)
        {
            ret = NETMDERR_USB;
        }

        if (ret == NETMDERR_NO_ERROR)
        {
            marker = true;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      register hotplug callback function
//
//! @param[in]  cb  callback function to e called on device add / removal
//!                 if is nullptr, the callback will be removed
//--------------------------------------------------------------------------
void CNetMdDev::registerDeviceCallback(EvtCallback cb)
{
    mDevApiCallback = cb;
}

//--------------------------------------------------------------------------
//! @brief      check if hotplug is supported
//
//! @return     true if so; false otherwise
//--------------------------------------------------------------------------
bool CNetMdDev::hotplugSupported() const
{
    return !!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
}

} // namespace netmd
