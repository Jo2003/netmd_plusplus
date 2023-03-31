/*
 * CNetMdAPatch.h
 *
 * This file is part of netmd++, a library for accessing NetMD devices.
 *
 * It makes use of knowledge / code collected by Marc Britten and
 * Alexander Sulfrian for the Linux Minidisc project.
 * Asivery made this possible!
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
#include "CNetMdDev.hpp"
#include "netmd_defines.h"
#include <cstdint>

namespace netmd {

class CNetMdApi;

//------------------------------------------------------------------------------
//! @brief      This class describes a net md patch.
//------------------------------------------------------------------------------
class CNetMdPatch
{
    // make friends
    friend CNetMdApi;

    static constexpr uint32_t PERIPHERAL_BASE = 0x03802000;
    static constexpr uint8_t  MAX_PATCH       = 8;

    enum SonyDevInfo : uint32_t
    {
        SDI_S1200   = (1ul <<  0),    //!< S1.200 version
        SDI_S1300   = (1ul <<  1),    //!< S1.300 version
        SDI_S1400   = (1ul <<  2),    //!< S1.400 version
        SDI_S1500   = (1ul <<  3),    //!< S1.500 version
        SDI_S1600   = (1ul <<  4),    //!< S1.600 version
        SDI_UNKNOWN = (1ul << 31),    //!< unsupported or unknown
    };

    enum PatchId : uint8_t
    {
        PID_UNUSED,
        PID_DEVTYPE,
        PID_PATCH_0_A,
        PID_PATCH_0_B,
        PID_PATCH_0,
        PID_PREP_PATCH,
        PID_PATCH_CMN_1,
        PID_PATCH_CMN_2,
        PID_TRACK_TYPE,
        PID_SAFETY,
    };

    enum MemAcc : uint8_t
    {
        NETMD_MEM_CLOSE      = 0x0,
        NETMD_MEM_READ       = 0x1,
        NETMD_MEM_WRITE      = 0x2,
        NETMD_MEM_READ_WRITE = 0x3,
    };

    struct PayLoad
    {
        uint32_t mDevs;
        NetMDByteVector mPtData;
    };

    using PatchAddr       = std::map<SonyDevInfo, uint32_t>;
    using PatchAdrrTab    = std::map<PatchId, PatchAddr>;
    using PatchPayloadTab = std::map<PatchId, PayLoad>;

    static const PatchAdrrTab    smPatchAddrTab;
    static const PatchPayloadTab smPatchPayloadTab;

    CNetMdPatch(CNetMdDev& netMd) : mNetMd(netMd)
    {}


    CNetMdDev& mNetMd;
};

} // ~namespace