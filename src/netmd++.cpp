/*
 * netmd++.cpp
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

#include "log.h"
#include "netmd++.h"
#include <libusb-1.0/libusb.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>

using namespace netmdutils;

#define mBYTE(x_) static_cast<uint8_t>(x_)
#define mWORD(x_) static_cast<uint16_t>(x_)
#define mDWORD(x_) static_cast<uint32_t>(x_)
#define mQWORD(x_) static_cast<uint64_t>(x_)

#define mkDevEntry(a_, b_, c_, d_) CNetMDpp::vendorDev(a_, b_), {a_, b_, c_, d_}


/// map with known / supported NetMD devices
const CNetMDpp::KnownDevices CNetMDpp::smKnownDevices = {
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

/// log configuration
structlog LOGCFG = {true, DEBUG, nullptr};

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//--------------------------------------------------------------------------
CNetMDpp::CNetMDpp()
{
    mInitialized = libusb_init(NULL) == 0;
    LOG(INFO) << "Init:" << mInitialized;
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMDpp::~CNetMDpp()
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
//! @brief      Sets the log level.
//!
//! @param[in]  severity  The severity
//--------------------------------------------------------------------------
void CNetMDpp::setLogLevel(typelog severity)
{
    LOGCFG.level = severity;
}

//--------------------------------------------------------------------------
//! @brief      Initializes the device.
//!
//! @return     0 -> ok; < 0 -> error
//--------------------------------------------------------------------------
int CNetMDpp::initDevice()
{
    int ret = -1;

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
                            ret = 0;
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
//! @brief      Gets the device name.
//!
//! @return     The device name.
//--------------------------------------------------------------------------
const std::string& CNetMDpp::getDeviceName()
{
    char buff[64] = {'\0',};

    if (libusb_get_string_descriptor_ascii(mDevice.mDevHdl, 2, reinterpret_cast<unsigned char*>(buff), 64) == 0)
    {
        mDevice.mName = buff;
    }

    return mDevice.mName;
}

//--------------------------------------------------------------------------
//! @brief      polls to see if minidisc wants to send data
//!
//! @param      buf    The poll buffer buffer
//! @param[in]  tries  The number of tries
//!
//! @return     < 0 -> NetMdErr; else number of bytes device wants to send
//--------------------------------------------------------------------------
int CNetMDpp::poll(uint8_t buf[4], int tries)
{
    int sleepytime = 5;

    for (int i = 0; i < tries; i++)
    {
        // send a poll message
        memset(buf, 0, 4);

        if (libusb_control_transfer(mDevice.mDevHdl,
                                    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                    0x01, 0, 0, buf, 4, NETMD_POLL_TIMEOUT) < 0)
        {
            LOG(ERROR) << __FUNCTION__ << " libusb_control_transfer failed!";
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
int CNetMDpp::sendCmd(unsigned char* cmd, size_t cmdLen, bool factory)
{
    unsigned char pollbuf[4];
    int len;

    // poll to see if we can send data
    len = poll(pollbuf, 1);
    if (len != 0)
    {
        LOG(ERROR) << __FUNCTION__ << " poll() failed!";
        return (len > 0) ? NETMDERR_NOTREADY : len;
    }

    // send data
    LOG(DEBUG) << (factory ? "factory " : "") << "command:" << LOG::hexFormat(DEBUG, cmd, cmdLen);

    if (libusb_control_transfer(mDevice.mDevHdl,
                                LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                factory ? 0xff : 0x80, 0, 0, cmd, cmdLen,
                                NETMD_SEND_TIMEOUT) < 0)
    {
        LOG(ERROR) << __FUNCTION__ << " libusb_control_transfer failed!";
        return NETMDERR_USB;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Gets the response.
//!
//! @param      response  The response
//!
//! @return     The response size or NetMdErr.
//--------------------------------------------------------------------------
int CNetMDpp::getResponse(NetMDResp& response)
{
    unsigned char pollbuf[4];

    // poll for data that minidisc wants to send
    int ret = poll(pollbuf, NETMD_RECV_TRIES);
    if (ret <= 0)
    {
        LOG(ERROR) << __FUNCTION__ << " poll failed!";
        return (ret == 0) ? NETMDERR_TIMEOUT : ret;
    }

    response = NetMDResp(new unsigned char[ret]);

    // receive data
    if (libusb_control_transfer(mDevice.mDevHdl,
                                LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
                                pollbuf[1], 0, 0, response.get(), ret,
                                NETMD_RECV_TIMEOUT) < 0)
    {
        LOG(ERROR) << __FUNCTION__ << " libusb_control_transfer failed!";
        return NETMDERR_USB;
    }

    LOG(DEBUG) << "Response:" << LOG::hexFormat(DEBUG, response.get(), ret);

    // return length
    return ret;
}

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
int CNetMDpp::exchange(unsigned char* cmd, size_t cmdLen, NetMDResp* response, bool factory)
{
    int ret = 0;
    NetMDResp tmpRsp;
    NetMDResp* pResp = response;


    if (response == nullptr)
    {
        pResp = &tmpRsp;
    }

    if ((ret = sendCmd(cmd, cmdLen, factory)) == NETMDERR_NO_ERROR)
    {
        ret = getResponse(*pResp);

        if (*pResp == nullptr)
        {
            ret = NETMDERR_CMD_FAILED;
        }
        else if ((*pResp)[0] == NETMD_STATUS_INTERIM)
        {
            LOG(DEBUG) << __FUNCTION__ << " Re-read ...!";
            (*pResp) = nullptr;
            ret = getResponse(*pResp);

            if (*pResp == nullptr)
            {
                ret = NETMDERR_USB;
            }
        }
        else
        {
            LOG(DEBUG) << "Response code:" << LOG::hexFormat(DEBUG, &(*pResp)[0], 1);
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      wait for the device respond to a command
//!
//! @return     0 -> not synced; > 0 -> synced
//--------------------------------------------------------------------------
int CNetMDpp::waitForSync()
{
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
            LOG(DEBUG) << __FUNCTION__ << " libusb error " << ret << " waiting for control transfer!";
        }
        else if (ret != 4)
        {
            LOG(DEBUG) << __FUNCTION__ << " control transfer returned " << ret << " bytes instead of the expected 4!";
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
        LOG(WARN) << __FUNCTION__ << " no sync response from device!";
    }
    else
    {
        LOG(DEBUG) << __FUNCTION__ << " device successfully synced!";
    }

    return success ? 1 : 0;
}

//--------------------------------------------------------------------------
//! @brief      Calculates the checksum.
//!
//! @param[in]  data     The data
//! @param[in]  dataLen  The data length
//!
//! @return     The checksum.
//--------------------------------------------------------------------------
unsigned int CNetMDpp::calculateChecksum(unsigned char* data, size_t dataLen)
{
    unsigned int crc  = 0;
    unsigned int temp = dataLen;

    for (size_t i = 0; i < dataLen; i++)
    {
        temp = (temp & 0xffff0000) | data[i];
        crc ^= temp;

        for (int j = 0; j < 16; j++)
        {
            unsigned int ts = crc & 0x8000;
            crc <<= 1;
            if (ts)
            {
                crc ^= 0x1021;
            }
        }
    }

    return (crc & 0xffff);
}

//--------------------------------------------------------------------------
//! @brief      cache table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::cacheTOC()
{
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    return exchange(request, sizeof(request));
}

//--------------------------------------------------------------------------
//! @brief      sync table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::syncTOC()
{
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    return exchange(request, sizeof(request));
}

//--------------------------------------------------------------------------
//! @brief      aquire device (needed for Sharp)
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::aquireDev()
{
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    NetMDResp response;

    exchange(request, sizeof(request), &response);

    if ((response != nullptr) && (response[0] == NETMD_STATUS_ACCEPTED))
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
int CNetMDpp::releaseDev()
{
    int ret = 0;
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    ret = exchange(request, sizeof(request));
    return (ret < 0) ? ret : NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      request track count
//!
//! @return     < 0 -> NetMdErr; else -> track count
//--------------------------------------------------------------------------
int CNetMDpp::trackCount()
{
    unsigned char req[] = {0x00, 0x18, 0x06, 0x02, 0x10, 0x10,
                           0x01, 0x30, 0x00, 0x10, 0x00, 0xff,
                           0x00, 0x00, 0x00, 0x00, 0x00};

    NetMDResp response;

    int ret = exchange(req, sizeof(req), &response);

    if ((ret > 0) && (response != nullptr))
    {
        // last byte contains the track count
        ret = response[ret - 1];
    }
    else
    {
        ret = NETMDERR_CMD_FAILED;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      request disc flags
//!
//! @return     < 0 -> NetMdErr; else -> flags
//--------------------------------------------------------------------------
int CNetMDpp::discFlags()
{
    unsigned char req[] = {0x00, 0x18, 0x06, 0x01, 0x10, 0x10,
                           0x00, 0xff, 0x00, 0x00, 0x01, 0x00,
                           0x0b};

    NetMDResp response;

    int ret = exchange(req, sizeof(req), &response);

    if ((ret > 0) && (response != nullptr))
    {
        // last byte contains the flags
        ret = response[ret - 1];
    }
    else
    {
        ret = NETMDERR_CMD_FAILED;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      erase MD
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::eraseDisc()
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};

    ret = exchange(request, sizeof(request));

    return (ret < 0) ? ret : NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      format query for netmd exchange
//!
//! @param[in]  format  The format
//! @param[in]  params  The parameters
//! @param[out] query   The query
//!
//! @return     < 0 -> NetMdErr; else -> query size
//--------------------------------------------------------------------------
int CNetMDpp::formatQuery(const char* format, const NetMDParams& params, NetMDResp& query)
{
    int ret = 0;
    NetMDByteVector queryBuffer;

    uint8_t b;
    char   tok[3]     = {'\0',};
    size_t tokIdx     = 0;
    size_t argno      = 0;
    int    esc        = 0;

    uint8_t wordBuff[sizeof(uint64_t)] = {0,};

    // remove spaces
    while (*format != '\0')
    {
        // add some kind of sanity check
        if (argno > params.size())
        {
            LOG(ERROR) << __FUNCTION__ << " Error sanity check while creating query!";
            return NETMDERR_PARAM;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;

            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    b = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        queryBuffer.push_back(b);
                    }
                    else
                    {
                        // can't convert char* to number
                        LOG(ERROR) << __FUNCTION__ << " Can't convert token '" << tok << "' into hex number";
                        return NETMDERR_PARAM;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case 'b':
                if (params.at(argno).index() == UINT8_T)
                {
                    queryBuffer.push_back(std::get<uint8_t>(params.at(argno++)));
                    esc = 0;
                }
                else
                {
                    LOG(ERROR) << __FUNCTION__ << " Stored parameter isn't of type BYTE!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'w':
                if (params.at(argno).index() == UINT16_T)
                {
                    *reinterpret_cast<uint16_t*>(wordBuff) = toNetMD(std::get<uint16_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint16_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                }
                else
                {
                    LOG(ERROR) << __FUNCTION__ << " Stored parameter isn't of type WORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'd':
                if (params.at(argno).index() == UINT32_T)
                {
                    *reinterpret_cast<uint32_t*>(wordBuff) = toNetMD(std::get<uint32_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint32_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                }
                else
                {
                    LOG(ERROR) << __FUNCTION__ << " Stored parameter isn't of type DWORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case 'q':
                if (params.at(argno).index() == UINT64_T)
                {
                    *reinterpret_cast<uint64_t*>(wordBuff) = toNetMD(std::get<uint64_t>(params.at(argno++)));
                    for (size_t s = 0; s < sizeof(uint64_t); s++)
                    {
                        queryBuffer.push_back(wordBuff[s]);
                    }
                    esc = 0;
                }
                else
                {
                    LOG(ERROR) << __FUNCTION__ << " Stored parameter isn't of type QWORD!";
                    return NETMDERR_PARAM;
                }
                break;

            case '*':
                if (params.at(argno).index() == BYTE_VECTOR)
                {
                    NetMDByteVector ba = std::get<NetMDByteVector>(params.at(argno++));
                    for (const auto& v : ba)
                    {
                        queryBuffer.push_back(v);
                    }
                    esc = 0;
                }
                else
                {
                    LOG(ERROR) << __FUNCTION__ << " Stored parameter isn't of type NetMDByteVector!";
                    return NETMDERR_PARAM;
                }
                break;

            case '<':
            case '>':
                // ignore endianess, we always need little endian
                break;

            default:
                LOG(ERROR) << __FUNCTION__ << " Unsupported format option '"
                           << static_cast<char>(c) << "' used query format!";
                return NETMDERR_PARAM;
                break;
            }
        }
        format++;
    }

    if (!queryBuffer.empty())
    {
        ret   = static_cast<int>(queryBuffer.size());
        query = NetMDResp(new uint8_t[ret]);

        for(int i = 0; i < ret; i++)
        {
            query[i] = queryBuffer.at(i);
        }

        LOG(DEBUG) << "Formatted query code:" << LOG::hexFormat(DEBUG, query.get(), ret);
    }

    return ret;

}

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
int CNetMDpp::scanQuery(const uint8_t data[], size_t size, const char* format, NetMDParams& params)
{
    params.clear();
    int     ret                 = NETMDERR_PARAM;
    int     esc                 =  0;
    char    tok[3]              = {'\0',};
    size_t  tokIdx              =  0;
    size_t  dataIdx             = 0;
    uint8_t cmp                 = 0;

    LOG(DEBUG) << "Scan query:" << LOG::hexFormat(DEBUG, data, size);

    // remove spaces
    while (*format != '\0')
    {
        if (dataIdx >= size)
        {
            LOG(ERROR) << __FUNCTION__ << " Error sanity check scanning response!";
            return ret;
        }

        if (!esc)
        {
            switch(*format)
            {
            case '\t':
            case ' ':
                // ignore
                break;
            case '%':
                esc = 1;
                break;
            default:
                tok[tokIdx++] = *format;
                if (tokIdx == 2)
                {
                    char *end;
                    cmp = strtoul(tok, &end, 16);

                    if (end != tok)
                    {
                        if (cmp != data[dataIdx++])
                        {
                            LOG(ERROR) << __FUNCTION__ << " Error! Got: " << std::hex
                                       << static_cast<int>(data[dataIdx - 1])
                                       << " expected: " << std::hex << static_cast<int>(cmp) << std::dec;
                            return ret;
                        }
                    }
                    else
                    {
                        // can't convert char* to number
                        LOG(ERROR) << __FUNCTION__ <<  " Can't convert token '" << tok << "' into hex number!";
                        return ret;
                    }

                    tokIdx = 0;
                }
                break;
            }
        }
        else
        {
            int c;
            switch((c = tolower(*format)))
            {
            case '?':
                esc = 0;
                dataIdx ++;
                break;

            case 'b':
                // capture byte
                params.push_back(data[dataIdx++]);
                esc = 0;
                break;

            case 'w':
                // capture word
                params.push_back(fromNetMD(*reinterpret_cast<const uint16_t*>(&data[dataIdx])));
                dataIdx += sizeof(uint16_t);
                esc = 0;
                break;

            case 'd':
                // capture dword
                params.push_back(fromNetMD(*reinterpret_cast<const uint32_t*>(&data[dataIdx])));
                dataIdx += sizeof(uint32_t);
                esc = 0;
                break;

            case 'q':
                // capture qword
                params.push_back(fromNetMD(*reinterpret_cast<const uint64_t*>(&data[dataIdx])));
                dataIdx += sizeof(uint64_t);
                esc = 0;
                break;

            case '*':
                // capture byte array
                {
                    NetMDByteVector ba;
                    while (dataIdx < size)
                    {
                        ba.push_back(data[dataIdx++]);
                    }
                    params.push_back(ba);
                    esc = 0;
                }
                break;

            case '<':
            case '>':
                // ignore endianess, we always get little endian
                break;

            default:
                LOG(ERROR) << __FUNCTION__ << " Unsupported format option '"
                           << static_cast<char>(c) << "' used query format!";
                return ret;
                break;
            }
        }

        format ++;
    }

    if (!params.empty())
    {
        ret = NETMDERR_NO_ERROR;
    }


    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get track time
//!
//! @param[in]  trackNo    The track no
//! @param      trackTime  The track time
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::trackTime(int trackNo, TrackTime& trackTime)
{
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp query, response;

    if ((formatQuery("00 1806 02 20 10 01 %w 30 00 01 00 ff 00 00 00 00 00",
                    {{mWORD(trackNo)}}, query) == 19) && (query != nullptr))
    {
        exchange(hs, sizeof(hs));
        if ((exchange(query.get(), 19, &response) >= 31) && (response != nullptr))
        {
            trackTime.mMinutes   = bcd_to_proper(&response[28], 1) & 0xff;
            trackTime.mSeconds   = bcd_to_proper(&response[29], 1) & 0xff;
            trackTime.mTenthSecs = bcd_to_proper(&response[30], 1) & 0xff;
            return NETMDERR_NO_ERROR;
        }
    }

    LOG(ERROR) << __FUNCTION__ << " Error receiving track times!";
    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      get disc title
//!
//! @param[out] title  The title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::discTitle(std::string& title)
{
    int ret;
    uint16_t total = 1, remaining = 0, read = 0, chunkSz = 0;
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp request, response;
    const char* format = "00 1806 02 20 18 01 00 00 30 00 0a 00 ff 00 %w %w";

    exchange(hs1, sizeof(hs1));

    while (read < total)
    {
        request  = nullptr;
        response = nullptr;

        if (formatQuery(format, {{remaining}, {read}}, request) == 19)
        {
            if (((ret = exchange(request.get(), 19, &response)) > 0) && (response != nullptr))
            {
                if (remaining == 0)
                {
                    // first answer
                    total   = fromNetMD(*reinterpret_cast<uint16_t*>(&response[23]));
                    chunkSz = fromNetMD(*reinterpret_cast<uint16_t*>(&response[15])) - 6;

                    for(uint16_t i = 25; i < (25 + chunkSz); i++)
                    {
                        title.push_back(static_cast<char>(response[i]));
                    }
                }
                else
                {
                    chunkSz = fromNetMD(*reinterpret_cast<uint16_t*>(&response[15]));

                    for(uint16_t i = 19; i < (19 + chunkSz); i++)
                    {
                        title.push_back(static_cast<char>(response[i]));
                    }
                }

                read += chunkSz;
                remaining = total - read;
            }
            else
            {
                LOG(ERROR) << __FUNCTION__ << " Error in exchange()!";
                return NETMDERR_PARAM;
            }
        }
        else
        {
            LOG(ERROR) << __FUNCTION__ << " Error formatting query!";
            return NETMDERR_PARAM;
        }
    }

    ret = NETMDERR_NO_ERROR;

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Initializes the disc header.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::initDiscHeader()
{
    std::string head;

    if (discTitle(head) == NETMDERR_NO_ERROR)
    {
        return mDiscHeader.fromString(head);
    }

    return NETMDERR_OTHER;
}

//--------------------------------------------------------------------------
//! @brief      Writes a disc header.
//!
//! @param[in]  title  The title (optional)
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMDpp::writeDiscHeader(const std::string& title)
{
    const char* content;
    size_t contentSz = 0;
    std::string currHead;

    int ret = discTitle(currHead);

    if (ret != NETMDERR_NO_ERROR)
    {
        return ret;
    }

    ret = NETMDERR_OTHER;

    if (title.empty())
    {
        content = mDiscHeader.stringHeader();
    }
    else
    {
        content = title.c_str();
    }

    contentSz = strlen(content);

    size_t old_header_size = currHead.size();

    NetMDResp request;

    unsigned char hs[]  = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};

    const char* format = "00 1807 02 20 18 01 00 00 30 00 0a 00 50 00 %w 00 00 %w %*";
    NetMDByteVector ba;

    for (size_t i = 0; i < contentSz; i++)
    {
        ba.push_back(static_cast<uint8_t>(content[i]));
    }

    if (((ret = formatQuery(format, {{mWORD(contentSz)}, {mWORD(old_header_size)}, {ba}},
        request)) > 0) && (request != nullptr))
    {
        exchange(hs , sizeof(hs) );
        exchange(hs2, sizeof(hs2));
        exchange(hs3, sizeof(hs3));

        if (exchange(request.get(), ret) > -1)
        {
            ret = NETMDERR_NO_ERROR;
        }

        exchange(hs2, sizeof(hs2));
    }

    return ret;
}
