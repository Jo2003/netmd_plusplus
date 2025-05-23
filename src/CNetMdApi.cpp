/*
 * CNetMdApi.cpp
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

#include "log.h"
#include "CNetMdApi.h"
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <chrono>

/// log configuration
structlog LOGCFG = {true, true, DEBUG, &std::cout, {}};

namespace netmd {

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//--------------------------------------------------------------------------
CNetMdApi::CNetMdApi()
    : mpDiscHeader(nullptr), mpNetMd(nullptr), 
      mpSecure(nullptr), mHotplugCallback(nullptr)
{
    mpDiscHeader = new CMDiscHeader;
    mpNetMd      = new CNetMdDev;

    if (mpNetMd != nullptr)
    {
        mpSecure = new CNetMdSecure(*mpNetMd);
        mpNetMd->registerDeviceCallback(std::bind(&CNetMdApi::hotplugEvent, this, std::placeholders::_1));
    }
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdApi::~CNetMdApi()
{
    if (mpSecure != nullptr)
    {
        delete mpSecure;
        mpSecure = nullptr;
    }

    if (mpNetMd != nullptr)
    {
        delete mpNetMd;
        mpNetMd = nullptr;
    }

    if (mpDiscHeader != nullptr)
    {
        delete mpDiscHeader;
        mpDiscHeader = nullptr;
    }
}

//--------------------------------------------------------------------------
//! @brief      Sets the log level.
//!
//! @param[in]  severity  The severity
//--------------------------------------------------------------------------
void CNetMdApi::setLogLevel(int severity)
{
    std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);
    LOGCFG.level = severity;
}

//--------------------------------------------------------------------------
//! @brief      Sets the log stream.
//!
//! @param      os    The stream instance to log to
//--------------------------------------------------------------------------
void CNetMdApi::setLogStream(std::ostream& os)
{
    std::unique_lock<std::mutex> lck(LOGCFG.mtxLog);
    LOGCFG.sout = &os;
}

//--------------------------------------------------------------------------
//! @brief      init libusb hotplug (native or emulation)
//
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::initHotPlug()
{
    int ret;
    if ((ret = mpNetMd->initHotPlug()) == NETMDERR_NO_ERROR)
    {
        return initDiscHeader();
    }
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Initializes the device.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::initDevice()
{
    int ret;
    if ((ret = mpNetMd->initDevice()) == NETMDERR_NO_ERROR)
    {
        return initDiscHeader();
    }
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Gets the device name.
//!
//! @return     The device name.
//--------------------------------------------------------------------------
std::string CNetMdApi::getDeviceName() const
{
    return mpNetMd->getDeviceName();
}


//--------------------------------------------------------------------------
//! @brief      cache table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::cacheTOC()
{
    mFLOW(DEBUG);
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    int ret = mpNetMd->exchange(request, sizeof(request));

    if (ret > 0) ret = NETMDERR_NO_ERROR;

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      sync table of contents
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::syncTOC()
{
    mFLOW(DEBUG);
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    int ret = mpNetMd->exchange(request, sizeof(request));

    if (ret > 0) ret = NETMDERR_NO_ERROR;

    return ret;
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

    int ret = mpNetMd->exchange(req, sizeof(req), &response);

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

    int ret = mpNetMd->exchange(req, sizeof(req), &response);

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
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};
    int ret = mpNetMd->exchange(request, sizeof(request));

    if (ret > 0)
    {
        return initDiscHeader();
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
int CNetMdApi::trackTime(int trackNo, TrackTime& trackTime)
{
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp query, response;

    if ((formatQuery("00 1806 02 20 10 01 %>w 30 00 01 00 ff 00 00 00 00 00",
                    {{mWORD(trackNo)}}, query) == 19) && (query != nullptr))
    {
        mpNetMd->exchange(hs, sizeof(hs));
        if ((mpNetMd->exchange(query.get(), 19, &response) >= 31) && (response != nullptr))
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
//! @brief      get raw disc header
//!
//! @param[out] header  The buffer for disc header
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::rawDiscHeader(std::string& header)
{
    header.clear();

    int ret;
    uint16_t total = 1, remaining = 0, read = 0, chunkSz = 0;
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};

    NetMDResp request, response;
    const char* format = "00 1806 02 20 18 01 00 00 30 00 0a 00 ff 00 %>w %>w";

    mpNetMd->exchange(hs1, sizeof(hs1));

    while (read < total)
    {
        request  = nullptr;
        response = nullptr;

        if (formatQuery(format, {{remaining}, {read}}, request) == 19)
        {
            if ((mpNetMd->exchange(request.get(), 19, &response) > 0) && (response != nullptr))
            {
                if (remaining == 0)
                {
                    // first answer
                    total   = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[23]));
                    chunkSz = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[15])) - 6;

                    mLOG(DEBUG) << "Total size: " << total << ", chunk size: " << chunkSz;

                    for(uint16_t i = 25; i < (25 + chunkSz); i++)
                    {
                        header.push_back(static_cast<char>(response[i]));
                    }
                }
                else
                {
                    chunkSz = fromBigEndian(*reinterpret_cast<uint16_t*>(&response[15]));

                    for(uint16_t i = 19; i < (19 + chunkSz); i++)
                    {
                        header.push_back(static_cast<char>(response[i]));
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

    if (rawDiscHeader(head) == NETMDERR_NO_ERROR)
    {
        return mpDiscHeader->fromString(head);
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      get disc title
//!
//! @param      title  The title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::discTitle(std::string& title)
{
    title = mpDiscHeader->discTitle();
    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  title  The title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::setDiscTitle(const std::string& title)
{
    if (mpDiscHeader->setDiscTitle(title) == 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      Writes a disc header.
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::writeRawDiscHeader()
{
    const char* content;
    size_t contentSz = 0;
    std::string currHead;
    std::string tmpStr;

    int ret = rawDiscHeader(currHead);

    if (ret != NETMDERR_NO_ERROR)
    {
        return ret;
    }

    tmpStr    = mpDiscHeader->toString();
    contentSz = tmpStr.size();
    content   = tmpStr.c_str();

    size_t old_header_size = currHead.size();

    NetMDResp request;

    unsigned char hs[]  = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};

    const char* format = "00 1807 02 20 18 01 00 00 30 00 0a 00 50 00 %>w 00 00 %>w %*";
    NetMDByteVector ba;
    addArrayData(ba, reinterpret_cast<const uint8_t*>(content), contentSz);

    if (((ret = formatQuery(format, {{mWORD(contentSz)}, {mWORD(old_header_size)}, {ba}},
        request)) > 0) && (request != nullptr))
    {
        mpNetMd->exchange(hs , sizeof(hs) );
        mpNetMd->exchange(hs2, sizeof(hs2));
        mpNetMd->exchange(hs3, sizeof(hs3));

        if ((ret = mpNetMd->exchange(request.get(), ret)) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }

        mpNetMd->exchange(hs2, sizeof(hs2));
    }
    else
    {
        ret = NETMDERR_CMD_FAILED;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      move a track (number)
//!
//! @param[in]  from  from position
//! @param[in]  to    to position
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::moveTrack(uint16_t from, uint16_t to)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x00, 0x00};

    const char* format = "00 1843 ff 00 00 20 10 01 %>w 20 10 01 %>w";

    NetMDResp query;

    if (((ret = formatQuery(format, {{from}, {to}}, query)) == 16) && (query != nullptr))
    {
        mpNetMd->exchange(hs, sizeof(hs));
        if ((ret = mpNetMd->exchange(query.get(), ret)) > 0)
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

//--------------------------------------------------------------------------
//! @brief      Sets the group title.
//!
//! @param[in]  group  The group
//! @param[in]  title  The title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::setGroupTitle(uint16_t group, const std::string& title)
{
    if (mpDiscHeader->renameGroup(group, title) == 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      Creates a group.
//!
//! @param[in]  title  The title
//! @param[in]  first  The first track
//! @param[in]  last   The last track
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::createGroup(const std::string& title, int first, int last)
{
    if (mpDiscHeader->addGroup(title, first, last) >= 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  track  The track
//! @param[in]  group  The group
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::addTrackToGroup(int track, int group)
{
    // this might fail
    mpDiscHeader->delTrackFromGroup(group, track);

    if (mpDiscHeader->addTrackToGroup(group, track) == 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      remove track from group
//!
//! @param[in]  track  The track
//! @param[in]  group  The group
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::delTrackFromGroup(int track, int group)
{
    if (mpDiscHeader->delTrackFromGroup(group, track) == 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      delete a group
//!
//! @param[in]  group  The group
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::deleteGroup(int group)
{
    if (mpDiscHeader->delGroup(group) == 0)
    {
        return writeRawDiscHeader();
    }

    return NETMDERR_PARAM;
}

//--------------------------------------------------------------------------
//! @brief      delete track
//!
//! @param[in]  track  The track number
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::deleteTrack(uint16_t track)
{
    int ret = 0;

    if (trackCount() > static_cast<int>(track))
    {
        const char* format = "00 1840 ff 01 00 20 10 01 %>w";
        NetMDResp query;
        if (((ret = formatQuery(format, {{track}}, query)) == 11) && (query != nullptr))
        {
            cacheTOC();
            if ((ret = mpNetMd->exchange(query.get(), ret)) > 0)
            {
                mpNetMd->waitForSync();
                ret = NETMDERR_NO_ERROR;
            }
            syncTOC();

            if (ret == NETMDERR_NO_ERROR)
            {
                initDiscHeader();
            }
        }
        else
        {
            ret = NETMDERR_PARAM;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get track bitrate data
//!
//! @param[in]  track     The track number
//! @param[out] encoding  The encoding flag
//! @param[out] channel   The channel flag
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::trackBitRate(uint16_t track, AudioEncoding& encoding, uint8_t& channel)
{
    encoding = AudioEncoding::UNKNOWN;
    channel  = 0;

    int ret;
    const char* format = "00 1806 02 20 10 01 %>w 30 80 07 00 ff 00 00 00 00 00";
    NetMDResp query, response;

    if (((ret = formatQuery(format, {{track}}, query)) == 19) && (query != nullptr))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if ((mpNetMd->exchange(query.get(), ret, &response) >= 29) && (response != nullptr))
        {
            encoding = static_cast<AudioEncoding>(response[27]);
            channel  = response[28];
            ret = NETMDERR_NO_ERROR;
        }
        else
        {
            ret = NETMDERR_PARAM;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get track flags
//!
//! @param[in]  track  The track number
//! @param[out] flags  The track flags
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::trackFlags(uint16_t track, TrackProtection& flags)
{
    flags = TrackProtection::UNKNOWN;
    int ret;

    NetMDResp query, response;
    const char* format = "00 1806 01 20 10 01 %>w ff 00 00 01 00 08";

    if (((ret = formatQuery(format, {{track}}, query)) == 15) && (query != nullptr))
    {
        if (((ret = mpNetMd->exchange(query.get(), ret, &response)) > 0) && (response != nullptr))
        {
            flags = static_cast<TrackProtection>(response[ret - 1]);
            ret = NETMDERR_NO_ERROR;
        }
        else
        {
            ret = NETMDERR_PARAM;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get track title
//!
//! @param[in]  track  The track number
//! @param[out] title  The track title
//!
//! @return     NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::trackTitle(uint16_t track, std::string& title)
{
    title.clear();

    int ret;

    NetMDResp query, response;
    const char* format  = "00 1806 02 20 18 02 %>w 30 00 0a 00 ff 00 00 00 00 00";

    if (((ret = formatQuery(format, {{track}}, query)) == 19) && (query != nullptr))
    {
        if (((ret = mpNetMd->exchange(query.get(), ret, &response)) >= 25) && (response != nullptr))
        {
            for (int i = 25; i < ret; i++)
            {
                title.push_back(static_cast<char>(response[i]));
            }

            ret = NETMDERR_NO_ERROR;
        }
        else
        {
            ret = NETMDERR_PARAM;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      is SP upload supported?
//!
//! @return     true if yes
//--------------------------------------------------------------------------
bool CNetMdApi::spUploadSupported()
{
    return mpSecure->spUploadSupported();
}

//--------------------------------------------------------------------------
//! @brief      Sets the track title.
//!
//! @param[in]  trackNo  The track no
//! @param[in]  title    The title
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::setTrackTitle(uint16_t trackNo, const std::string& title)
{
    int ret;
    std::string currTitle;
    uint8_t oldSz = 0;

    if (trackTitle(trackNo, currTitle) == NETMDERR_NO_ERROR)
    {
        oldSz = currTitle.size();
    }

    NetMDByteVector ba;
    NetMDResp query;
    addArrayData(ba, reinterpret_cast<const uint8_t*>(title.c_str()), title.size());
    NetMDParams params = {
        {trackNo                },
        {mBYTE(title.size())    },
        {oldSz                  },
        {ba                     }
    };

    const char* format = "00 1807 02 20 18 02 %>w 30 00 0a 00 50 00 00 %b 00 00 00 %b %*";

    if (((ret = formatQuery(format, params, query)) > 0) && (query != nullptr))
    {
        cacheTOC();
        if (mpNetMd->exchange(query.get(), ret) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }
        else
        {
            ret = NETMDERR_PARAM;
            mLOG(CRITICAL) << "exchange() failed.";
        }
        syncTOC();
    }
    else
    {
        ret = NETMDERR_PARAM;
    }


    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Sends an audio track
//!
//! @param[in]  filename  The filename
//! @param[in]  title     The title
//! @param[in]  otf       The disk format
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::sendAudioFile(const std::string& filename, const std::string& title, DiskFormat otf)
{
    mFLOW(INFO);
    return mpSecure->sendAudioTrack(filename, title, otf);
}

//--------------------------------------------------------------------------
//! @brief      is on the fly encoding supported by device
//!
//! @return     true if so
//--------------------------------------------------------------------------
bool CNetMdApi::otfEncodeSupported()
{
    return mpNetMd->mDevice.mKnownDev.mOtfEncode;
}

//--------------------------------------------------------------------------
//! @brief      get disc capacity
//!
//! @param      dcap  The buffer for disc capacity
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::discCapacity(DiscCapacity& dcap)
{
    int ret;
    uint8_t hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x00, 0x01, 0x00};
    uint8_t request[] = {0x00, 0x18, 0x06, 0x02, 0x10, 0x10,
                         0x00, 0x30, 0x80, 0x03, 0x00, 0xff,
                         0x00, 0x00, 0x00, 0x00, 0x00};

    NetMDResp resp;
    mpNetMd->exchange(hs, sizeof(hs));

    if ((mpNetMd->exchange(request, sizeof(request), &resp) >= 46) && (resp != nullptr))
    {
        parse_time(resp.get() + 27, dcap.recorded);
        parse_time(resp.get() + 34, dcap.total);
        parse_time(resp.get() + 41, dcap.available);
        ret = NETMDERR_NO_ERROR;
    }
    else
    {
        ret = NETMDERR_CMD_FAILED;
    }
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get MD track groups
//!
//! @return     vector of group structures
//--------------------------------------------------------------------------
Groups CNetMdApi::groups()
{
    return mpDiscHeader->groups();
}

//--------------------------------------------------------------------------
//! @brief      Reads an utoc sector.
//!
//! @param[in]  s     sector number
//!
//! @return     TOC sector data. (error if empty)
//--------------------------------------------------------------------------
NetMDByteVector CNetMdApi::readUTOCSector(UTOCSector s)
{
    return mpSecure->readUTOCSector(s);
}

//--------------------------------------------------------------------------
//! @brief      Writes an utoc sector.
//!
//! @param[in]  s     sector names
//! @param[in]  data  The data to be written
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::writeUTOCSector(UTOCSector s, const NetMDByteVector& data)
{
    return mpSecure->writeUTOCSector(s, data);
}

//--------------------------------------------------------------------------
//! @brief      finalize TOC through exploit
//!
//! @param[in]  reset      do reset if true (default: false)
//! @param[in]  resetWait  The optional reset wait time (20 seconds)
//!                        Only needed if reset is true
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::finalizeTOC(bool reset, uint8_t resetWait)
{
    int ret = mpSecure->finalizeTOC(reset);

    if (reset && (ret == NETMDERR_NO_ERROR))
    {
        int wait = resetWait * 1'000 / 10;
        for(int i = 91; i < 100; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait));
            mLOG(CAPTURE) << "Finalizing TOC: " << std::setw(2) << std::setfill('0') << i << "%";
        }
        if (!mpNetMd->mbHotPlug)
        {
            ret = initDevice();
        }
    }
    mLOG(CAPTURE) << "Finalizing TOC: 100%";
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      is TOC manipulation supported
//!
//! @return     true if supported, false if not
//------------------------------------------------------------------------------
bool CNetMdApi::tocManipSupported()
{
    return mpSecure->pcm2MonoSupported();
}

//--------------------------------------------------------------------------
//! @brief      is PCM to mono supported?
//!
//! @return     true if supported, false if not
//--------------------------------------------------------------------------
bool CNetMdApi::pcm2MonoSupported()
{
    // same devices support PCM to mono
    return mpSecure->tocManipSupported();
}

//--------------------------------------------------------------------------
//! @brief      is native mono upload supported?
//!
//! @return     true if supported, false if not
//--------------------------------------------------------------------------
bool CNetMdApi::nativeMonoUploadSupported()
{
    return mpSecure->nativeMonoUploadSupported();
}

//--------------------------------------------------------------------------
//! @brief      is PCM speedup supportd
//!
//! @return     true if supported, false if not
//--------------------------------------------------------------------------
bool CNetMdApi::pcmSpeedupSupported()
{
    return mpSecure->pcmSpeedupSupported();
}

//--------------------------------------------------------------------------
//! @brief start homebrew 
//
//! @param features OR'd HomebrewFeatures
//
//! @return NetMdErr
//! @see NetMdErr
//--------------------------------------------------------------------------
int CNetMdApi::startHBSession(uint32_t features)
{
    mFLOW(INFO);
    int ret = NETMDERR_NO_ERROR;
    int e   = NETMDERR_NO_ERROR;

    if (features != NOTHING)
    {
        if ((features & USB_EXEC) && tocManipSupported())
        {
            mLOG(INFO) << "apply USB Exec patch ...";
            if ((e = mpSecure->applyUSBExecPatch()) != NETMDERR_NO_ERROR)
            {
                ret = e;
            }
        }

        if ((features & PCM_2_MONO) && pcm2MonoSupported())
        {
            mLOG(INFO) << "apply PCM to mono patch ...";
            if ((e = mpSecure->applyPCM2MonoPatch()) != NETMDERR_NO_ERROR)
            {
                ret = e;
            }
        }

        if ((features & PCM_SPEEDUP) && pcmSpeedupSupported())
        {
            mLOG(INFO) << "apply PCM speedup patch ...";
            if ((e = mpSecure->applyPCMSpeedupPatch()) != NETMDERR_NO_ERROR)
            {
                ret = e;
            }
        }

        if ((features & SP_UPLOAD) && spUploadSupported())
        {
            mLOG(INFO) << "apply SP upload patch ...";
            if ((e = mpSecure->applySPUploadPatch()) != NETMDERR_NO_ERROR)
            {
                ret = e;
            }
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief  stop homebrew session
//
//! @param features OR'd HomebrewFeatures
//
//! @return NetMdErr
//! @see NetMdErr
//--------------------------------------------------------------------------
void CNetMdApi::endHBSession(uint32_t features)
{
    mFLOW(INFO);
    if (features & SP_UPLOAD)
    {
        mpSecure->undoSPUploadPatch();
    }    
    if (features & USB_EXEC)
    {
        mpSecure->undoUSBExecPatch();
    }

    if (features & PCM_2_MONO)
    {
        mpSecure->undoPCM2MonoPatch();
    }

    if (features & PCM_SPEEDUP)
    {
        mpSecure->undoPCMSpeedupPatch();
    }

    if (features & SP_UPLOAD)
    {
        mpSecure->undoSPUploadPatch();
    }
} 

//--------------------------------------------------------------------------
//! @brief      register device callback function
//
//! @param[in]  added  callback function to e called on device add / removal
//--------------------------------------------------------------------------           
void CNetMdApi::hotplugEvent(bool added)
{
    std::unique_lock<std::mutex> lck(mMutexHotplug);
    if (mHotplugCallback)
    {
        mHotplugCallback(added);
    }

    if (!added)
    {
        mLOG(INFO) << "Device removed";
        mpSecure->deviceRemoved();
    }
    else
    {
        mLOG(INFO) << "Device added";
    }
}

//--------------------------------------------------------------------------
//! @brief      register hotplug callback function
//
//! @param[in]  cb  callback function to e called on device add / removal
//--------------------------------------------------------------------------
void CNetMdApi::registerForHotplugEvents(EvtCallback cb)
{
    std::unique_lock<std::mutex> lck(mMutexHotplug);
    mHotplugCallback = cb;
}

} // ~namespace
