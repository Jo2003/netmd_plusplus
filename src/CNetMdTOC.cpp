/*
 * CNetMdTOC.cpp
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

#include "CNetMdTOC.h"
#include "netmd_utils.h"

namespace netmd {

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//!
//! @param[in]     trackCount  The track count
//! @param[in]     lenInMs     The length in milliseconds
//! @param[in/out] data        The TOC data
//--------------------------------------------------------------------------
CNetMdTOC::CNetMdTOC(int trackCount, uint32_t lenInMs, uint8_t* data)
    :mpToc(nullptr), mAudioStart(0), mAudioEnd(0), mTracksCount(trackCount),
    mLengthInMs(lenInMs)
{
    import(data);
}

//--------------------------------------------------------------------------
//! @brief      import TOC data
//!
//! @param      data  The TOC data
//--------------------------------------------------------------------------
void CNetMdTOC::import(uint8_t data[sizeof(toc::TOC)])
{
    if (data)
    {
        mpToc = reinterpret_cast<toc::TOC*>(data);
        CSG csg(mpToc->mTracks.fraglist[mpToc->mTracks.trackmap[1]].start);
        mAudioStart = static_cast<uint32_t>(csg);
        mCurPos     = mAudioStart;

        csg = mpToc->mTracks.fraglist[mpToc->mTracks.trackmap[1]].end;
        mAudioEnd = static_cast<uint32_t>(csg);

        mpToc->mTitles.free_title_slot = 1;
    }
}

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
//!
//! @return     0 -> ok; -1 -> error
//--------------------------------------------------------------------------
int CNetMdTOC::addTrack(uint8_t no, uint32_t lengthMs, const std::string& title)
{
    if (mpToc == nullptr)
    {
        return -1;
    }

    // track audio data splitting...
    float allGroups   = mAudioEnd - mAudioStart;
    float trackGroups = static_cast<float>(lengthMs) * allGroups /  static_cast<float>(mLengthInMs);

    mpToc->mTracks.ntracks           = no;
    mpToc->mTracks.trackmap[no]      = no;
    mpToc->mTracks.nonempty          = 1;
    mpToc->mTracks.sign              = toBigEndian(toc::SIGNATURE);
    mpToc->mTracks.fraglist[no].mode = toc::DEF_TRACK_MODE;
    mpToc->mTracks.fraglist[no].link = 0;

    if (no == 1)
    {
        // start is set ...
        mCurPos += std::ceil(trackGroups) - 1;
        mpToc->mTracks.fraglist[no].end  = static_cast<toc::discaddr>(mCurPos);
    }
    else if (no == mTracksCount)
    {
        mpToc->mTracks.fraglist[no].start = mCurPos.nextAddr();
        mpToc->mTracks.fraglist[no].end   = static_cast<toc::discaddr>(CSG(mAudioEnd));
    }
    else
    {
        mpToc->mTracks.fraglist[no].start = mCurPos.nextAddr();
        mCurPos += std::ceil(trackGroups);
        mpToc->mTracks.fraglist[no].end   = static_cast<toc::discaddr>(mCurPos);
    }

    mpToc->mTracks.free_track_slot   = no + 1;

    setTrackTitle(no, title);
    setTrackTStamp(no);

    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Sets the track title.
//!
//! @param[in]  no     The track number
//! @param[in]  title  The title
//!
//! @return     0
//--------------------------------------------------------------------------
int CNetMdTOC::setTrackTitle(uint8_t no, const std::string& title)
{
    mpToc->mTitles.titlemap[no] = mpToc->mTitles.free_title_slot;

    for(size_t sz = 0; sz < title.size(); sz += 7)
    {
        uint8_t i = mpToc->mTitles.free_title_slot;
        char* pTitle = mpToc->mTitles.titlelist[i].title;
        memset(pTitle, 0, 7);
        size_t toCpy = ((title.size() - sz) < 7) ? (title.size() - sz) : 7;
        memcpy(pTitle, title.c_str() + sz, toCpy);

        mpToc->mTitles.free_title_slot ++;

        if ((title.size() - (sz + toCpy)) > 0)
        {
            mpToc->mTitles.titlelist[i].link = mpToc->mTitles.free_title_slot;
        }
        else
        {
            mpToc->mTitles.titlelist[i].link = 0;
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Sets the track time stamp.
//!
//! @param[in]  no    The new value
//!
//! @return     0
//--------------------------------------------------------------------------
int CNetMdTOC::setTrackTStamp(int no)
{
    mpToc->mTimes.timemap[no] = no;

    mpToc->mTimes.timelist[no].d  = 0x03;
    mpToc->mTimes.timelist[no].mo = 0x05;
    mpToc->mTimes.timelist[no].y  = 0x23;
    mpToc->mTimes.timelist[no].h  = 0x11;
    mpToc->mTimes.timelist[no].m  = 0x11;
    mpToc->mTimes.timelist[no].s  = 0x11;
    mpToc->mTimes.timelist[no].signature = toBigEndian(toc::SIGNATURE);

    mpToc->mTimes.free_time_slot = no + 1;
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  title  The title
//!
//! @return     0 -> ok; -1 -> error
//--------------------------------------------------------------------------
int CNetMdTOC::setDiscTitle(const std::string& title)
{
    if (mpToc == nullptr)
    {
        return -1;
    }

    // first part will be wriiten to titlelist[0]
    mpToc->mTitles.titlemap[0] = 0;
    size_t i = 0;

    for(size_t sz = 0; sz < title.size(); sz += 7)
    {
        char* pTitle = mpToc->mTitles.titlelist[i].title;
        memset(pTitle, 0, 7);
        size_t toCpy = ((title.size() - sz) < 7) ? (title.size() - sz) : 7;
        memcpy(pTitle, title.c_str() + sz, toCpy);

        if (i)
        {
            mpToc->mTitles.free_title_slot ++;
        }


        if ((title.size() - (sz + toCpy)) > 0)
        {
            mpToc->mTitles.titlelist[i].link = mpToc->mTitles.free_title_slot;
        }
        else
        {
            mpToc->mTitles.titlelist[i].link = 0;
        }

        i = mpToc->mTitles.free_title_slot;
    }

    return 0;
}

//--------------------------------------------------------------------------
//! @brief      get track count
//!
//! @return     number of tracks
//--------------------------------------------------------------------------
int CNetMdTOC::trackCount() const
{
    if (mpToc)
    {
        return mpToc->mTracks.ntracks;
    }
    return -1;
}

//--------------------------------------------------------------------------
//! @brief      get MD title
//!
//! @return     title
//--------------------------------------------------------------------------
std::string CNetMdTOC::discTitle() const
{
    return trackTitle(0);
}

//--------------------------------------------------------------------------
//! @brief      get track title
//!
//! @param[in]  trackNo  The track number
//!
//! @return     title
//--------------------------------------------------------------------------
std::string CNetMdTOC::trackTitle(int trackNo) const
{
    std::string s;
    if (mpToc)
    {
        int cell = mpToc->mTitles.titlemap[trackNo];
        do
        {
            for (int i = 0; i < 7; i++)
            {
                if (mpToc->mTitles.titlelist[cell].title[i] != '\0')
                {
                    s.push_back(mpToc->mTitles.titlelist[cell].title[i]);
                }
                else
                {
                    break;
                }
            }
            cell = mpToc->mTitles.titlelist[cell].link;
        }
        while(cell != 0);
    }
    return s;
}

//--------------------------------------------------------------------------
//! @brief      get track info
//!
//! @param[in]  trackNo  The track number
//!
//! @return     track info
//--------------------------------------------------------------------------
std::string CNetMdTOC::trackInfo(int trackNo) const
{
    std::ostringstream oss;

    if (mpToc)
    {
        oss << trackTitle(trackNo) << std::endl;

        int fragment = mpToc->mTracks.trackmap[trackNo];

        do
        {
            CSG begin(mpToc->mTracks.fraglist[fragment].start);
            CSG end(mpToc->mTracks.fraglist[fragment].end);

            oss << "Fragment #" << fragment << ": begin=" << static_cast<uint32_t>(begin)
                << ", end=" << static_cast<uint32_t>(end) << ", mode=0x" << std::hex
                << static_cast<int>(mpToc->mTracks.fraglist[fragment].mode) << std::dec
                << ", length: " << CSG::toTime(static_cast<uint32_t>(end) - static_cast<uint32_t>(begin),
                                               mpToc->mTracks.fraglist[fragment].mode & toc::F_STEREO)
                << std::endl;

            fragment = mpToc->mTracks.fraglist[fragment].link;
        }
        while(fragment != 0);
    }

    return oss.str();
}

//--------------------------------------------------------------------------
//! @brief      get disc info
//!
//! @return     disc info
//--------------------------------------------------------------------------
std::string CNetMdTOC::discInfo() const
{
    std::ostringstream oss;

    if (mpToc)
    {
        oss << discTitle() << std::endl;
        oss << "Track Count: " << static_cast<int>(mpToc->mTracks.ntracks)
            << ", next free: " << static_cast<int>(mpToc->mTracks.free_track_slot)
            << ", non empty: 0x" << std::hex << static_cast<int>(mpToc->mTracks.nonempty) << std::dec
            << ", signature: 0x" << std::hex << fromBigEndian(mpToc->mTracks.sign);
    }

    return oss.str();
}

} // ~netmd