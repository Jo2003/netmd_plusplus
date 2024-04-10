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

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "CNetMdTOC.h"
#include "log.h"
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
    :mpToc(nullptr), mCurPos(0), mDAOTrack(0), mDAOGroups(0)
{
    import(trackCount, lenInMs, data);
}

//--------------------------------------------------------------------------
//! @brief      Destroys the object.
//--------------------------------------------------------------------------
CNetMdTOC::~CNetMdTOC()
{
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
    mDAOTrack    = 0;
    mCurPos      = 0;
    mpToc        = nullptr;
    mDAOFragments.clear();

    if (data)
    {
        mpToc = reinterpret_cast<toc::TOC*>(data);

        // find last track since it contains the data we
        // need to manipulate
        mDAOTrack = mpToc->mTracks.ntracks;

        // collect all used track fragments
        uint8_t link = mpToc->mTracks.trackmap[mDAOTrack];

        do
        {
            toc::fragment& fragment = mpToc->mTracks.fraglist[link];
            CSG gStart(fragment.start);
            CSG gEnd(fragment.end);
            link = fragment.link;
            mDAOFragments.push_back({static_cast<uint32_t>(gStart), static_cast<uint32_t>(gEnd)});
        }
        while(link);

        // get whole groups count
        mDAOGroups = daoGroupCount();
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
//! @param[in]  lengthMs  The length in milliseconds
//! @param[in]  title     The track title
//! @param[in]  tstamp    The time stamp
//!
//! @return     0 -> ok; -1 -> error
//--------------------------------------------------------------------------
int CNetMdTOC::addTrack(uint8_t no, uint32_t lengthMs, const std::string& title, const std::time_t& tstamp)
{
    if (mpToc == nullptr)
    {
        return -1;
    }

    if (no == 1)
    {
        // free used DAO track fragment in TOC
        mpToc->mTracks.trackmap[mDAOTrack] = 0;
    }

    // track audio data splitting...
    float    allGroups   = mDAOGroups;
    uint32_t trackGroups = std::round(static_cast<float>(lengthMs) * allGroups /  static_cast<float>(mLengthInMs));
    int      currTrack   = mDAOTrack + no - 1;
    int      fragNo      = nextFreeTrackFragment();

    // Splitting can be done on each sound group.
    // Most important is the addressing scheme.

    mpToc->mTracks.ntracks = currTrack;
    mpToc->mTracks.trackmap[currTrack] = fragNo;

    DAOFragments trFrags = getTrackFragments(no, trackGroups);

    for (auto it = trFrags.begin(); it != trFrags.end(); it ++)
    {
        auto& fragment = mpToc->mTracks.fraglist[fragNo];
        // make sure any old link is deleted!
        fragment.link  = 0;
        fragment.start = static_cast<toc::discaddr>(CSG(it->mStart));
        fragment.end   = static_cast<toc::discaddr>(CSG(it->mEnd));
        fragment.mode  = toc::DEF_TRACK_MODE;
        fragNo         = (it == (trFrags.end() - 1)) ? 0 : nextFreeTrackFragment();
        fragment.link  = fragNo;
    }

    setTrackTitle(currTrack, title);
    setTrackTStamp(currTrack, tstamp);

    mpToc->mTracks.free_track_slot = nextFreeTrackFragment();

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
    if (mpToc == nullptr)
    {
        return -1;
    }

    if (no == mDAOTrack)
    {
        mpToc->mTitles.titlemap[no] = 0;
        mpToc->mTitles.free_title_slot = nextFreeTitleCell();
    }
    else if (no == 0)
    {
        mpToc->mTitles.free_title_slot = 0;
    }

    mpToc->mTitles.titlemap[no] = mpToc->mTitles.free_title_slot;

    for(size_t sz = 0; sz < title.size(); sz += 7)
    {
        uint8_t i = mpToc->mTitles.free_title_slot;
        toc::titlecell& cell = mpToc->mTitles.titlelist[i];

        // make sure any old link is deleted!
        cell.link = 0;
        memset(cell.title, 0, 7);
        size_t toCpy = std::min<uint32_t>(title.size() - sz, 7);
        memcpy(cell.title, title.c_str() + sz, toCpy);

        mpToc->mTitles.free_title_slot = nextFreeTitleCell();

        if ((title.size() - (sz + toCpy)) > 0)
        {
            cell.link = mpToc->mTitles.free_title_slot;
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Sets the track time stamp.
//!
//! @param[in]  no     The track number
//! @param[in]  tstamp The time stamp
//!
//! @return     0
//--------------------------------------------------------------------------
int CNetMdTOC::setTrackTStamp(int no, const std::time_t& tstamp)
{
    mpToc->mTimes.timemap[no] = no;

    auto* tm = gmtime(&tstamp);

#define mDecToHexDate(x__) (((x__ / 10) << 4) | ((x__ % 10) & 0xf))

    mpToc->mTimes.timelist[no].d  = mDecToHexDate(tm->tm_mday);         // 1 ... 31
    mpToc->mTimes.timelist[no].mo = mDecToHexDate((tm->tm_mon + 1));    // 0 ... 11
    mpToc->mTimes.timelist[no].y  = mDecToHexDate((tm->tm_year % 100)); // since 1900
    mpToc->mTimes.timelist[no].h  = mDecToHexDate(tm->tm_hour);         // 0 ... 23
    mpToc->mTimes.timelist[no].m  = mDecToHexDate(tm->tm_min);          // 0 ... 59
    mpToc->mTimes.timelist[no].s  = mDecToHexDate(tm->tm_sec);          // 0 ... 59
    mpToc->mTimes.timelist[no].signature = toBigEndian(toc::SIGNATURE);

    mpToc->mTimes.free_time_slot = no + 1;
#undef mDecToHexDate
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
    return setTrackTitle(0, title);
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

            uint32_t groups = static_cast<uint32_t>(end) - static_cast<uint32_t>(begin);
            bool     stereo = !!(mpToc->mTracks.fraglist[fragment].mode & toc::F_STEREO);

            oss << "Fragment #" << fragment << ": begin=" << static_cast<uint32_t>(begin)
                << " (" << static_cast<std::string>(begin) << "), end=" << static_cast<uint32_t>(end)
                << " (" << static_cast<std::string>(end) << "), groups: " << groups << ", mode=0x" << std::hex
                << static_cast<int>(mpToc->mTracks.fraglist[fragment].mode) << std::dec
                << ", length: " << CSG::toTime(groups, stereo) << std::endl;

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
//! @brief      get next free title cell
//!
//! @return     cell number or -1 on error
//--------------------------------------------------------------------------
int CNetMdTOC::nextFreeTitleCell()
{
    if (!mpToc)
    {
        return -1;
    }

    std::vector<uint8_t> used;

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
            return i;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------
//! @brief      get next free track fragment
//!
//! @return     fragment number or -1 on error
//--------------------------------------------------------------------------
int CNetMdTOC::nextFreeTrackFragment()
{
    if (!mpToc)
    {
        return -1;
    }

    std::vector<uint8_t> used;

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
            return i;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------
//! @brief      get group count of DAO track
//!
//! @return     group count
//--------------------------------------------------------------------------
uint32_t CNetMdTOC::daoGroupCount() const
{
    uint32_t ret = 0;
    for (const auto& f : mDAOFragments)
    {
        ret += ((f.mEnd - f.mStart) + 1);
    }
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Gets the track fragments.
//!
//! @param[in]  trackNo  The track number
//! @param[in]  groups   The groups count of the track
//!
//! @return     The track fragments.
//--------------------------------------------------------------------------
CNetMdTOC::DAOFragments CNetMdTOC::getTrackFragments(int trackNo, uint32_t groups)
{
    DAOFragments ret;
    uint32_t start  = 0;
    uint32_t end    = 0;

    mLOG(DEBUG) << "Track: " << trackNo << ", song groups: " << groups;

    for (auto it = mDAOFragments.begin(); it != mDAOFragments.end() && groups;)
    {
        mLOG(DEBUG) << "Handling DAO fragment (" << it->mStart << " ... " << it->mEnd << ")";
        if (mCurPos == 0)
        {
            mCurPos = it->mStart;
        }

        start   = mCurPos;
        end     = start + groups - 1;
        // take care for DAO fragment border crossing
        end     = std::min(end, it->mEnd);
        mCurPos = end + 1;
        groups -= ((end - start) + 1);

        // We may have some rounding offset here.
        // So, on last track and last fragmemt we use the end value anyway.
        if ((trackNo == mTracksCount) && (it == (mDAOFragments.end() - 1)))
        {
            end = it->mEnd;
            groups = 0;
        }

        ret.push_back({start, end});

        mLOG(DEBUG) << "Track: " << trackNo << ", fragment: " << ret.size() << ", start: " << start
                    << ", end: " << end << ", groups still to place: " << groups;

        if (end == it->mEnd)
        {
            // current position will be the start of the next fragment
            mCurPos = 0;

            // DAO fragment completely used,
            // erase this fragment and update iterator.
            it = mDAOFragments.erase(it);
        }
    }

    return ret;
}

} // ~netmd
