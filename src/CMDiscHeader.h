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
#ifndef CMD_DISC_HEADER_H
    #define CMD_DISC_HEADER_H
#ifdef __cplusplus
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "netmd_defines.h"

namespace netmd {

//------------------------------------------------------------------------------
//! @brief      This class describes a minidisc header
//------------------------------------------------------------------------------
class CMDiscHeader
{
public:
    //-----------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //-----------------------------------------------------------------------------
    CMDiscHeader();
    
    //-----------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param[in]  header  The RAW disc header as string
    //-----------------------------------------------------------------------------
    CMDiscHeader(const std::string& header);

    //-----------------------------------------------------------------------------
    //! @brief      Destroys the object.
    //-----------------------------------------------------------------------------
    ~CMDiscHeader();

    //-----------------------------------------------------------------------------
    //! @brief      create header from string
    //!
    //! @param[in]  header  The RAW disc header as string
    //!
    //! @return     { description_of_the_return_value }
    //-----------------------------------------------------------------------------
    int fromString(const std::string& header);

    //-----------------------------------------------------------------------------
    //! @brief      Returns a string representation of the object.
    //!
    //! @return     String representation of the object.
    //-----------------------------------------------------------------------------
    std::string toString();

    //-----------------------------------------------------------------------------
    //! @brief      Adds a group.
    //!
    //! @param[in]  name   The name
    //! @param[in]  first  The first track (optional)
    //! @param[in]  last   The last track (otional)
    //!
    //! @return     >= 0 -> group idx; -1 -> error
    //-----------------------------------------------------------------------------
    int addGroup(const std::string& name, int16_t first = -1, int16_t last = -1);

    //-----------------------------------------------------------------------------
    //! @brief      list/print all groups
    //-----------------------------------------------------------------------------
    void listGroups() const;

    //-----------------------------------------------------------------------------
    //! @brief      Adds a track to group.
    //!
    //! @param[in]  gid    The group id
    //! @param[in]  track  The track number
    //!
    //! @return     0 -> ok; -1 -> error
    //-----------------------------------------------------------------------------
    int addTrackToGroup(int gid, int16_t track);

    //-----------------------------------------------------------------------------
    //! @brief      remove a track from a group.
    //!
    //! @param[in]  gid    The group id
    //! @param[in]  track  The track number
    //!
    //! @return     0 -> ok; -1 -> error
    //-----------------------------------------------------------------------------
    int delTrackFromGroup(int gid, int16_t track);

    //-----------------------------------------------------------------------------
    //! @brief      remove a track
    //!
    //! @param[in]  track  The track number
    //!
    //! @return     0 -> ok; -1 -> error
    //-----------------------------------------------------------------------------
    int delTrack(int16_t track);

    //-----------------------------------------------------------------------------
    //! @brief      remove a group (included tracks become ungrouped)
    //!
    //! @param[in]  gid   The group id
    //!
    //! @return     0 -> ok; -1 -> error
    //-----------------------------------------------------------------------------
    int delGroup(int gid);

    //-----------------------------------------------------------------------------
    //! @brief      Sets the disc title.
    //!
    //! @param[in]  title  The title
    //!
    //! @return     0 -> ok; else -> error
    //-----------------------------------------------------------------------------
    int setDiscTitle(const std::string& title);

    //-----------------------------------------------------------------------------
    //! @brief      gets the disc title.
    //!
    //! @return     disc title
    //-----------------------------------------------------------------------------
    std::string discTitle();

    //-----------------------------------------------------------------------------
    //! @brief      rename one group
    //!
    //! @param[in]  gid    The group id
    //! @param[in]  title  The new title
    //!
    //! @return     0 -> ok; else -> error
    //-----------------------------------------------------------------------------
    int renameGroup(int gid, const std::string& title);

    //-----------------------------------------------------------------------------
    //! @brief      get the group title for track
    //!
    //! @param[in]  track  The track
    //! @param      pGid   The gid
    //!
    //! @return     group title or empty string
    //-----------------------------------------------------------------------------
    std::string trackGroup(int16_t track, int16_t* pGid);

    //-----------------------------------------------------------------------------
    //! @brief      return the C string header
    //!
    //! @return     C string with MD header data
    //-----------------------------------------------------------------------------
    const char* stringHeader();

    //-----------------------------------------------------------------------------
    //! @brief      returns last buildt C string
    //!
    //! @return     C string
    //-----------------------------------------------------------------------------
    const char* lastString();

    //-----------------------------------------------------------------------------
    //! @brief      compare function used to sort groups in header
    //!
    //! @param[in]  a     group a
    //! @param[in]  b     group b
    //!
    //! @return     true if a less b
    //-----------------------------------------------------------------------------
    static bool groupCompare(const Group& a, const Group& b);

    //-----------------------------------------------------------------------------
    //! @brief      ungroup a track
    //!
    //! @param[in]  track  The track
    //!
    //! @return     0 -> ok; -1 -> error
    //-----------------------------------------------------------------------------
    int unGroup(int16_t track);

    //-----------------------------------------------------------------------------
    //! @brief      get all groups
    //!
    //! @return     const reference to groups
    //-----------------------------------------------------------------------------
    Groups groups() const;

protected:
    //-----------------------------------------------------------------------------
    //! @brief      check groups / tracks for sanity
    //!
    //! @param[in]  grps  The grps
    //!
    //! @return     0 -> all well; -1 -> error
    //-----------------------------------------------------------------------------
    int sanityCheck(const Groups& grps) const;

private:
    Groups       mGroups;
    int          mGroupId;
    char*        mpCStringHeader;
    char*        mpLastString;
};

extern "C" {
#endif // __cplusplus 
#include <stdint.h>

/* copy start */

//! define a MD Header handle
typedef void* HndMdHdr;

//! c structure to hold a groups
typedef struct {
    int      mGid;      //!< group id
    int16_t  mFirst;    //!< first track
    int16_t  mLast;     //!< last track
    char*    mpName;    //!< group name
} MDGroup;

//! a groups container
typedef struct {
    int      mCount;
    MDGroup* mpGroups;
} MDGroups;

//------------------------------------------------------------------------------
//! @brief      Creates a md header.
//!
//! @param[in]  content  The content
//!
//! @return     The handle md header.
//------------------------------------------------------------------------------
HndMdHdr create_md_header(const char* content);

//------------------------------------------------------------------------------
//! @brief         free the MD header
//!
//! @param[in/out] hdl   handle to MD header
//------------------------------------------------------------------------------
void free_md_header(HndMdHdr* hdl);

//------------------------------------------------------------------------------
//! @brief      create C string from MD header
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     C string or NULL
//------------------------------------------------------------------------------
const char* md_header_to_string(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      add a group to the MD header
//!
//! @param[in]  hdl    The MD header handlehdl
//! @param[in]  name   The name
//! @param[in]  first  The first
//! @param[in]  last   The last
//!
//! @return     > -1 -> group id; else -> error
//------------------------------------------------------------------------------
int md_header_add_group(HndMdHdr hdl, const char* name, int16_t first, int16_t last);

//------------------------------------------------------------------------------
//! @brief      list groups in MD header
//!
//! @param[in]  hdl   The MD header handle
//------------------------------------------------------------------------------
void md_header_list_groups(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      Adds a track to group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_add_track_to_group(HndMdHdr hdl, int gid, int16_t track);

//-----------------------------------------------------------------------------
//! @brief      remove a track from a group.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track_from_group(HndMdHdr hdl, int gid, int16_t track);

//-----------------------------------------------------------------------------
//! @brief      remove a track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track number
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_track(HndMdHdr hdl, int16_t track);

//-----------------------------------------------------------------------------
//! @brief      remove a group (included tracks become ungrouped)
//!
//! @param[in]  hdl   The MD header handle
//! @param[in]  gid   The group id
//!
//! @return     0 -> ok; -1 -> error
//-----------------------------------------------------------------------------
int md_header_del_group(HndMdHdr hdl, int gid);

//-----------------------------------------------------------------------------
//! @brief      Sets the disc title.
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  title  The title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_set_disc_title(HndMdHdr hdl, const char* title);

//-----------------------------------------------------------------------------
//! @brief      rename one group
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  gid    The group id
//! @param[in]  title  The new title
//!
//! @return     0 -> ok; else -> error
//-----------------------------------------------------------------------------
int md_header_rename_group(HndMdHdr hdl, int gid, const char* title);

//------------------------------------------------------------------------------
//! @brief      get disc title
//!
//! @param[in]  hdl   The MD header handle
//!
//! @return     C string
//------------------------------------------------------------------------------
const char* md_header_disc_title(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      get group name for track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track number
//! @param[out] pGid   The buffer for gid
//!
//! @return     C string or nullptr
//------------------------------------------------------------------------------
const char* md_header_track_group(HndMdHdr hdl, int16_t track, int16_t* pGid);

//------------------------------------------------------------------------------
//! @brief      unhroup track
//!
//! @param[in]  hdl    The MD header handle
//! @param[in]  track  The track
//!
//! @return     0 -> ok; -1 -> error
//------------------------------------------------------------------------------
int md_header_ungroup_track(HndMdHdr hdl, int16_t track);

//------------------------------------------------------------------------------
//! @brief      export all minidisc groups
//!
//! @param[in]  hdl The MD header handle
//!
//! @return     array of groups
//------------------------------------------------------------------------------
MDGroups* md_header_groups(HndMdHdr hdl);

//------------------------------------------------------------------------------
//! @brief      free groups you've got through md_header_groups()
//!
//! @param      groups  The groups
//------------------------------------------------------------------------------
void md_header_free_groups(MDGroups** groups);

} //~namespace

/* copy end */

#ifdef __cplusplus
}
#endif //  __cplusplus
#endif // CMD_DISC_HEADER_H