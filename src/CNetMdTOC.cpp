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

#include <vector>
#include <algorithm>
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
    :mpToc(nullptr), mAudioStart(0), mAudioEnd(0), mpCurPos(nullptr), mDAOTrack(0)
{
    import(trackCount, lenInMs, data);
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdTOC::~CNetMdTOC()
{
    if (mpCurPos != nullptr)
    {
        delete mpCurPos;
    }
}

//--------------------------------------------------------------------------
//! @brief      import TOC data
//!
//! @param[in]  trackCount  The track count
//! @param[in]  lenInMs     The length in milliseconds
//! @param      data        The TOC data
//--------------------------------------------------------------------------
void CNetMdTOC::import(int trackCount, uint32_t lenInMs, uint8_t* data)
{
    mTracksCount = trackCount;
    mLengthInMs  = lenInMs;
    mAudioStart  = 0;
    mAudioEnd    = 0;
    mDAOTrack    = 0;
    mpToc        = nullptr;

    if (mpCurPos)
    {
        delete mpCurPos;
        mpCurPos = nullptr;
    }

    if (data)
    {
        mpToc = reinterpret_cast<toc::TOC*>(data);

        // find last track since it contains the data we
        // need to manipulate
        mDAOTrack = mpToc->mTracks.ntracks;
        CSG csg(mpToc->mTracks.fraglist[mpToc->mTracks.trackmap[mDAOTrack]].start);
        mAudioStart = static_cast<uint32_t>(csg);
        mpCurPos    = new CSG(mAudioStart);

        csg = mpToc->mTracks.fraglist[mpToc->mTracks.trackmap[mDAOTrack]].end;
        mAudioEnd = static_cast<uint32_t>(csg);
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
    int   currTrack   = mDAOTrack + no - 1;

    // for our first track the fragment was already given
    int   fragNo      = (no == 1) ? mpToc->mTracks.trackmap[currTrack] : nextFreeTrackFragment();

    mpToc->mTracks.ntracks = currTrack;
    mpToc->mTracks.trackmap[currTrack] = fragNo;

    auto& fragment = mpToc->mTracks.fraglist[fragNo];
    fragment.mode = toc::DEF_TRACK_MODE;
    fragment.link = 0;

    if (no == 1)
    {
        // start is set ...
        *mpCurPos += std::ceil(trackGroups) - 1;
        fragment.end  = static_cast<toc::discaddr>(*mpCurPos);
    }
    else if (no == mTracksCount)
    {
        fragment.start = mpCurPos->nextAddr();
        fragment.end   = static_cast<toc::discaddr>(CSG(mAudioEnd));
    }
    else
    {
        fragment.start = mpCurPos->nextAddr();
        *mpCurPos += std::ceil(trackGroups);
        fragment.end   = static_cast<toc::discaddr>(*mpCurPos);
    }

    setTrackTitle(currTrack, title);
    setTrackTStamp(currTrack);

    mpToc->mTracks.free_track_slot = nextFreeTrackFragment((no == 1) ? true : false);

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
    if (no == mDAOTrack)
    {
        freeTitle(no);
        mpToc->mTitles.free_title_slot = nextFreeTitleCell(true);
    }

    mpToc->mTitles.titlemap[no] = mpToc->mTitles.free_title_slot;

    for(size_t sz = 0; sz < title.size(); sz += 7)
    {
        uint8_t i = mpToc->mTitles.free_title_slot;
        char* pTitle = mpToc->mTitles.titlelist[i].title;
        memset(pTitle, 0, 7);
        size_t toCpy = ((title.size() - sz) < 7) ? (title.size() - sz) : 7;
        memcpy(pTitle, title.c_str() + sz, toCpy);

        mpToc->mTitles.free_title_slot = nextFreeTitleCell();

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
    freeTitle(0);
    mpToc->mTitles.titlemap[0] = 0;
    size_t i = 0;

    for(size_t sz = 0; sz < title.size(); sz += 7)
    {
        char* pTitle = mpToc->mTitles.titlelist[i].title;
        memset(pTitle, 0, 7);
        size_t toCpy = ((title.size() - sz) < 7) ? (title.size() - sz) : 7;
        memcpy(pTitle, title.c_str() + sz, toCpy);
        mpToc->mTitles.free_title_slot = nextFreeTitleCell();

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

//--------------------------------------------------------------------------
//! @brief      free segment starting at segment
//!
//! @param[in]  segment  The segment index
//--------------------------------------------------------------------------
void CNetMdTOC::freeTitle(int segment)
{
    if (!mpToc)
    {
        return;
    }

    uint8_t link = mpToc->mTitles.titlemap[segment];

    do
    {
        auto& cell = mpToc->mTitles.titlelist[link];
        link = cell.link;
        memset(&cell, 0, sizeof(toc::titlecell));
    }
    while(link);
}

//--------------------------------------------------------------------------
//! @brief      get next free title cell
//!
//! @param[in]  cleanup  if true clear unused cells
//!
//! @return     cell number or -1 on error
//--------------------------------------------------------------------------
int CNetMdTOC::nextFreeTitleCell(bool cleanup)
{
    if (!mpToc)
    {
        return -1;
    }

    std::vector<uint8_t> used;
    std::vector<uint8_t> unused;
    for (int i = 0; i <= mpToc->mTracks.ntracks; i++)
    {
        uint8_t link = mpToc->mTitles.titlemap[i];

        do
        {
            used.push_back(link);
            link = mpToc->mTitles.titlelist[link].link;
        }
        while(link);
    }

    // cell 0 is never free
    for (int i = 1; i <= 255; i++)
    {
        if (std::find(used.begin(), used.end(), i) == used.end())
        {
            unused.push_back(i);
        }
    }

    if (cleanup)
    {
        for (const auto& c : unused)
        {
            auto& cell = mpToc->mTitles.titlelist[c];
            memset(&cell, 0, sizeof(toc::titlecell));
        }
    }

    return unused.empty() ? -1 : unused.at(0);
}

//--------------------------------------------------------------------------
//! @brief      get next free track fragment
//!
//! @param[in]  cleanup  if true clear unused cells
//!
//! @return     fragment number or -1 on error
//--------------------------------------------------------------------------
int CNetMdTOC::nextFreeTrackFragment(bool cleanup)
{
    if (!mpToc)
    {
        return -1;
    }

    std::vector<uint8_t> used;
    std::vector<uint8_t> unused;
    for (int i = 0; i <= mpToc->mTracks.ntracks; i++)
    {
        uint8_t link = mpToc->mTracks.trackmap[i];

        do
        {
            used.push_back(link);
            link = mpToc->mTracks.fraglist[link].link;
        }
        while(link);
    }

    // fragment zero is never used
    for (int i = 1; i <= 255; i++)
    {
        if (std::find(used.begin(), used.end(), i) == used.end())
        {
            unused.push_back(i);
        }
    }

    if (cleanup)
    {
        for (const auto& f : unused)
        {
            auto& fragment = mpToc->mTracks.fraglist[f];
            memset(&fragment, 0, sizeof(toc::fragment));
        }
    }

    return unused.empty() ? -1 : unused.at(0);
}

} // ~netmd