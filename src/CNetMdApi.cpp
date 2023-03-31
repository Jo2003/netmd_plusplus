/*
 * CNetMdApi.cpp
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

#include "CNetMdDev.hpp"
#include "log.h"
#include "CNetMdApi.h"
#include "netmd_utils.h"
#include <cstring>
#include <sys/types.h>

/// log configuration
structlog LOGCFG = {true, DEBUG, nullptr};

namespace netmd {

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//--------------------------------------------------------------------------
CNetMdApi::CNetMdApi()
{
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdApi::~CNetMdApi()
{
}

//--------------------------------------------------------------------------
//! @brief      Sets the log level.
//!
//! @param[in]  severity  The severity
//--------------------------------------------------------------------------
void CNetMdApi::setLogLevel(int severity)
{
    LOGCFG.level = severity;
}

//--------------------------------------------------------------------------
//! @brief      Initializes the device.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::initDevice()
{
    return mNetMd.initDevice();
}

//--------------------------------------------------------------------------
//! @brief      Gets the device name.
//!
//! @return     The device name.
//--------------------------------------------------------------------------
std::string CNetMdApi::getDeviceName()
{
    return mNetMd.getDeviceName();
}


//--------------------------------------------------------------------------
//! @brief      cache table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::cacheTOC()
{
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    return mNetMd.exchange(request, sizeof(request));
}

//--------------------------------------------------------------------------
//! @brief      sync table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::syncTOC()
{
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    return mNetMd.exchange(request, sizeof(request));
}

//--------------------------------------------------------------------------
//! @brief      request track count
//!
//! @return     < 0 -> NetMdErr; else -> track count
//--------------------------------------------------------------------------
int CNetMdApi::trackCount()
{
    unsigned char req[] = {0x00, 0x18, 0x06, 0x02, 0x10, 0x10,
                           0x01, 0x30, 0x00, 0x10, 0x00, 0xff,
                           0x00, 0x00, 0x00, 0x00, 0x00};

    NetMDResp response;

    int ret = mNetMd.exchange(req, sizeof(req), &response);

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
int CNetMdApi::discFlags()
{
    unsigned char req[] = {0x00, 0x18, 0x06, 0x01, 0x10, 0x10,
                           0x00, 0xff, 0x00, 0x00, 0x01, 0x00,
                           0x0b};

    NetMDResp response;

    int ret = mNetMd.exchange(req, sizeof(req), &response);

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
int CNetMdApi::eraseDisc()
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};

    ret = mNetMd.exchange(request, sizeof(request));

    return (ret < 0) ? ret : NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      get track time
//!
//! @param[in]  trackNo    The track no
//! @param      trackTime  The track time
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::trackTime(int trackNo, TrackTime& trackTime)
{
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp query, response;

    if ((formatQuery("00 1806 02 20 10 01 %>w 30 00 01 00 ff 00 00 00 00 00",
                    {{mWORD(trackNo)}}, query) == 19) && (query != nullptr))
    {
        mNetMd.exchange(hs, sizeof(hs));
        if ((mNetMd.exchange(query.get(), 19, &response) >= 31) && (response != nullptr))
        {
            trackTime.mMinutes   = bcd_to_proper(&response[28], 1) & 0xff;
            trackTime.mSeconds   = bcd_to_proper(&response[29], 1) & 0xff;
            trackTime.mTenthSecs = bcd_to_proper(&response[30], 1) & 0xff;

            mLOG(DEBUG) << "Track " << (trackNo + 1) << " length: " << trackTime;
            return NETMDERR_NO_ERROR;
        }
    }

    mLOG(CRITICAL) << "Error receiving track times!";
    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      get disc title
//!
//! @param[out] title  The title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::discTitle(std::string& title)
{
    int ret;
    uint16_t total = 1, remaining = 0, read = 0, chunkSz = 0;
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp request, response;
    const char* format = "00 1806 02 20 18 01 00 00 30 00 0a 00 ff 00 %>w %>w";

    mNetMd.exchange(hs1, sizeof(hs1));

    while (read < total)
    {
        request  = nullptr;
        response = nullptr;

        if (formatQuery(format, {{remaining}, {read}}, request) == 19)
        {
            if (((ret = mNetMd.exchange(request.get(), 19, &response)) > 0) && (response != nullptr))
            {
                if (remaining == 0)
                {
                    // first answer
                    total   = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[23]));
                    chunkSz = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[15])) - 6;

                    mLOG(DEBUG) << "Total size: " << total << ", chunk size: " << chunkSz;

                    for(uint16_t i = 25; i < (25 + chunkSz); i++)
                    {
                        title.push_back(static_cast<char>(response[i]));
                    }
                }
                else
                {
                    chunkSz = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[15]));

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
                mLOG(CRITICAL) << "Error in exchange()!";
                return NETMDERR_PARAM;
            }
        }
        else
        {
            mLOG(CRITICAL) << "Error formatting query!";
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
int CNetMdApi::initDiscHeader()
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
int CNetMdApi::writeDiscHeader(const std::string& title)
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

    const char* format = "00 1807 02 20 18 01 00 00 30 00 0a 00 50 00 %>w 00 00 %>w %*";
    NetMDByteVector ba;

    for (size_t i = 0; i < contentSz; i++)
    {
        ba.push_back(static_cast<uint8_t>(content[i]));
    }

    if (((ret = formatQuery(format, {{mWORD(contentSz)}, {mWORD(old_header_size)}, {ba}},
        request)) > 0) && (request != nullptr))
    {
        mNetMd.exchange(hs , sizeof(hs) );
        mNetMd.exchange(hs2, sizeof(hs2));
        mNetMd.exchange(hs3, sizeof(hs3));

        if (mNetMd.exchange(request.get(), ret) > -1)
        {
            ret = NETMDERR_NO_ERROR;
        }

        mNetMd.exchange(hs2, sizeof(hs2));
    }

    return ret;
}

} // ~namespace