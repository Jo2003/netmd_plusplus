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
#include "md_toc.h"

namespace netmd {

//------------------------------------------------------------------------------
//! @brief      This class describes a cluster-sector-group helper
//------------------------------------------------------------------------------
class CSG
{
public:
    /// groups in a sector
    static constexpr uint32_t SECTOR_SIZE  = 11;

    /// sectors in a cluster
    static constexpr uint32_t CLUSTER_SIZE = SECTOR_SIZE * 32;


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
        uint8_t  sector  =  (groups % CLUSTER_SIZE) / SECTOR_SIZE;
        uint8_t  group   = ((groups % CLUSTER_SIZE) % SECTOR_SIZE);

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

        return cluster * CLUSTER_SIZE + sector * SECTOR_SIZE + group;
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

private:
    /// internal groups counter
    uint32_t mGroups;
};


//------------------------------------------------------------------------------
//! @brief      This class describes a net md TOC.
//------------------------------------------------------------------------------
class CNetMdTOC
{
public:

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param[in]     trackCount  The track count
    //! @param[in]     lenInMs     The length in milliseconds
    //! @param[in/out] data        The TOC data
    //--------------------------------------------------------------------------
    CNetMdTOC(int trackCount = 0, uint32_t lenInMs = 0, uint8_t* data = nullptr);

    //--------------------------------------------------------------------------
    //! @brief      import TOC data
    //!
    //! @param      data  The TOC data
    //--------------------------------------------------------------------------
    void import(uint8_t data[sizeof(toc::TOC)]);

    //--------------------------------------------------------------------------
    //! @brief      Adds a track.
    //!
    //! @param[in]  no        track nomber (starting with 1)
    //! @param[in]  lengthMs  The length milliseconds
    //! @param[in]  title     The title
    //!
    //! @return     0 -> ok; -1 -> error
    //--------------------------------------------------------------------------
    int addTrack(uint8_t no, uint32_t lengthMs, const std::string& title);

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
    //! @param[in]  no    The new value
    //!
    //! @return     0
    //--------------------------------------------------------------------------
    int setTrackTStamp(int no);


private:
    /// TOC pointer
    toc::TOC* mpToc;

    /// group number where audio starts
    uint32_t  mAudioStart;

    /// group number where audio ends
    uint32_t  mAudioEnd;

    /// number of tracks for this TOC
    int       mTracksCount;

    /// complete length of all tracks in ms
    uint32_t  mLengthInMs;

    /// current group position
    CSG       mCurPos;
};

} // ~netmd