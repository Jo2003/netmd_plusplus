/*
 * CNetMdDev.cpp
 *
 * This file is part of netmd++, a library for accessing NetMD devices.
 *
 * It makes use of knowledge / code collected by Marc Britten and
 * Alexander Sulfrian for the Linux Minidisc project.
 *
 * Asivery helped to make this possible!
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

namespace netmd {

#define mkDevEntry(a_, b_, c_, d_) CNetMdDev::vendorDev(a_, b_), {a_, b_, c_, d_}

/// map with known / supported NetMD devices
const CNetMdDev::KnownDevices CNetMdDev::smKnownDevices = {
    { mkDevEntry(0x054c, 0x0034, "Sony PCLK-XX"                                  , false) },
    { mkDevEntry(0x054c, 0x0036, "Sony (unknown model)"                          , false) },
    { mkDevEntry(0x054c, 0x006F, "Sony NW-E7"                                    , false) },
    { mkDevEntry(0x054c, 0x0075, "Sony MZ-N1"                                    , false) },
    { mkDevEntry(0x054c, 0x007c, "Sony (unknown model)"                          , false) },
    { mkDevEntry(0x054c, 0x0080, "Sony LAM-1"                                    , false) },
    { mkDevEntry(0x054c, 0x0081, "Sony MDS-JE780/JB980"                          , true ) },
    { mkDevEntry(0x054c, 0x0084, "Sony MZ-N505"                                  , false) },
    { mkDevEntry(0x054c, 0x0085, "Sony MZ-S1"                                    , false) },
    { mkDevEntry(0x054c, 0x0086, "Sony MZ-N707"                                  , false) },
    { mkDevEntry(0x054c, 0x008e, "Sony CMT-C7NT"                                 , false) },
    { mkDevEntry(0x054c, 0x0097, "Sony PCGA-MDN1"                                , false) },
    { mkDevEntry(0x054c, 0x00ad, "Sony CMT-L7HD"                                 , false) },
    { mkDevEntry(0x054c, 0x00c6, "Sony MZ-N10"                                   , false) },
    { mkDevEntry(0x054c, 0x00c7, "Sony MZ-N910"                                  , false) },
    { mkDevEntry(0x054c, 0x00c8, "Sony MZ-N710/NE810/NF810"                      , false) },
    { mkDevEntry(0x054c, 0x00c9, "Sony MZ-N510/NF610"                            , false) },
    { mkDevEntry(0x054c, 0x00ca, "Sony MZ-NE410/DN430/NF520"                     , false) },
    { mkDevEntry(0x054c, 0x00eb, "Sony MZ-NE810/NE910"                           , false) },
    { mkDevEntry(0x054c, 0x00e7, "Sony CMT-M333NT/M373NT"                        , false) },
    { mkDevEntry(0x054c, 0x0101, "Sony LAM-10"                                   , false) },
    { mkDevEntry(0x054c, 0x0113, "Aiwa AM-NX1"                                   , false) },
    { mkDevEntry(0x054c, 0x0119, "Sony CMT-SE9"                                  , false) },
    { mkDevEntry(0x054c, 0x013f, "Sony MDS-S500"                                 , false) },
    { mkDevEntry(0x054c, 0x014c, "Aiwa AM-NX9"                                   , false) },
    { mkDevEntry(0x054c, 0x017e, "Sony MZ-NH1"                                   , false) },
    { mkDevEntry(0x054c, 0x0180, "Sony MZ-NH3D"                                  , false) },
    { mkDevEntry(0x054c, 0x0182, "Sony MZ-NH900"                                 , false) },
    { mkDevEntry(0x054c, 0x0184, "Sony MZ-NH700/800"                             , false) },
    { mkDevEntry(0x054c, 0x0186, "Sony MZ-NH600/600D"                            , false) },
    { mkDevEntry(0x054c, 0x0188, "Sony MZ-N920"                                  , false) },
    { mkDevEntry(0x054c, 0x018a, "Sony LAM-3"                                    , false) },
    { mkDevEntry(0x054c, 0x01e9, "Sony MZ-DH10P"                                 , false) },
    { mkDevEntry(0x054c, 0x0219, "Sony MZ-RH10"                                  , false) },
    { mkDevEntry(0x054c, 0x021b, "Sony MZ-RH910"                                 , false) },
    { mkDevEntry(0x054c, 0x021d, "Sony CMT-AH10"                                 , false) },
    { mkDevEntry(0x054c, 0x022c, "Sony CMT-AH10"                                 , false) },
    { mkDevEntry(0x054c, 0x023c, "Sony DS-HMD1"                                  , false) },
    { mkDevEntry(0x054c, 0x0286, "Sony MZ-RH1"                                   , false) },
    { mkDevEntry(0x04dd, 0x7202, "Sharp IM-MT880H/MT899H"                        , false) },
    { mkDevEntry(0x04dd, 0x9013, "Sharp IM-DR400/DR410"                          , true ) },
    { mkDevEntry(0x04dd, 0x9014, "Sharp IM-DR80/DR420/DR580 or Kenwood DMC-S9NET", false) },
    { mkDevEntry(0x0004, 0x23b3, "Panasonic SJ-MR250"                            , false) },
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

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//--------------------------------------------------------------------------
CNetMdDev::CNetMdDev()
{
    mInitialized = libusb_init(NULL) == 0;
    mLOG(INFO) << "Init: " << mInitialized;
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdDev::~CNetMdDev()
{
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
//! @brief      Initializes the device.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdDev::initDevice()
{
    int ret = NETMDERR_USB;

    if (mInitialized)
    {
        if (mDevice.mDevHdl != nullptr)
        {
            if (libusb_release_interface(mDevice.mDevHdl, 0) == 0)
            {
                libusb_close(mDevice.mDevHdl);
                mDevice.mDevHdl = nullptr;
            }
        }

        libusb_device **devs = nullptr;
        libusb_device_descriptor descr;
        KnownDevices::const_iterator cit;

        ssize_t cnt = libusb_get_device_list(NULL, &devs);

        if (cnt > -1)
        {
            for (ssize_t i = 0; ((i < cnt) && (ret == -1)); i++)
            {
                if (libusb_get_device_descriptor(devs[i], &descr) == 0)
                {
                    if ((cit = smKnownDevices.find(vendorDev(descr.idVendor, descr.idProduct))) != smKnownDevices.cend())
                    {
                        mDevice.mKnownDev = cit->second;

                        if ((libusb_open(devs[i], &mDevice.mDevHdl) == 0) && (libusb_claim_interface(mDevice.mDevHdl, 0) == 0))
                        {
                            ret = NETMDERR_NO_ERROR;
                        }
                    }
                }
            }
        }

        if (devs != nullptr)
        {
            libusb_free_device_list(devs, 1);
        }
    }

    // cleanup anything which might still be in "pipe"
    if (ret == NETMDERR_NO_ERROR)
    {
        static_cast<void>(waitForSync());
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
    return {mDevice.mKnownDev.mModel == nullptr ? "" : mDevice.mKnownDev.mModel};
}

//--------------------------------------------------------------------------
//! @brief      polls to see if minidisc wants to send data
//!
//! @param      buf    The poll buffer buffer
//! @param[in]  tries  The number of tries
//!
//! @return     < 0 -> NetMdErr; else number of bytes device wants to send
//--------------------------------------------------------------------------
int CNetMdDev::poll(uint8_t buf[4], int tries)
{
    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    int sleepytime = 5;

    for (int i = 0; i < tries; i++)
    {
        // send a poll message
        memset(buf, 0, 4);

        if (libusb_control_transfer(mDevice.mDevHdl,
                                    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                    0x01, 0, 0, buf, 4, NETMD_POLL_TIMEOUT) < 0)
        {
            mLOG(CRITICAL) << "libusb_control_transfer failed!";
            return NETMDERR_USB;
        }

        if (buf[0] != 0)
        {
            break;
        }

        if (i > 0)
        {
            usleep(sleepytime * 1000);
            sleepytime = 100;
        }
        if (i > 10)
        {
            sleepytime = 1000;
        }
    }

    return (static_cast<int>(buf[3]) << 8) | static_cast<int>(buf[2]);
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
    unsigned char pollbuf[4];
    int len;

    // poll to see if we can send data
    len = poll(pollbuf, 1);
    if (len != 0)
    {
        mLOG(CRITICAL) << "poll() failed!";
        return (len > 0) ? NETMDERR_NOTREADY : len;
    }

    // send data
    mLOG(INFO) << (factory ? "factory " : "") << "command:" << LOG::hexFormat(INFO, cmd, cmdLen);

    if (libusb_control_transfer(mDevice.mDevHdl,
                                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                factory ? 0xff : 0x80, 0, 0, cmd, cmdLen,
                                NETMD_SEND_TIMEOUT) < 0)
    {
        mLOG(CRITICAL) << "libusb_control_transfer failed!";
        return NETMDERR_USB;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Gets the response.
//!
//! @param[out] response  The response
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMdDev::getResponse(NetMDResp& response)
{
    uint8_t pollbuf[4];

    // poll for data that minidisc wants to send
    int ret = poll(pollbuf, NETMD_RECV_TRIES);
    if (ret <= 0)
    {
        mLOG(CRITICAL) << "poll failed!";
        return (ret == 0) ? NETMDERR_TIMEOUT : ret;
    }

    response = NetMDResp(new unsigned char[ret]);

    // receive data
    if (libusb_control_transfer(mDevice.mDevHdl,
                                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                pollbuf[1], 0, 0, response.get(), ret,
                                NETMD_RECV_TIMEOUT) < 0)
    {
        mLOG(CRITICAL) << "libusb_control_transfer failed!";
        return NETMDERR_USB;
    }

    mLOG(INFO)  << "Response: 0x" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(response[0]) << " / " << static_cast<NetMdStatus>(response[0])
                << std::dec << LOG::hexFormat(DEBUG, response.get(), ret);

    // return length
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      excahnge data with NetMD device
//!
//! @param[in]  cmd       The command
//! @param[in]  cmdLen    The command length
//! @param[out] response  The response pointer (optional)
//! @param[in]  factory   if true, use factory mode (optional)
//! @param[in]  expected  The expected status (optional)
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMdDev::exchange(unsigned char* cmd, size_t cmdLen, NetMDResp* response,
                        bool factory, NetMdStatus expected)
{
    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    int ret = 0;
    NetMDResp tmpRsp;
    NetMDResp* pResp = (response == nullptr) ? &tmpRsp : response;

    if ((ret = sendCmd(cmd, cmdLen, factory)) == NETMDERR_NO_ERROR)
    {
        ret = getResponse(*pResp);

        if (*pResp == nullptr)
        {
            ret = NETMDERR_CMD_FAILED;
        }
        else if (((*pResp)[0] == NETMD_STATUS_INTERIM) && (expected != NETMD_STATUS_INTERIM))
        {
            mLOG(DEBUG) << "Re-read ...!";
            (*pResp) = nullptr;
            ret = getResponse(*pResp);

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
        else if ((*pResp)[0] != NETMD_STATUS_ACCEPTED)
        {
            ret = NETMDERR_CMD_FAILED;
        }
    }

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
                               << cmdLen << " bytes: " << libusb_strerror(err);
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
            mLOG(DEBUG) << "libusb error " << ret << " waiting for control transfer!";
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

        usleep(100'000);
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
    mLOG(DEBUG);
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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
    mLOG(DEBUG);
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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
    mLOG(DEBUG);
    int ret = NETMDERR_OTHER;
    DscrtData::const_iterator cit = smDescrData.find(d);

    if (cit != smDescrData.cend())
    {
        NetMDResp query;
        if (((ret = formatQuery("00 1808 %* %b 00", {{cit->second}, {mBYTE(a)}}, query)) > 0)
            && (query != nullptr))
        {
            if ((ret = exchange(query.get(), ret)) > 0)
            {
                ret = NETMDERR_NO_ERROR;
            }
        }
    }
    return ret;
}

} // namespace netmd