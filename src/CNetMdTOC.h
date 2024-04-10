/*
 * CNetMdTOC.h
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

/**
@file CNetMdTOC.h

# TOC edit
We use and edit the TOC only for splitting audio tracks to get gapless Audio.

We assume the TOC contains only 1 audio track.

We don't touch the free list.

We virtualy split the tracks and give them titles.
*/
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include "md_toc.h"

namespace netmd {

/// forward declaration
class CSG;

//------------------------------------------------------------------------------
//! @brief      This class describes a net md TOC.
//------------------------------------------------------------------------------
class CNetMdTOC
{
public:

    /// a fragment used in DAO track
    struct DAOFragment
    {
        uint32_t mStart;    ///< start group
        uint32_t mEnd;      ///< end group
    };

    /// type to store all DAO track fragments (for fragmented, non empty discs)
    using DAOFragments = std::vector<DAOFragment>;

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param[in]     trackCount  The track count
    //! @param[in]     lenInMs     The length in milliseconds
    //! @param[in/out] data        The TOC data
    //--------------------------------------------------------------------------
    CNetMdTOC(int trackCount = 0, uint32_t lenInMs = 0, uint8_t* data = nullptr);

    //--------------------------------------------------------------------------
    //! @brief      Destroys the object.
    //--------------------------------------------------------------------------
    ~CNetMdTOC();

    //--------------------------------------------------------------------------
    //! @brief      import TOC data
    //!
    //! @param[in]  trackCount  The track count
    //! @param[in]  lenInMs     The length in milliseconds
    //! @param      data        The TOC data
    //--------------------------------------------------------------------------
    void import(int trackCount = 0, uint32_t lenInMs = 0, uint8_t* data = nullptr);

    //--------------------------------------------------------------------------
    //! @brief      Adds a track.
    //!
    //! This function has to be used to split a DAO transferred disc audio
    //! track into the parts as on the original disc. This functions has to
    //! be called for all tracks in their correct order!
    //! **Breaking the order will break the TOC!**
    //!
    //! @param[in]  no        track number (starting with 1)
    //! @param[in]  lengthMs  The length milliseconds
    //! @param[in]  title     The title
    //! @param[in]  tstamp    The time stamp
    //!
    //! @return     0 -> ok; -1 -> error
    //--------------------------------------------------------------------------
    int addTrack(uint8_t no, uint32_t lengthMs, const std::string& title, const std::time_t& tstamp);

    //--------------------------------------------------------------------------
    //! @brief      Sets the disc title.
    //!
    //! @param[in]  title  The title
    //!
    //! @return     0 -> ok; -1 -> error
    //--------------------------------------------------------------------------
    int setDiscTitle(const std::string& title);

    //--------------------------------------------------------------------------
    //! @brief      get track count
    //!
    //! @return     number of tracks
    //--------------------------------------------------------------------------
    int trackCount() const;

    //--------------------------------------------------------------------------
    //! @brief      get MD title
    //!
    //! @return     title
    //--------------------------------------------------------------------------
    std::string discTitle() const;

    //--------------------------------------------------------------------------
    //! @brief      get track title
    //!
    //! @param[in]  trackNo  The track number
    //!
    //! @return     title
    //--------------------------------------------------------------------------
    std::string trackTitle(int trackNo) const;

    //--------------------------------------------------------------------------
    //! @brief      get track info
    //!
    //! @param[in]  trackNo  The track number
    //!
    //! @return     track info
    //--------------------------------------------------------------------------
    std::string trackInfo(int trackNo) const;

    //--------------------------------------------------------------------------
    //! @brief      get disc info
    //!
    //! @return     disc info
    //--------------------------------------------------------------------------
    std::string discInfo() const;

protected:

    //--------------------------------------------------------------------------
    //! @brief      Sets the track title.
    //!
    //! @param[in]  no     The track number
    //! @param[in]  title  The title
    //!
    //! @return     0
    //--------------------------------------------------------------------------
    int setTrackTitle(uint8_t no, const std::string& title);

    //--------------------------------------------------------------------------
    //! @brief      Sets the track time stamp.
    //!
    //! @param[in]  no     The track number
    //! @param[in]  tstamp The time stamp
    //!
    //! @return     0
    //--------------------------------------------------------------------------
    int setTrackTStamp(int no, const std::time_t& tstamp);

    //--------------------------------------------------------------------------
    //! @brief      get next free title cell
    //!
    //! @return     cell number or -1 on error
    //--------------------------------------------------------------------------
    int nextFreeTitleCell();

    //--------------------------------------------------------------------------
    //! @brief      get next free track fragment
    //!
    //! @return     fragment number or -1 on error
    //--------------------------------------------------------------------------
    int nextFreeTrackFragment();

    //--------------------------------------------------------------------------
    //! @brief      get group count of DAO track
    //!
    //! @return     group count
    //--------------------------------------------------------------------------
    uint32_t daoGroupCount() const;

    //--------------------------------------------------------------------------
    //! @brief      Gets the track fragments.
    //!
    //! @param[in]  trackNo  The track number
    //! @param[in]  groups   The groups count of the track
    //!
    //! @return     The track fragments.
    //--------------------------------------------------------------------------
    DAOFragments getTrackFragments(int trackNo, uint32_t groups);

private:
    /// TOC pointer
    toc::TOC* mpToc;

    /// number of tracks for this TOC
    int       mTracksCount;

    /// complete length of all tracks in ms
    uint32_t  mLengthInMs;

    /// current group position
    uint32_t  mCurPos;

    /// track we need to split
    int       mDAOTrack;

    /// whole groups count of DAO track
    uint32_t  mDAOGroups;

    /// the fragments used for DAO track
    DAOFragments mDAOFragments;
};

//------------------------------------------------------------------------------
//! @brief      This class describes a cluster-sector-group helper
//------------------------------------------------------------------------------
class CSG
{
/**
## Addressing in UTOC
The disc start and end addresses each consist of a cluster, sector, and sound group, all packed into 3 bytes.
The smallest unit is a sound frame, representing 11.6ms of mono audio (212 bytes), while the smallest
<b>addressable</b> unit is the sound group, containing 2 sound frames. A sector contains 11
sound frames / 5.5 sound groups. Addressing must be done through sound group. Sound groups are numbered
0 ... 10. Sound groups 0 ... 5 are part of the even sector, while sound groups 5 ... 10 are part of the odd sector.
Group 5 overlaps both even and odd sectors and can therefore be addressed on both sectors.
<pre>
+-------------------------------------------+
|                sector pair                |
+---------------------+---------------------+
|   even sector (2n)  |  odd sector (2n+1)  |
+---+---+---+---+---+-+-+---+---+---+---+---+
| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| <- sound groups
+---+---+---+---+---+---+---+---+---+---+---+
</pre>
A cluster is an aggregate of 32 audio sectors (176 sound groups) representing 2.04 seconds of stereo audio; 
it is the smallest unit of data that can be written to a MiniDisc. In the 3 byte packing, there are 14 bits 
allocated to the cluster number, 6 bits to the sector, and 4 bits to the soundgroup; this arrangement allows 
addressing of up to 9.2 hours of stereo audio.
*/
public:
    /// We handle a pair to avoid floating point usage. 
    /// groups in a sector pair
    static constexpr uint32_t SECTOR_PAIR = 11;

    /// sector pairs in a cluster
    static constexpr uint32_t CLUSTER_SIZE = SECTOR_PAIR * 16;

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param[in]  groups  The groups number
    //--------------------------------------------------------------------------
    CSG(uint32_t groups = 0) :mGroups(groups)
    {
    }

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param[in]  csg disc address
    //--------------------------------------------------------------------------
    CSG(const toc::discaddr& csg) :mGroups(fromCsg(csg))
    {
    }

    //--------------------------------------------------------------------------
    //! @brief      convert groups to disc address
    //!
    //! @param[in]  groups  The groups
    //!
    //! @return     disc address
    //--------------------------------------------------------------------------
    static toc::discaddr fromGroups(uint32_t groups)
    {
        uint16_t cluster =   groups / CLUSTER_SIZE;
        uint8_t  sector  =  ((groups % CLUSTER_SIZE) / SECTOR_PAIR) << 1;
        uint8_t  group   = ((groups % CLUSTER_SIZE) % SECTOR_PAIR);

        if (group > 5)
        {
            sector ++;
        }

        toc::discaddr csg;
        csg.csg[0] = static_cast<uint8_t>((cluster >> 6) & 0xff);
        csg.csg[1] = static_cast<uint8_t>((cluster << 2) & 0xfc) | (sector >> 4);
        csg.csg[2] = sector << 4 | (group & 0xf);
        return csg;
    }

    //--------------------------------------------------------------------------
    //! @brief      convert disc address to groups
    //!
    //! @param[in]  csg   The disc address
    //!
    //! @return     groups
    //--------------------------------------------------------------------------
    static uint32_t fromCsg(const toc::discaddr& csg)
    {
        uint16_t cluster  =  (csg.csg[0] << 6) | (csg.csg[1] >> 2);
        uint8_t  sector   = ((csg.csg[1] & 0b11) << 4) | (csg.csg[2] >> 4);
        uint8_t  group    = csg.csg[2] & 0xf;

        return cluster * CLUSTER_SIZE + (sector >> 1) * SECTOR_PAIR + group;
    }

    //--------------------------------------------------------------------------
    //! @brief      convert group count into time
    //!
    //! @param[in]  groupCount  The group count
    //! @param[in]  stereo      true, if stereo
    //!
    //! @return     formatted time
    //--------------------------------------------------------------------------
    static std::string toTime(uint32_t groupCount, bool stereo = true)
    {
        std::ostringstream oss;

        // one group is about 11.6ms in stereo
        if (!stereo)
        {
            groupCount *= 2;
        }

        // time in ms
        uint32_t ms = std::round(static_cast<float>(groupCount) * 11.6);

        oss << std::setw(2) << std::setfill('0') << (ms / 60'000) << ":"
            << std::setw(2) << std::setfill('0') << ((ms % 60'000) / 1'000) << "."
            << std::setw(3) << std::setfill('0') << (ms % 1'000);

        return oss.str();
    }

    //--------------------------------------------------------------------------
    //! @brief      get next disc address
    //!
    //! @return     next disc address
    //--------------------------------------------------------------------------
    toc::discaddr nextAddr()
    {
        uint32_t groups = mGroups + 1;
        return fromGroups(groups);
    }

    //--------------------------------------------------------------------------
    //! @brief      Addition assignment operator.
    //!
    //! @param[in]  groups  The groups
    //!
    //! @return     The result of the addition assignment
    //--------------------------------------------------------------------------
    CSG& operator+=(const uint32_t& groups)
    {
        mGroups += groups;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! @brief      Assignment operator.
    //!
    //! @param[in]  groups  The groups
    //!
    //! @return     The result of the assignment
    //--------------------------------------------------------------------------
    CSG& operator=(const uint32_t& groups)
    {
        mGroups = groups;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! @brief      Assignment operator.
    //!
    //! @param[in]  other  The other
    //!
    //! @return     The result of the assignment
    //--------------------------------------------------------------------------
    CSG& operator=(const CSG& other)
    {
        mGroups = other.mGroups;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! @brief      Assignment operator.
    //!
    //! @param[in]  csg   The csg
    //!
    //! @return     The result of the assignment
    //--------------------------------------------------------------------------
    CSG& operator=(const toc::discaddr& csg)
    {
        mGroups = fromCsg(csg);
        return *this;
    }

    //--------------------------------------------------------------------------
    //! @brief      Uint32_t conversion operator.
    //--------------------------------------------------------------------------
    operator uint32_t() const
    {
        return mGroups;
    }

    //--------------------------------------------------------------------------
    //! @brief      disc address conversion operator.
    //--------------------------------------------------------------------------
    operator toc::discaddr() const
    {
        return fromGroups(mGroups);
    }

    //--------------------------------------------------------------------------
    //! @brief      convert to string (when casted to std::string)
    //--------------------------------------------------------------------------
    operator std::string() const
    {
        std::ostringstream oss;
        uint16_t cluster =   mGroups / CLUSTER_SIZE;
        uint8_t  sector  =  ((mGroups % CLUSTER_SIZE) / SECTOR_PAIR) << 1;
        uint8_t  group   = ((mGroups % CLUSTER_SIZE) % SECTOR_PAIR);

        if (group > 5)
        {
            sector ++;
        }
        oss << cluster << "c:" << sector << "s:" << group << "g";
        return oss.str();
    }

private:
    /// internal groups counter
    uint32_t mGroups;
};


} // ~netmd
