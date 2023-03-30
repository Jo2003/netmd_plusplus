/**
 * Copyright (C) 2021 Jo2003 (olenka.joerg@gmail.com)
 * This file is part of netmd
 *
 * cd2netmd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cd2netmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 */
#include <algorithm>
#include <cstring>
#include <regex>
#include <sstream>
#include "CMDiscHeader.h"
#include "log.h"

/// log configuration
structlog LOGCFG = {true, DEBUG, nullptr};

//-----------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//-----------------------------------------------------------------------------
CMDiscHeader::CMDiscHeader() : mGroupId(0), 
    mpCStringHeader(nullptr), mpLastString(nullptr)
{
    // add title entry
    mGroups.push_back({mGroupId++, 0, -1, ""});
}

//-----------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//!
//! @param[in]  header  The RAW disc header as string
//-----------------------------------------------------------------------------
CMDiscHeader::CMDiscHeader(const std::string& header) : mGroupId(0), 
    mpCStringHeader(nullptr), mpLastString(nullptr)
{
    fromString(header);
}

//-----------------------------------------------------------------------------
//! @brief      Destroys the object.
//-----------------------------------------------------------------------------
CMDiscHeader::~CMDiscHeader()
{
    if (mpCStringHeader != nullptr)
    {
        free(mpCStringHeader);
        mpCStringHeader = nullptr;
    }

    if (mpLastString != nullptr)
    {
        free(mpLastString);
        mpLastString = nullptr;
    }
}

//-----------------------------------------------------------------------------
//! @brief      create header from string
//!
//! @param[in]  header  The RAW disc header as string
//!
//! @return     { description_of_the_return_value }
//-----------------------------------------------------------------------------
int CMDiscHeader::fromString(const std::string& header)
{
    int ret;
    constexpr int GROUP_TRACKS = 1;
    constexpr int GROUP_NAME   = 2;

    mGroups.clear();

    // always add disc title!
    mGroups.push_back({mGroupId++, 0, -1, ""});

    // good ol' plain disc header?
    if (!header.empty() && (header.find("//") == std::string::npos))
    {
        mGroups[0].mName = header;
    }
    else
    {
        // we all love them: Regular Expressions!
        std::regex pattern(R"(([0-9-]+);([^/]*)//)", std::regex_constants::extended);
        auto start = std::sregex_iterator(header.begin(), header.end(), pattern);
        auto end   = std::sregex_iterator();

        if (std::distance(start, end))
        {
            Group_t                group;
            std::string::size_type dash;
            std::smatch            match;
            std::string            numb;

            for (auto m = start; m != end; m++)
            {
                match = *m;
                numb  = match[GROUP_TRACKS].str();

                // 0 -> whole match
                // 1 -> track(s)
                // 2 -> group title
                if (numb == "0")
                {
                    // disc title ...
                    mGroups[0].mName = match[GROUP_NAME].str();
                }
                else
                {
                    group.mFirst = -1;
                    group.mLast  = -1;
                    group.mName  = match[GROUP_NAME].str();

                    if ((dash = numb.find('-')) != std::string::npos)
                    {
                        group.mFirst = atoi(numb.substr(0, dash).c_str());
                        group.mLast  = atoi(numb.substr(dash + 1).c_str());
                    }
                    else
                    {
                        group.mFirst = atoi(numb.c_str());
                    }

                    // don't add unused groups!
                    if (group.mFirst != -1)
                    {
                        group.mGid = mGroupId++;
                        mGroups.push_back(group);
                    }
                }
            }
        }
    }

    if ((ret = sanityCheck(mGroups)) == 0)
    {
        listGroups();
    }

    return ret;
}

//-----------------------------------------------------------------------------
//! @brief      check groups / tracks for sanity
//!
//! @param[in]  grps  The grps
//!
//! @return     0 -> all well; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::sanityCheck(const Groups_t& grps) const
{
    int last = 0, ret = 0;

    Groups_t tmpGrps = grps;

    std::sort(tmpGrps.begin(), tmpGrps.end(), 
        &CMDiscHeader::groupCompare);

    for(const auto& g : tmpGrps)
    {
        if ((g.mFirst == 0) && (g.mLast != -1))
        {
            LOG(ERROR) << "Title group can't have a last entry!";
            ret = -1;
            break;
        }
        else if ((g.mFirst == -1) && (g.mLast != -1))
        {
            LOG(ERROR) << "An empty group can't have a last entry!";
            ret = -1;
            break;
        }
        else if ((g.mFirst > g.mLast) && (g.mLast != -1))
        {
            LOG(ERROR) << "First track number can't be larger than last track number!";
            ret = -1;
            break;
        }
        else if ((g.mFirst > 0) && (g.mFirst <= last))
        {
            LOG(ERROR) << "Some groups share the same track numbers!";
            ret = -1;
            break;
        }

        last  = (g.mLast == -1) ? g.mFirst : g.mLast;
    }

    return ret;
}

//-----------------------------------------------------------------------------
//! @brief      compare function used to sort groups in header
//!
//! @param[in]  a     group a
//! @param[in]  b     group b
//!
//! @return     true if a less b
//-----------------------------------------------------------------------------
bool CMDiscHeader::groupCompare(const Group_t& a, const Group_t& b)
{
    // empty groups (first == -1) must be last
    return ((a.mFirst < b.mFirst) && (a.mFirst != -1));
}

//-----------------------------------------------------------------------------
//! @brief      Returns a string representation of the object.
//!
//! @return     String representation of the object.
//-----------------------------------------------------------------------------
std::string CMDiscHeader::toString()
{
    std::ostringstream oss;

    if (mpCStringHeader != nullptr)
    {
        free(mpCStringHeader);
        mpCStringHeader = nullptr;
    }

    Groups_t tmpGrps = mGroups;

    Group_t& title = tmpGrps.at(0);

    if (title.mFirst == 0)
    {
        if (tmpGrps.size() == 1)
        {
            mpCStringHeader = strdup(title.mName.c_str());
            return title.mName;
        }
        else
        {
            oss << "0;" << title.mName << "//";
            tmpGrps.erase(tmpGrps.begin());
        }
    }

    std::sort(tmpGrps.begin(), tmpGrps.end(), 
        &CMDiscHeader::groupCompare);

    for (const auto& g : tmpGrps)
    {
        if (g.mFirst != -1)
        {
            oss << g.mFirst;
        }

        if (g.mLast != -1)
        {
            oss << "-" << g.mLast;
        }

        oss << ";" << g.mName << "//";
    }

    mpCStringHeader = strdup(oss.str().c_str());

    return oss.str();
}

//-----------------------------------------------------------------------------
//! @brief      Adds a group.
//!
//! @param[in]  name   The name
//! @param[in]  first  The first track (optional)
//! @param[in]  last   The last track (otional)
//!
//! @return     >= 0 -> group idx; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::addGroup(const std::string& name, int16_t first, int16_t last)
{
    Groups_t tmpGrps = mGroups;
    tmpGrps.push_back({mGroupId++, first, last, name});

    if (sanityCheck(tmpGrps) == 0)
    {
        LOG(DEBUG) << "Sanity check for 'addGroup()' successful!";
        mGroups = tmpGrps;
        return mGroupId - 1;
    }
    else
    {
        LOG(ERROR) << "Sanity check for 'addGroup()' not(!) successful!";
    }

    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      list/print all groups
//-----------------------------------------------------------------------------
void CMDiscHeader::listGroups() const
{
    std::ostringstream oss;
    for (const auto& g : mGroups)
    {
        oss.clear();
        oss.str("");
        oss << "Group " << g.mGid;

        if (g.mName.empty())
        {
            oss << " <untitled>";
        }
        else
        {
            oss << " '" << g.mName << "'";
        }

        if (g.mFirst == 0)
        {
            oss << ", disc title";
        }
        
        if (g.mFirst > 0)
        {
            oss << ", track(s) " << g.mFirst;
        }
        
        if (g.mLast != -1)
        {
            oss << " - " << g.mLast;
        }

        LOG(ERROR) << oss.str();
    }
}

//-----------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::addTrackToGroup(int gid, int16_t track)
{
    Groups_t tmpGrps = mGroups;
    int16_t first, last;
    bool changed = false;

    for (auto& g : tmpGrps)
    {
        if (g.mGid == gid)
        {
            if ((g.mFirst == -1) && (g.mLast == -1))
            {
                g.mFirst = track;
                changed  = true;
                break;
            }

            first = g.mFirst;
            last  = (g.mLast == -1) ? first : g.mLast;

            if ((first - track) == 1)
            {
                g.mFirst = track;
                changed  = true;
            }
            else if ((track - last) == 1)
            {
                g.mLast = track;
                changed  = true;
            }
            break;
        }
    }

    if (changed && (sanityCheck(tmpGrps) == 0))
    {
        mGroups = tmpGrps;
        return 0;
    }

    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::delTrackFromGroup(int gid, int16_t track)
{
    Groups_t tmpGrps = mGroups;
    int16_t first, last;
    bool changed = false;

    Groups_t::iterator it;
    for (it = tmpGrps.begin(); it != tmpGrps.end();)
    {
        first   = it->mFirst;
        last    = (it->mLast == -1) ? first : it->mLast;
        
        if (it->mGid == gid)
        {
            if ((track >= first) && (track <= last))
            {
                changed = true;

                last --;

                if (last < first)
                {
                    // erase empty group
                    it = tmpGrps.erase(it);
                    continue;
                }

                if (first == last)
                {
                    last = -1;
                }

                it->mLast = last;
            }
            else
            {
                // track not found in group -> no change!
                break;
            }
        }
        else if ((it->mGid > gid) && (first > track))
        {
            changed = true;
            it->mFirst --;
            if (last != -1)
            {
                it->mLast --;
            }
        }
        it ++;
    }

    if (changed && (sanityCheck(tmpGrps) == 0))
    {
        mGroups = tmpGrps;
        return 0;
    }

    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track
//!
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::delTrack(int16_t track)
{
    Groups_t tmpGrps = mGroups;
    int16_t first, last;
    bool changed = false;

    Groups_t::iterator it;

    for (it = tmpGrps.begin(); it != tmpGrps.end();)
    {
        first   = it->mFirst;
        last    = (it->mLast == -1) ? first : it->mLast;

        if ((track >= first) && (track <= last))
        {
            changed = true;

            last --;

            if (last < first)
            {
                // erase empty group
                it = tmpGrps.erase(it);
                continue;
            }

            if (first == last)
            {
                last = -1;
            }

            it->mLast = last;
        }
        else if (first > track)
        {
            changed = true;
            it->mFirst --;
            if (last != -1)
            {
                it->mLast --;
            }
        }
        it ++;
    }

    if (changed && (sanityCheck(tmpGrps) == 0))
    {
        mGroups = tmpGrps;
        return 0;
    }

    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::delGroup(int gid)
{
    int ret = -1;

    Groups_t::const_iterator cit;

    for (cit = mGroups.cbegin(); cit != mGroups.cend(); cit ++)
    {
        if (cit->mGid == gid)
        {
            LOG(DEBUG) << "Delete group " << cit->mGid << ", name: '" << cit->mName << "'";
            mGroups.erase(cit);
            ret = 0;
            break;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::setDiscTitle(const std::string& title)
{
    mGroups.at(0).mName = title;
    return 0;
}

//-----------------------------------------------------------------------------
//! @brief      gets the disc title.
//!
//! @return     disc title
//-----------------------------------------------------------------------------
std::string CMDiscHeader::discTitle()
{
    if (mpLastString != nullptr)
    {
        free(mpLastString);
        mpLastString = nullptr;

    }
    mpLastString = strdup(mGroups.at(0).mName.c_str());
    return mGroups.at(0).mName;
}

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::renameGroup(int gid, const std::string& title)
{
    for (auto& g : mGroups)
    {
        if (g.mGid == gid)
        {
            g.mName = title;
            return 0;
        }
    }

    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      return the C string header
//!
//! @return     C string with MD header data
//-----------------------------------------------------------------------------
const char* CMDiscHeader::stringHeader()
{
    return mpCStringHeader;
}

//-----------------------------------------------------------------------------
//! @brief      returns last buildt C string
//!
//! @return     C string
//-----------------------------------------------------------------------------
const char* CMDiscHeader::lastString()
{
    return mpLastString;
}

//-----------------------------------------------------------------------------
//! @brief      get the group title for track
//!
//! @param[in]  track  The track
//! @param      pGid   The gid
//!
//! @return     group title or empty string
//-----------------------------------------------------------------------------
std::string CMDiscHeader::trackGroup(int16_t track, int16_t* pGid)
{
    std::string ret;
    int16_t first, last;
    *pGid = -1;

    if (mpLastString != nullptr)
    {
        free(mpLastString);
        mpLastString = nullptr;
    }

    for (const auto& g : mGroups)
    {
        first =  g.mFirst;
        last  = (g.mLast == -1) ? g.mFirst : g.mLast;

        if ((track >= first) && (track <= last))
        {
            ret   = g.mName;
            *pGid = g.mGid;

            mpLastString = strdup(ret.c_str());
            break;
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------
//! @brief      ungroup a track
//!
//! @param[in]  track  The track
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int CMDiscHeader::unGroup(int16_t track)
{
    int16_t first, last;
    for (const auto& g : mGroups)
    {
        first =  g.mFirst;
        last  = (g.mLast == -1) ? g.mFirst : g.mLast;

        if ((track >= first) && (track <= last))
        {
            return delTrackFromGroup(g.mGid, track);
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      get all groups
//!
//! @return     const reference to groups
//-----------------------------------------------------------------------------
CMDiscHeader::Groups_t CMDiscHeader::groups() const
{
    Groups_t tmpGrps = mGroups;
    std::sort(tmpGrps.begin(), tmpGrps.end(), 
        &CMDiscHeader::groupCompare);
    return tmpGrps;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! @brief      Creates a md header.
//!
//! @param[in]  content  The content
//!
//! @return     The handle md header.
//------------------------------------------------------------------------------
HndMdHdr create_md_header(const char* content)
{
    return static_cast<HndMdHdr>(new CMDiscHeader((content == nullptr) ? "" : content));
}

//------------------------------------------------------------------------------
//! @brief         free the MD header
//!
//! @param[in/out] hdl   handle to MD header
//------------------------------------------------------------------------------
void free_md_header(HndMdHdr* hdl)
{
    if (*hdl != nullptr)
    {
        delete static_cast<CMDiscHeader*>(*hdl);
        *hdl = nullptr;
    }
}

//------------------------------------------------------------------------------
//! @brief      create C string from MD header
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     c String or NULL
//------------------------------------------------------------------------------
const char* md_header_to_string(HndMdHdr hdl)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        static_cast<void>(pMDH->toString());
        return pMDH->stringHeader();
    }
    return "";
}

//------------------------------------------------------------------------------
//! @brief      add a group to the MD header
//!
//! @param[in]  hdl    The MD header handlehdl
//! @param[in]  name   The name
//! @param[in]  first  The first
//! @param[in]  last   The last
//!
//! @return     >-1 -> group id; else -> error
//------------------------------------------------------------------------------
int md_header_add_group(HndMdHdr hdl, const char* name, int16_t first, int16_t last)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->addGroup(name, first, last);
    }
    return -1;
}


//------------------------------------------------------------------------------
//! @brief      list groups in MD header
//!
//! @param[in]  hdl   The MD header handle
//------------------------------------------------------------------------------
void md_header_list_groups(HndMdHdr hdl)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        pMDH->listGroups();
    }
}

//------------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_add_track_to_group(HndMdHdr hdl, int gid, int16_t track)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->addTrackToGroup(gid, track);
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track_from_group(HndMdHdr hdl, int gid, int16_t track)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->delTrackFromGroup(gid, track);
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track(HndMdHdr hdl, int16_t track)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->delTrack(track);
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  hdl   The MD header handle
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_group(HndMdHdr hdl, int gid)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->delGroup(gid);
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_set_disc_title(HndMdHdr hdl, const char* title)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->setDiscTitle(title);
    }
    return -1;
}

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_rename_group(HndMdHdr hdl, int gid, const char* title)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->renameGroup(gid, title);
    }
    return -1;
}

//------------------------------------------------------------------------------
//! @brief      get disc title
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     C string
//------------------------------------------------------------------------------
const char* md_header_disc_title(HndMdHdr hdl)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        if (!pMDH->discTitle().empty())
        {
            return pMDH->lastString();
        }
        
    }
    return "<untitled>";
}

//------------------------------------------------------------------------------
//! @brief      get group name for track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track number
//! @param[out] pGid   The buffer for gid
//!
//! @return     C string or nullptr
//------------------------------------------------------------------------------
const char* md_header_track_group(HndMdHdr hdl, int16_t track, int16_t* pGid)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        if (!pMDH->trackGroup(track, pGid).empty())
        {
            return pMDH->lastString();
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
//! @brief      unhroup track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_ungroup_track(HndMdHdr hdl, int16_t track)
{
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        return pMDH->unGroup(track);
    }
    return -1;
}

//------------------------------------------------------------------------------
//! @brief      export all minidisc groups
//!
//! @param[in]  hdl The MD header handle
//!
//! @return     array of groups
//------------------------------------------------------------------------------
MDGroups* md_header_groups(HndMdHdr hdl)
{
    MDGroups* groups = nullptr;
    CMDiscHeader* pMDH = static_cast<CMDiscHeader*>(hdl);
    if (pMDH != nullptr)
    {
        CMDiscHeader::Groups_t g = pMDH->groups();
        if (!g.empty())
        {
            groups = new MDGroups;

            if (groups != nullptr)
            {
                groups->mCount = g.size();
                groups->mpGroups = new MDGroup[groups->mCount];

                if (groups->mpGroups != nullptr)
                {
                    MDGroup* pCGroup = groups->mpGroups;
                    for (const auto& a : g)
                    {
                        pCGroup->mGid   = a.mGid;
                        pCGroup->mFirst = a.mFirst;
                        pCGroup->mLast  = a.mLast;
                        pCGroup->mpName = new char[a.mName.size() + 1];
                        if (pCGroup->mpName != nullptr)
                        {
                            std::strcpy(pCGroup->mpName, a.mName.c_str());
                        }
                        pCGroup ++;
                    }
                }
            }
        }
    }
    return groups;
}

//------------------------------------------------------------------------------
//! @brief      free groups you've got through md_header_groups()
//!
//! @param      groups  The groups
//------------------------------------------------------------------------------
void md_header_free_groups(MDGroups** groups)
{
    if (*groups != nullptr)
    {
        MDGroup* pCGroup = (*groups)->mpGroups;

        if (pCGroup != nullptr)
        {
            for (int i = 0; i < (*groups)->mCount; i++)
            {
                if (pCGroup->mpName != nullptr)
                {
                    delete [] pCGroup->mpName;
                }
                pCGroup ++;
            }
            delete [] (*groups)->mpGroups;
        }

        delete *groups;
        *groups = nullptr;
    }
}
