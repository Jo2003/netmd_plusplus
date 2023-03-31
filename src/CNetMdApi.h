/*
 * CNetMdApi.h
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
#include "netmd_defines.h"
#include "CNetMdDev.hpp"
#include "CMDiscHeader.h"

namespace netmd {

//------------------------------------------------------------------------------
//! @brief      This class describes a C++ NetMD access library
//------------------------------------------------------------------------------
class CNetMdApi
{
public:

    /// track times
    struct TrackTime
    {
        int mMinutes;
        int mSeconds;
        int mTenthSecs;
    };

    //--------------------------------------------------------------------------
    //! @brief      format helper for TrackTime
    //!
    //! @param      o     ref. to ostream
    //! @param[in]  tt    TrackTime
    //!
    //! @return     formatted TrackTime stored in ostream
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& o, const TrackTime& tt)
    {
        o << std::dec
          << std::setw(2) << std::setfill('0') << tt.mMinutes << ":"
          << std::setw(2) << std::setfill('0') << tt.mSeconds << "."
          << tt.mTenthSecs;

        return o;
    }

    /// type safe protection flags
    enum class TrackProtection : uint8_t
    {
        UNPROTECTED = 0x00,
        PROTECTED   = 0x03,
        UNKNOWN     = 0xFF
    };

    //--------------------------------------------------------------------------
    //! @brief      format helper for TrackProtection
    //!
    //! @param      o     ref. to ostream
    //! @param[in]  tp    TrackProtection
    //!
    //! @return     formatted TrackProtection stored in ostream
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& o, const TrackProtection& tp)
    {
        switch (tp)
        {
        case TrackProtection::UNPROTECTED:
            o << "UnPROT";
            break;
        case TrackProtection::PROTECTED:
            o << "TrPROT";
            break;
        default:
            o << "N/A";
            break;
        }
        return o;
    }

    /// type safe encoding flags
    enum class AudioEncoding : uint8_t
    {
        SP      = 0x90,
        LP2     = 0x92,
        LP4     = 0x93,
        UNKNOWN = 0xff
    };

    //--------------------------------------------------------------------------
    //! @brief      format helper for AudioEncoding
    //!
    //! @param      o     ref. to ostream
    //! @param[in]  ae    AudioEncoding
    //!
    //! @return     formatted AudioEncoding stored in ostream
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& o, const AudioEncoding& ae)
    {
        switch (ae)
        {
        case AudioEncoding::SP:
            o << "SP";
            break;
        case AudioEncoding::LP2:
            o << "LP2";
            break;
        case AudioEncoding::LP4:
            o << "LP4";
            break;
        default:
            o << "N/A";
            break;
        }
        return o;
    }

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //--------------------------------------------------------------------------
    CNetMdApi();

    //--------------------------------------------------------------------------
    //! @brief      Destroys the object.
    //--------------------------------------------------------------------------
    ~CNetMdApi();

    //--------------------------------------------------------------------------
    //! @brief      Initializes the device.
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int initDevice();

    //--------------------------------------------------------------------------
    //! @brief      Initializes the disc header.
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int initDiscHeader();

    //--------------------------------------------------------------------------
    //! @brief      Gets the device name.
    //!
    //! @return     The device name.
    //--------------------------------------------------------------------------
    std::string getDeviceName();

    //--------------------------------------------------------------------------
    //! @brief      Sets the log level.
    //!
    //! @param[in]  severity  The severity
    //--------------------------------------------------------------------------
    static void setLogLevel(int severity);

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

    //--------------------------------------------------------------------------
    //! @brief      get track time
    //!
    //! @param[in]  trackNo    The track no
    //! @param      trackTime  The track time
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int trackTime(int trackNo, TrackTime& trackTime);

    //--------------------------------------------------------------------------
    //! @brief      get disc title
    //!
    //! @param[out] title  The title
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int discTitle(std::string& title);

    //--------------------------------------------------------------------------
    //! @brief      Writes a disc header.
    //!
    //! @param[in]  title  The title (optional)
    //!
    //! @return     NetMdErr
    //--------------------------------------------------------------------------
    int writeDiscHeader(const std::string& title = "");

protected:


private:
    /// disc header
    CMDiscHeader mDiscHeader;

    /// access device class
    CNetMdDev mNetMd;
};

} // ~namespace