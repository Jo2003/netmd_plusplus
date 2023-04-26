/*
 * md_toc.h
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
#pragma once
#include <cstdint>

namespace netmd
{
namespace toc
{
    constexpr uint16_t SIGNATURE    = 0x121;    //!< take as default signature (MZ-N510)
    constexpr uint8_t  F_PREEMPH    = (1 << 0); //!< premphasis
    constexpr uint8_t  F_STEREO     = (1 << 1); //!< 1 for SP stereo/LP2 mode, 0 for SP mono/LP4 mode audio tracks
    constexpr uint8_t  F_SP_MODE    = (1 << 2); //!< 1 for SP mode, 0 for LP mode audio tracks
    constexpr uint8_t  F_UNK1       = (1 << 3); //!< value always 0 for normal audio tracks
    constexpr uint8_t  F_AUDIO      = (1 << 4); //!< 0 for audio track (1 for non-audio?)
    constexpr uint8_t  F_SCMS_CPY   = (1 << 5); //!< SCMS: digital copy (original if clear)
    constexpr uint8_t  F_SCMS_UNR   = (1 << 6); //!< SCMS: unrestricted i.e. no copyright (copyrighted if clear)
    constexpr uint8_t  F_WRTENB     = (1 << 7); //!< writeable  (fragment is readonly when clear)

    //! SCMS shorthand
    constexpr uint8_t F_SCMSBITS = 0x60;                    //!< mask for extracting SCMS bits
    constexpr uint8_t F_SCMS00   = (F_SCMS_CPY|F_SCMS_UNR); //!< unlimited copies allowed
    constexpr uint8_t F_SCMS10   = (F_SCMS_CPY);            //!< no copies allowed (recording source was digital)
    constexpr uint8_t F_SCMS11   = 0;                       //!< allow 1 generation (recording source was analog)

    //! default track mode
    constexpr uint8_t DEF_TRACK_MODE = (F_STEREO | F_SP_MODE | F_AUDIO | F_SCMS00 | F_WRTENB);

    //! Cluster, Sector, Group; 3 bytes, packed as follows:
    //! c = cluster, s = sector, g = sound group.
    //! byte 0    byte 1    byte 2
    //! cccc.cccc cccc.ccss ssss.gggg
    //! The TOC uses a c.s.g. notation:
    //! A group is 11.5ms of mono audio (2 groups - 11.5ms of stereo audio).
    //! 1 sector is 11 groups,
    //! 1 cluster is 32 sectors
    struct discaddr
    {
        uint8_t csg[3];
    };

    //! Fragment definition --
    //! contains start and end addresses for one contiguous region of audio.
    //! 8 bytes.
    struct fragment
    {
        discaddr start;
        uint8_t mode;           //!< track mode
        discaddr end;
        uint8_t link;           //!< if non-zero, gives next fragment
    };

    //! UTOC sector #0 (Address info). 2336 bytes
    struct UTOC_0
    {
        uint8_t unknown_0[0xc];
        uint16_t sign;            //!< signature of last UTOC updater
        uint8_t nonempty;         //!< ?flag indicating disc is not blank
        uint8_t ntracks;          //!< number of tracks on disc
        uint8_t unknown_1[0xf];
        uint8_t free_track_slot;  //!< next empty track slot
        uint8_t trackmap[256];    //!< map track no. to slot in fraglist, entry 0 represents freelist
        fragment fraglist[256];
    };

    //! Title cell -- up to 7 characters of a title
    struct titlecell
    {
        char title[7];        //!< ASCII title info
        uint8_t link;         //!< if non-zero, title continues at this titlelist entry
    };

    //! UTOC sector #1 (Title info). 2336 bytes */
    struct UTOC_1
    {
        uint8_t unknown[0x1f];
        uint8_t free_title_slot;    //!< next free titlelist slot
        uint8_t titlemap[256];      //!< map track no. to slot in titlelist
        titlecell titlelist[256];
    };

    //! Timestamp
    struct timestamp
    {
        uint8_t y, mo, d, h, m, s;  //!< values are printed correctly in hex!
        uint16_t signature;         //!< "signature" of machine that wrote this track
    };

    //! UTOC sector #2 (Date and Time info). 2336 bytes
    struct  UTOC_2
    {
        uint8_t unknown[0x1f];
        uint8_t free_time_slot;     //!< next free timelist slot
        uint8_t timemap[256];       //!< map track no. to slot in timelist
        timestamp timelist[256];
    };

    //! unsupported UTOC sector. 7104 bytes
    struct UTOC_3
    {
        uint8_t unknown[64 * 111];
    };

    //! structure with the whole TOC data mapped.
    struct TOC
    {
        UTOC_0 mTracks;
        UTOC_1 mTitles;
        UTOC_2 mTimes;
        UTOC_3 mFWTitles;
    };
}} // ~namespaces