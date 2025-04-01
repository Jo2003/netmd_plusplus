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
    { mkDevEntry(0x04dd, 0x9013, "Sharp IM-DR400/DR410"            , true , false, false) },
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
                        mLOG(DEBUG) << "Found supported device: " << cit->second.mModel;
                        mDevice.mKnownDev = cit->second;
                        bool success = false;
#ifdef __linux__
                        int cycle    = 5;
                        do
                        {
                            if (libusb_open(devs[i], &mDevice.mDevHdl) == 0)
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

                            uwait(100'000);
                        }
                        while(!success && cycle--);

#else // not linux
                        if ((ret = libusb_open(devs[i], &mDevice.mDevHdl)) == 0)
                        {
                            if ((ret = libusb_claim_interface(mDevice.mDevHdl, 0)) == 0)
                            {
                                success = true;
                                ret     = NETMDERR_NO_ERROR;
                            }
                            else
                            {
                                mLOG(CRITICAL) << "Can't claim USB device: " << libusb_strerror(static_cast<libusb_error>(ret));
                                libusb_close(mDevice.mDevHdl);
                                mDevice.mDevHdl = nullptr;
                            }
                        }
                        else
                        {
                            mLOG(CRITICAL) << "Can't open USB device: " << libusb_strerror(static_cast<libusb_error>(ret));
                        }
#endif // __linux__
                        if (success)
                        {
                            static_cast<void>(waitForSync());
                            static_cast<void>(getStrings(descr));
                            mLOG(INFO) << "Product name: " << mDevice.mName << ", serial number: " << mDevice.mSerial;
                        }
                        else
                        {
                            ret = NETMDERR_USB;
                            mLOG(CRITICAL) << "Can't init usb device!";
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

    return ret;
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
    uint8_t buff[255] = {0,};
    int ret = NETMDERR_NO_ERROR;
    int sz;

    mDevice.mName   = "";
    mDevice.mSerial = "";

    int          vals[]  = {descr.iProduct, descr.iSerialNumber};
    std::string* s[]     = {&mDevice.mName, &mDevice.mSerial};
    const char* strErr[] = {"product name", "serial number"};

    for(int i = 0; i < 2; i++)
    {
        if (vals[i] > -1)
        {
            if ((sz = libusb_get_string_descriptor_ascii(mDevice.mDevHdl, vals[i], buff, 255)) < 0)
            {
                mLOG(DEBUG) << "Can't read " << strErr[i] << ": " << libusb_strerror(static_cast<libusb_error>(sz));
                ret = NETMDERR_USB;
            }
            else
            {
                for(int j = 0; j < sz; j++)
                {
                    s[i]->push_back(static_cast<char>(buff[j]));
                }
            }
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Gets the device name.
//!
//! @return     The device name.
//--------------------------------------------------------------------------
std::string CNetMdDev::getDeviceName() const
{
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
    if (mDevice.mDevHdl == nullptr)
    {
        mLOG(CRITICAL) << "No NetMD device available!";
        return NETMDERR_NOTREADY;
    }

    uint8_t pollbuf[] = {0, 0, 0, 0};
    static constexpr uint8_t REQ_TYPE = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE;
    int ret = 0;

    if ((ret = libusb_control_transfer(mDevice.mDevHdl, REQ_TYPE, 0x01, 0, 0, pollbuf, 4, NETMD_POLL_TIMEOUT)) > 0)
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
    uint8_t req = 0;
    int ret = responseLength(req);

    if (ret > 0)
    {
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
    // send data
    mLOG(DEBUG) << (factory ? "factory " : "") << "command:" << LOG::hexFormat(DEBUG, cmd, cmdLen);

    // read any data still in response queue
    // of the NetMD device
    cleanupRespQueue();

    int ret;

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

            uwait(sleep);
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

        uwait(100'000);
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

//--------------------------------------------------------------------------
//! @brief      check for this device might be patchable
//!
//! @return     true if so
//--------------------------------------------------------------------------
bool CNetMdDev::isMaybePatchable() const
{
    return mDevice.mKnownDev.mPatchAble;
}


} // namespace netmd
