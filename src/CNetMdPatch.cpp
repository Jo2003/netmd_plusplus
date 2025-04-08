/*
 * CNetMdAPatch.cpp
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
#include "CNetMdPatch.h"
#include "CNetMdDev.hpp"
#include "log.h"
#include "netmd_defines.h"
#include "netmd_utils.h"
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <utility>

namespace netmd {

#define SDI(x_) SonyDevInfo::SDI_##x_

/// patch addresses
const CNetMdPatch::PatchAdrrTab CNetMdPatch::smPatchAddrTab =
{
    {
        PID_DEVTYPE,
        {
            {SDI(S1600), 0x02003fcf},
            {SDI(S1500), 0x02003fc7},
            {SDI(S1400), 0x03000220},
            {SDI(S1300), 0x02003e97},
            {SDI(S1000), 0x0200401b}
        }
    },
    {
        PID_PATCH_0_A,
        {
            {SDI(S1600), 0x0007f408},
            {SDI(S1500), 0x0007e988},
            {SDI(S1400), 0x0007e2c8},
            {SDI(S1300), 0x0007aa00},
            {SDI(S1000), 0x0007f59c}
        }
    },
    {
        PID_PATCH_0_B,
        {
            {SDI(S1600), 0x0007efec},
            {SDI(S1500), 0x0007e56c},
            {SDI(S1400), 0x0007deac},
            {SDI(S1300), 0x0007a5e4},
            {SDI(S1200), 0x00078dcc},
            {SDI(S1100), 0x000783c0},
            {SDI(S1000), 0x0007f180}
        }
    },
    {
        PID_PREP_PATCH,
        {
            {SDI(S1600), 0x00077c04},
            {SDI(S1500), 0x0007720c},
            {SDI(S1400), 0x00076b38},
            {SDI(S1300), 0x00073488},
            {SDI(S1200), 0x00071e5c},
            {SDI(S1100), 0x000714d4},
            {SDI(S1000), 0x00077d6c}
        }
    },
    {
        PID_PATCH_CMN_1,
        {
            {SDI(S1600), 0x0007f4e8},
            {SDI(S1500), 0x0007ea68},
            {SDI(S1400), 0x0007e3a8},
            {SDI(S1300), 0x0007aae0},
            {SDI(S1200), 0x00078eac},
            {SDI(S1100), 0x000784a0},
            {SDI(S1000), 0x0007f67c}
        }
    },
    {
        PID_PATCH_CMN_2,
        {
            {SDI(S1600), 0x0007f4ec},
            {SDI(S1500), 0x0007ea6c},
            {SDI(S1400), 0x0007e3ac},
            {SDI(S1300), 0x0007aae4},
            {SDI(S1200), 0x00078eb0},
            {SDI(S1100), 0x000784a0},
            {SDI(S1000), 0x0007f680}
        }
    },
    {
        PID_TRACK_TYPE,
        {
            {SDI(S1600), 0x000852b0},
            {SDI(S1500), 0x00084820},
            {SDI(S1400), 0x00084160},
            {SDI(S1300), 0x00080798},
            {SDI(S1200), 0x0007ea9c},
            {SDI(S1100), 0x0007e084},
            {SDI(S1000), 0x00085444}
        }
    },
    {   /// anti brick patch
        PID_SAFETY,
        {
            {SDI(S1600), 0x000000c4},
            {SDI(S1500), 0x000000c4},
            {SDI(S1400), 0x000000c4},
            {SDI(S1000), 0x000000c4}
        }
    },
    {
        PID_USB_EXE,
        {
            {SDI(R1000), 0x00056228},
            {SDI(R1100), 0x00056aac},
            {SDI(R1200), 0x000577f8},
            {SDI(R1300), 0x00057b48},
            {SDI(R1400), 0x00057be8},
            {SDI(S1000), 0x0000e784},
            {SDI(S1100), 0x0000d784},
            {SDI(S1200), 0x0000d834},
            {SDI(S1300), 0x0000daa8},
            {SDI(S1400), 0x0000e4c4},
            {SDI(S1500), 0x0000e538},
            {SDI(S1600), 0x0000e69c}
        }
    },
    {
        PID_PCM_TO_MONO,
        {
            {SDI(S1600), 0x00013d78},
            {SDI(S1500), 0x00013b8c},
            {SDI(S1400), 0x00013a84},
            {SDI(S1300), 0x00012c34},
            {SDI(S1200), 0x000129c0},
            {SDI(S1100), 0x00012910},
            {SDI(S1000), 0x00013e6c},
            {SDI(R1000), 0x000576e8},
            {SDI(R1100), 0x00057f8c},
            {SDI(R1200), 0x00058cf8},
            {SDI(R1300), 0x0005904c},
            {SDI(R1400), 0x000590ec}
        }
    },
    {
        PID_PCM_SPEEDUP_1,
        {
            {SDI(S1000), 0x0007852c},
            {SDI(S1100), 0x00071c04},
            {SDI(S1200), 0x0007258c},
            {SDI(S1300), 0x00073c40},
            {SDI(S1400), 0x00077300},
            {SDI(S1500), 0x000779d4},
            {SDI(S1600), 0x000783cc}
        }
    },
    {
        PID_PCM_SPEEDUP_2,
        {
            {SDI(S1000), 0x0001ad94},
            {SDI(S1100), 0x00019464},
            {SDI(S1200), 0x00019628},
            {SDI(S1300), 0x000198f8},
            {SDI(S1400), 0x0001a820},
            {SDI(S1500), 0x0001aa94},
            {SDI(S1600), 0x0001ac9c}
        }
    }
};

/// patch payload
const CNetMdPatch::PatchPayloadTab CNetMdPatch::smPatchPayloadTab =
{
    {
        PID_PATCH_0,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x00,0x00,0xa0,0xe1}}
        }
    },
    {
        PID_PREP_PATCH,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x0D,0x31,0x01,0x60}}
        }
    },
    {
        PID_PATCH_CMN_1,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x14,0x80,0x80,0x03}}
        }
    },
    {
        PID_PATCH_CMN_2,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x14,0x90,0x80,0x03}}
        }
    },
    {
        PID_TRACK_TYPE,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x06,0x02,0x00,0x04}}
        }
    },
    {   //! anti brick patch
        PID_SAFETY,
        {
            {SDI(S1000) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0xdc,0xff,0xff,0xea}}
        }
    },
    {
        PID_USB_EXE,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x13,0x48,0x00,0x47}},
            {SDI(R1000) | SDI(R1100) | SDI(R1200) | SDI(R1300) | SDI(R1400)                          , {0x1a,0x48,0x00,0x47}},
        }
    },
    {
        PID_PCM_TO_MONO,
        {
            {SDI(S1000) | SDI(S1600) | SDI(S1500)                          , {0x00,0x46,0x2a,0xf0}},
            {SDI(S1400)                                                    , {0x00,0x46,0x29,0xf0}},
            {SDI(S1300)                                                    , {0x00,0x46,0x28,0xf0}},
            {SDI(S1200)                                                    , {0x00,0x46,0x27,0xf0}},
            {SDI(S1100)                                                    , {0x28,0x1c,0x00,0x46}},
            {SDI(R1000) | SDI(R1100) | SDI(R1200) | SDI(R1300) | SDI(R1400), {0x03,0x29,0x0b,0xe0}},
        }
    },
    {
        PID_PCM_SPEEDUP_1,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x41,0x31,0x01,0x60}}
        }
    },
    {
        PID_PCM_SPEEDUP_2,
        {
            {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), {0x00,0x0f,0x0f,0xe0}}
        }
    }
};

/// exploit command lookup
const CNetMdPatch::ExploitCmds CNetMdPatch::smExploidCmds =
{
    {SDI(S1000) | SDI(S1100) | SDI(S1200) | SDI(S1300) | SDI(S1400) | SDI(S1500) | SDI(S1600), 0xd2},
    {SDI(R1000) | SDI(R1100) | SDI(R1200) | SDI(R1300) | SDI(R1400)                          , 0xd3}
};

/// exploit payload
const CNetMdPatch::ExploitPayloadTab CNetMdPatch::smExplPayloadTab =
{
    {
        EID_LOWER_HEAD,
        {
            {SDI(R1000), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xb1, 0xe5, 0x03, 0x00}},
            {SDI(R1100), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xbd, 0xec, 0x03, 0x00}},
            {SDI(R1200), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xe9, 0xf4, 0x03, 0x00}},
            {SDI(R1300), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x3d, 0xf6, 0x03, 0x00}},
            {SDI(R1400), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xdd, 0xf6, 0x03, 0x00}},
            {SDI(S1000), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x51, 0x2f, 0x05, 0x00}},
            {SDI(S1100), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xc5, 0xd2, 0x04, 0x00}},
            {SDI(S1200), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xf1, 0xd9, 0x04, 0x00}},
            {SDI(S1300), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xa1, 0xe9, 0x04, 0x00}},
            {SDI(S1400), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x9d, 0x1d, 0x05, 0x00}},
            {SDI(S1500), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x6d, 0x24, 0x05, 0x00}},
            {SDI(S1600), {0x02, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x01, 0x2e, 0x05, 0x00}}
        }
    },
    {
        EID_RAISE_HEAD,
        {
            {SDI(R1000), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xb1, 0xe5, 0x03, 0x00}},
            {SDI(R1100), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xbd, 0xec, 0x03, 0x00}},
            {SDI(R1200), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xe9, 0xf4, 0x03, 0x00}},
            {SDI(R1300), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x3d, 0xf6, 0x03, 0x00}},
            {SDI(R1400), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xdd, 0xf6, 0x03, 0x00}},
            {SDI(S1000), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x51, 0x2f, 0x05, 0x00}},
            {SDI(S1100), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xc5, 0xd2, 0x04, 0x00}},
            {SDI(S1200), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xf1, 0xd9, 0x04, 0x00}},
            {SDI(S1300), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xa1, 0xe9, 0x04, 0x00}},
            {SDI(S1400), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x9d, 0x1d, 0x05, 0x00}},
            {SDI(S1500), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x6d, 0x24, 0x05, 0x00}},
            {SDI(S1600), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x01, 0x2e, 0x05, 0x00}}
        }
    },
    {
        EID_TRIGGER,
        {
            {SDI(R1000), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x8d, 0xf4, 0x01, 0x00}},
            {SDI(R1100), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x99, 0xf6, 0x01, 0x00}},
            {SDI(R1200), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xc1, 0xf9, 0x01, 0x00}},
            {SDI(R1300), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x3d, 0xfa, 0x01, 0x00}},
            {SDI(R1400), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xbd, 0xfa, 0x01, 0x00}},
            {SDI(S1000), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x63, 0x6c, 0x01, 0x00}},
            {SDI(S1100), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x43, 0x4f, 0x01, 0x00}},
            {SDI(S1200), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0xe3, 0x50, 0x01, 0x00}},
            {SDI(S1300), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x5f, 0x53, 0x01, 0x00}},
            {SDI(S1400), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x77, 0x62, 0x01, 0x00}},
            {SDI(S1500), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x83, 0x69, 0x01, 0x00}},
            {SDI(S1600), {0x01, 0x00, 0xa0, 0xe3, 0x00, 0x10, 0x9f, 0xe5, 0x11, 0xff, 0x2f, 0xe1, 0x6f, 0x6b, 0x01, 0x00}}
        }
    },
    {
        EID_DEV_RESET,
        {
            {SDI(R1000), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(R1100), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(R1200), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(R1300), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(R1400), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1000), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1100), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1200), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1300), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1400), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1500), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}},
            {SDI(S1600), {0x00, 0x00, 0xa0, 0xe3, 0x10, 0xff, 0x2f, 0xe1}}
        }
    }
};

//--------------------------------------------------------------------------
//! @brief      print helper for PatchId
//!
//! @param[in, out] os ref. to ostream
//! @param[in]      pid patch id
//!
//! @returns       ref. to os
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const CNetMdPatch::PatchId& pid)
{
    switch(pid)
    {
        case CNetMdPatch::PatchId::PID_UNUSED       : os << "PID_UNUSED"       ; break;
        case CNetMdPatch::PatchId::PID_DEVTYPE      : os << "PID_DEVTYPE"      ; break;
        case CNetMdPatch::PatchId::PID_PATCH_0_A    : os << "PID_PATCH_0_A"    ; break;
        case CNetMdPatch::PatchId::PID_PATCH_0_B    : os << "PID_PATCH_0_B"    ; break;
        case CNetMdPatch::PatchId::PID_PATCH_0      : os << "PID_PATCH_0"      ; break;
        case CNetMdPatch::PatchId::PID_PREP_PATCH   : os << "PID_PREP_PATCH"   ; break;
        case CNetMdPatch::PatchId::PID_PATCH_CMN_1  : os << "PID_PATCH_CMN_1"  ; break;
        case CNetMdPatch::PatchId::PID_PATCH_CMN_2  : os << "PID_PATCH_CMN_2"  ; break;
        case CNetMdPatch::PatchId::PID_TRACK_TYPE   : os << "PID_TRACK_TYPE"   ; break;
        case CNetMdPatch::PatchId::PID_SAFETY       : os << "PID_SAFETY"       ; break;
        case CNetMdPatch::PatchId::PID_USB_EXE      : os << "PID_USB_EXE"      ; break;
        case CNetMdPatch::PatchId::PID_PCM_TO_MONO  : os << "PID_PCM_TO_MONO"  ; break;
        case CNetMdPatch::PatchId::PID_PCM_SPEEDUP_1: os << "PID_PCM_SPEEDUP_1"; break;
        case CNetMdPatch::PatchId::PID_PCM_SPEEDUP_2: os << "PID_PCM_SPEEDUP_2"; break;
        default                                     : os << "n/a"              ; break;
    }
    return os;
}

//--------------------------------------------------------------------------
//! @brief      print helper for NetMDByteVector
//!
//! @param[in, out] os ref. to ostream
//! @param[in]      pdata patch data
//!
//! @returns       ref. to os
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const NetMDByteVector& pdata)
{
    for (const auto& a : pdata)
    {
        os << " 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(a) << std::dec;
    }
    return os;
}

//--------------------------------------------------------------------------
//! @brief      print helper for PatchComplect
//!
//! @param[in, out] os ref. to ostream
//! @param[in]      patch patch complect
//!
//! @returns       ref. to os
//--------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const CNetMdPatch::PatchComplect& patch)
{
    os << "Device: " << patch.mDev << ", patch: " << patch.mPid 
       << ", address: 0x" << std::hex << std::setw(8) << std::setfill('0') << patch.mAddr << std::dec
       << ", content: " << patch.mPatchData << ", free slot: " << patch.mNextFreePatch;
    return os;
}

//--------------------------------------------------------------------------
//! @brief      Constructs a new instance.
//!
//! @param      netMd  The net md device reference
//--------------------------------------------------------------------------
CNetMdPatch::CNetMdPatch(CNetMdDev& netMd) 
    : mNetMd(netMd), mUsedPatches{PatchId::PID_UNUSED,}
{
}

//--------------------------------------------------------------------------
//! @brief      device was removed
//--------------------------------------------------------------------------
void CNetMdPatch::deviceRemoved()
{
    for (uint8_t i = 0; i < MAX_PATCH; i++)
    {
        mUsedPatches[i] = PatchId::PID_UNUSED;
    }
}

//--------------------------------------------------------------------------
//! @brief      get number of max patches
//!
//! @return     -1 -> error; else max number of patches
//--------------------------------------------------------------------------
int CNetMdPatch::maxPatches() const
{
    if ((mNetMd.sonyDevCode() >= SDI(S_START)) && (mNetMd.sonyDevCode() <= SDI(S_END)))
    {
        // 8
        return MAX_PATCH / 2;
    }
    else if ((mNetMd.sonyDevCode() >= SDI(R_START)) && (mNetMd.sonyDevCode() <= SDI(R_END)))
    {
        // 4
        return MAX_PATCH / 4;
    }
    return -1;
}

//--------------------------------------------------------------------------
//! @brief      get next pree patch index
//!
//! @param[in]  pid   The pid
//!
//! @return     -1 -> no more free | > -1 -> free patch index
//--------------------------------------------------------------------------
int CNetMdPatch::nextFreePatch(PatchId pid)
{
    for (int i = 0; i < maxPatches(); i++)
    {
        if ((mUsedPatches[i] == pid) || (mUsedPatches[i] == PID_UNUSED))
        {
            mUsedPatches[i] = pid;
            return i;
        }
    }
    return -1;
}

//--------------------------------------------------------------------------
//! @brief      mark patch as unused
//!
//! @param[in]  pid   The pid
//!
//! @return     -1 -> not found | > -1 -> last used patch index
//--------------------------------------------------------------------------
int CNetMdPatch::patchUnused(PatchId pid)
{
    int used = -1;
    for (int i = 0; i < maxPatches(); i++)
    {
        if (mUsedPatches[i] == pid)
        {
            used = i;
            mUsedPatches[i] = PID_UNUSED;
        }
    }
    return used;
}

//------------------------------------------------------------------------------
//! @brief      get patch address by name and device info
//!
//! @param[in]  devinfo    device info
//! @param[in]  pid        patch id
//!
//! @return     0 -> error | > 0 -> address
//------------------------------------------------------------------------------
uint32_t CNetMdPatch::patchAddress(SonyDevInfo devinfo, PatchId pid)
{
    const auto p = smPatchAddrTab.find(pid);

    if (p != smPatchAddrTab.cend())
    {
        const auto& a = p->second.find(devinfo);
        if (a != p->second.cend())
        {
            return a->second;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
//! @brief      get patch payload by name and device info
//!
//! @param[in]  devinfo    device info
//! @param[in]  pid        patch id
//!
//! @return     empty vector -> error; else -> patch content
//------------------------------------------------------------------------------
NetMDByteVector CNetMdPatch::patchPayload(SonyDevInfo devinfo, PatchId pid)
{
    const auto pl = smPatchPayloadTab.find(pid);
    if (pl != smPatchPayloadTab.cend())
    {
        for(const auto& pld : pl->second)
        {
            if (static_cast<uint32_t>(devinfo) & pld.mDevs)
            {
                return pld.mPtData;
            }
        }
    }

    return NetMDByteVector{};
}

//--------------------------------------------------------------------------
//! @brief      collect patch data
//!
//! @param[in]  pid   The patch id
//! @param[in]  dev   The device information
//! @param[out] patch Buffer for patch complect
//! @param[in]  plpid The optional payload pid (if different)
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::fillPatchComplect(PatchId pid, SonyDevInfo dev, PatchComplect& patch, PatchId plpid)
{
    if (plpid == PatchId::PID_UNUSED)
    {
        plpid = pid;
    }

    patch.mDev           = dev;
    patch.mPid           = pid;
    patch.mAddr          = patchAddress(dev, pid);
    patch.mPatchData     = patchPayload(dev, plpid);
    patch.mNextFreePatch = nextFreePatch(plpid);

    mLOG(INFO) << patch;

    if ((patch.mAddr > 0) && !patch.mPatchData.empty() && (patch.mNextFreePatch != -1))
    {
        return NETMDERR_NO_ERROR;
    }

    patchUnused(plpid);
    return NETMDERR_NOT_SUPPORTED;
}

//--------------------------------------------------------------------------
//! @brief      get exploit command
//!
//! @param[in]  devinfo  The devinfo
//!
//! @return     the exploit command; 0 on error
//--------------------------------------------------------------------------
uint8_t CNetMdPatch::exploitCmd(SonyDevInfo devinfo)
{
    for(const auto& cmd : smExploidCmds)
    {
        if(cmd.first & static_cast<uint32_t>(devinfo))
        {
            return cmd.second;
        }
    }
    return 0;
}

//--------------------------------------------------------------------------
//! @brief      get exploit data
//!
//! @param[in]  devinfo  The device info
//! @param[in]  eid      The exploit id
//!
//! @return     on error: empty byte vector
//--------------------------------------------------------------------------
NetMDByteVector CNetMdPatch::exploitData(SonyDevInfo devinfo, ExploitId eid)
{
    const auto cit = smExplPayloadTab.find(eid);

    if (cit != smExplPayloadTab.cend())
    {
        const auto data = cit->second.find(devinfo);
        if (data != cit->second.cend())
        {
            return data->second;
        }
    }

    return NetMDByteVector{};
}

//--------------------------------------------------------------------------
//! @brief      check if patch is active
//!
//! @param[in]  pid      The patch id
//! @param[in]  devinfo  The device info
//!
//! @return     1 -> patch is active; 0 -> not active
//--------------------------------------------------------------------------
int CNetMdPatch::checkPatch(PatchId pid, SonyDevInfo devinfo)
{
    mLOG(DEBUG);
    uint32_t        addr      = patchAddress(devinfo, pid);
    NetMDByteVector patch_cnt = patchPayload(devinfo, pid);

    try
    {
        NetMDByteVector cur_patch_data;
        uint32_t        cur_patch_addr = 0;

        if ((addr == 0) || patch_cnt.empty())
        {
            mNetMdThrow(NETMDERR_NO_ERROR, "Patch data not found!");
        }

        for (int i = 0; i < maxPatches(); i++)
        {
            if (readPatchData(i, cur_patch_addr, cur_patch_data) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't read patch data for patch #" << i << ".");
            }

            if ((cur_patch_addr == addr) && (cur_patch_data == patch_cnt))
            {
                mLOG(DEBUG) << "patch " << static_cast<int>(pid) << " found at patch slot #" << i << ".";
                mUsedPatches[i] = pid;
                return 1;
            }
        }
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return 0;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return 0;
    }

    return 0;
}

//--------------------------------------------------------------------------
//! @brief      Reads an utoc sector.
//!
//! @param[in]  s     sector name
//!
//! @return     TOC sector data. (error if empty)
//--------------------------------------------------------------------------
NetMDByteVector CNetMdPatch::readUTOCSector(UTOCSector s)
{
    constexpr uint16_t RD_SIZE = 0x10;
    NetMDByteVector ret, part;

    for (int i = 0; i < 147; i++)
    {
        // 147 * 16 = 2352 -> sector length
        part = mNetMd.readMetadataPeripheral(s, i * RD_SIZE, RD_SIZE);
        if (!part.empty())
        {
            ret += part;
            // mLOG(INFO) << "Read part of " << part.size() << " Bytes, sector is now " << ret.size() << " Bytes";
        }
        else
        {
            mLOG(CRITICAL) << "Can't read TOC data for sector " << static_cast<int>(s);
            return NetMDByteVector{};
        }
    }

    mLOG(INFO) << "Sector " << static_cast<int>(s) << LOG::hexFormat(INFO, ret);
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Writes an utoc sector.
//!
//! @param[in]  s     sector names
//! @param[in]  data  The data to be written
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::writeUTOCSector(UTOCSector s, const NetMDByteVector& data)
{
    int err = 0;
    constexpr uint16_t WR_SIZE = 0x10;
    if (data.size() !=  2352)
    {
        mLOG(CRITICAL) << "The TOC data provided is not a valid TOC Sector!";
        return NETMDERR_PARAM;
    }

    for (int i = 0; i < 147; i++)
    {
        if ((err = mNetMd.writeMetadataPeripheral(s, i * WR_SIZE, subVec(data, i * WR_SIZE, WR_SIZE))) != NETMDERR_NO_ERROR)
        {
            mLOG(CRITICAL) << "Can't write TOC data for sector " << static_cast<int>(s);
            return err;
        }
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      prepare TOC manipulation
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::prepareTOCManip()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return NETMDERR_NOT_SUPPORTED;
    }

    try
    {
        mLOG(DEBUG) << "Try to get device code ...";
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode == SDI(UNKNOWN)) || (devcode == SDI(NO_SUPPORT)))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }

        mLOG(DEBUG) << "Apply safety patch ...";
        if (safetyPatch() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable safety patch!");
        }

        // check, if patch is already active ...
        if (!checkUSBExec(devcode))
        {
            // patch ...
            PatchComplect pc;
            mLOG(DEBUG) << "=== Apply USB code execution patch ===";
            if (fillPatchComplect(PID_USB_EXE, devcode, pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_OTHER, "Can't find patch data USB code execution patch!");
            }
            if (patch(pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't apply USB code execution patch!");
            }
        }
        else
        {
            mLOG(INFO) << "USB execution patch active";
        }
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      USB execution (run exploit)
//!
//! @param[in]  devInfo   The device information
//! @param[in]  execData  The data to USBExecute
//! @param[out] pResp     The optional response pointer
//! @param[in]  sendOnly  The send only (no exchange) - optional
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::USBExecute(SonyDevInfo devInfo, const NetMDByteVector& execData,
                            NetMDResp* pResp, bool sendOnly)
{
    NetMDResp query, resp;
    NetMDParams params;
    int ret;

    if ((ret = formatQuery("00 18%b ff %*", {{exploitCmd(devInfo)}, {execData}}, query)) > 0)
    {
        if (sendOnly)
        {
            // we expect an error on some commands, since read doesn't work and re-init is needed!
            return mNetMd.sendCmd(query.get(), ret, false);
        }

        if ((ret = mNetMd.exchange(query.get(), ret, &resp, false, CNetMdDev::NETMD_STATUS_NOT_IMPLEMENTED, ret + 1)) > 0)
        {
            // capture result
            if (scanQuery(resp.get(), ret, "%? 18%? ff %*", params) == NETMDERR_NO_ERROR)
            {
                if ((params.size() == 1) && (params.at(0).index() == BYTE_VECTOR))
                {
                    if (pResp != nullptr)
                    {
                        NetMDByteVector data = simple_get<NetMDByteVector>(params.at(0));
                        *pResp = NetMDResp(new uint8_t[data.size()]);

                        for(size_t i = 0; i < data.size(); i++)
                        {
                            (*pResp)[i] = data.at(i);
                        }
                        ret = data.size();
                    }
                    else
                    {
                        ret = NETMDERR_NO_ERROR;
                    }

                    return ret;
                }
            }
        }
    }

    return NETMDERR_OTHER;
}

//--------------------------------------------------------------------------
//! @brief      check if USB execution works
//!
//! @param[in]  devcode   The device information
//!
//! @return     true if it works, false if not
//--------------------------------------------------------------------------
bool CNetMdPatch::checkUSBExec(const SonyDevInfo& devcode)
{
    mLOG(DEBUG);
    if (devcode != SDI(UNKNOWN))
    {
        NetMDResp resp;
        NetMDParams params;
        int ret;
        if ((ret = USBExecute(devcode, 
                             {0x01, 0x00, 0xa0, 0xe3, 0x00, 
                              0x00, 0xcf, 0xe5, 0x1e, 0xff, 
                              0x2f, 0xe1, 0x00}, 
                              &resp)) >= 0)
        {
            if (scanQuery(resp.get(), ret, "0100a0e30000cfe51eff2fe1 %b", params) == NETMDERR_NO_ERROR)
            {
                if ((params.size() == 1) && (params.at(0).index() == UINT8_T))
                {
                    int err = 0, result;
                    result = simple_get<uint8_t>(params.at(0), &err);
                    return ((err == 0) && !!result);
                }
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------
//! @brief      finalize TOC though exploit
//!
//! @param[in]  reset  do device reset if true
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::finalizeTOC(bool reset)
{
    try
    {
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode == SDI(UNKNOWN)) || (devcode == SDI(NO_SUPPORT)))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }

        mLOG(CAPTURE) << "Finalizing TOC: 00%";

        if (USBExecute(devcode, exploitData(devcode, EID_LOWER_HEAD)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_OTHER, "Exploit 'lower head' failed!");
        }
        mLOG(DEBUG) << "Lower head success!";

        for(int i = 1; i <= 5; i++)
        {
            uwait(250'000);
            mLOG(CAPTURE) << "Finalizing TOC: " << std::setw(2) << std::setfill('0') << i << "%";
        }
        
        if (USBExecute(devcode, exploitData(devcode, EID_TRIGGER)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_OTHER, "Exploit 'trigger' failed!");
        }
        mLOG(DEBUG) << "Trigger success!";

        for(int i = 6; i <= 89; i++)
        {
            uwait(250'000);
            mLOG(CAPTURE) << "Finalizing TOC: " << std::setw(2) << std::setfill('0') << i << "%";
        }

        if (USBExecute(devcode, exploitData(devcode, EID_RAISE_HEAD)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_OTHER, "Exploit 'raise head' failed!");
        }
        mLOG(DEBUG) << "Raise head success!";

        if (reset)
        {
            if (USBExecute(devcode, exploitData(devcode, EID_DEV_RESET), nullptr, true) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_OTHER, "Exploit 'device reset' failed!");
            }
            mLOG(DEBUG) << "Device reset success!";
        }

        mLOG(CAPTURE) << "Finalizing TOC: 90%";
    }
    catch(const ThrownData& e)
    {
        LOG(CRITICAL) << e.mErrDescr;
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Reads a patch data.
//!
//! @param[in]  patchNo  The patch no
//! @param      addr     The patch address
//! @param      patch    The patch
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::readPatchData(int patchNo, uint32_t& addr, NetMDByteVector& patch)
{
    mLOG(DEBUG);
    int            ret   = NETMDERR_NO_ERROR;
    const uint32_t base  = PERIPHERAL_BASE + patchNo * 0x10;
    NetMDByteVector reply;

    if ((mNetMd.cleanRead(base + 4, 4, reply) == NETMDERR_NO_ERROR) && (reply.size() == 4))
    {
        addr = fromLittleEndianByteVector<uint32_t>(reply);

        if ((mNetMd.cleanRead(base + 8, 4, patch) != NETMDERR_NO_ERROR) || (patch.size() != 4))
        {
            ret = NETMDERR_USB;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      do one patch
//!
//! @param[in]  pc    The patch complect
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::patch(const PatchComplect& pc)
{
    return patch(pc.mAddr, pc.mPatchData, pc.mNextFreePatch);
}

//--------------------------------------------------------------------------
//! @brief      do one patch
//!
//! @param[in]  addr     The address
//! @param[in]  data     The patch data
//! @param[in]  patchNo  The patch no
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::patch(uint32_t addr, const NetMDByteVector& data, int patchNo)
{
    mLOG(DEBUG);
    // Original method written by Sir68k.

    try
    {
        if (data.size() != 4)
        {
            mNetMdThrow(NETMDERR_PARAM, "Patch content needs to be 4 bytes! Have: " << data.size());
        }

        int iMaxPatches = maxPatches();

        if ((iMaxPatches < 0) || (patchNo < 0))
        {
            mNetMdThrow(NETMDERR_PARAM, "Error with patch number(s)!");
        }

        const uint32_t base    = PERIPHERAL_BASE + patchNo   * 0x10;
        const uint32_t control = PERIPHERAL_BASE + iMaxPatches * 0x10;

        NetMDByteVector reply;

        // Write 5, 12 to main control
        if (mNetMd.cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #1.");
        }

        if (mNetMd.cleanWrite(control, {12}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #2.");
        }

        // AND 0xFE with patch control
        if ((mNetMd.cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #1.");
        }

        reply[0] &= 0xfe;

        if (mNetMd.cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #1.");
        }

        // AND 0xFD with patch control
        if ((mNetMd.cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #2.");
        }

        reply[0] &= 0xfd;

        if (mNetMd.cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #2.");
        }

        if ((reply = toLittleEndianByteVector(addr)).size() != sizeof(addr))
        {
            mNetMdThrow(NETMDERR_USB, "Error converting address into little endian byte vector.");
        }

        // Write patch address
        if (mNetMd.cleanWrite(base + 4, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch address.");
        }

        // Write patch VALUE
        if (mNetMd.cleanWrite(base + 8, data) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch data.");
        }

        // OR 1 with patch control
        if ((mNetMd.cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #3.");
        }

        reply[0] |= 0x01;

        if (mNetMd.cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #3.");
        }

        // write 5, 9 to main control
        if (mNetMd.cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #3.");
        }

        if (mNetMd.cleanWrite(control, {9}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #4.");
        }

    }
    catch(const ThrownData& e)
    {
        LOG(CRITICAL) << e.mErrDescr;
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      unpatch a patch
//!
//! @param[in]  pid   patch id of patch to undo
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::unpatch(PatchId pid)
{
    mLOG(DEBUG);
    try
    {
        int patch_number = patchUnused(pid);
        int iMaxPatches  = maxPatches();

        if (patch_number == -1)
        {
            mNetMdThrow(NETMDERR_PARAM, "Can't find patch to undo: " << static_cast<int>(pid));
        }

        if (iMaxPatches < 0)
        {
            mNetMdThrow(NETMDERR_PARAM, "Error with patch number!");
        }

        const uint32_t base    = PERIPHERAL_BASE + patch_number  * 0x10;
        const uint32_t control = PERIPHERAL_BASE + iMaxPatches   * 0x10;

        NetMDByteVector reply;

        // Write 5, 12 to main control
        if (mNetMd.cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #1.");
        }

        if (mNetMd.cleanWrite(control, {12}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #2.");
        }

        // AND 0xFE with patch control
        if ((mNetMd.cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #1.");
        }

        reply[0] &= 0xfe;

        if (mNetMd.cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #1.");
        }

        // write 5, 9 to main control
        if (mNetMd.cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #3.");
        }

        if (mNetMd.cleanWrite(control, {9}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #4.");
        }
    }
    catch(const ThrownData& e)
    {
        LOG(CRITICAL) << e.mErrDescr;
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      do safety patch of needed
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::safetyPatch()
{
    mLOG(DEBUG);
    PatchComplect pc;

    try
    {
        mLOG(DEBUG) << "Try to get device code ...";
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode == SDI(UNKNOWN)) || (devcode == SDI(NO_SUPPORT)))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }

        if (fillPatchComplect(PID_SAFETY, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NO_ERROR, "Safety patch data not found!");
        }
    
        NetMDByteVector cur_patch_data;
        uint32_t        cur_patch_addr = 0;
        int             safety_loaded  = 0;

        for (int i = 0; i < maxPatches(); i++)
        {
            if (readPatchData(i, cur_patch_addr, cur_patch_data) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't read patch data fro patch #" << i << ".");
            }

            if ((cur_patch_addr == pc.mAddr) && (cur_patch_data == pc.mPatchData))
            {
                mLOG(DEBUG) << "Safety patch found at patch slot #" << i << ".";
                safety_loaded   = 1;
                mUsedPatches[i] = PID_SAFETY;
            }

            // developer device
            if ((cur_patch_addr == 0xe6c0) || (cur_patch_addr == 0xe69c))
            {
                mLOG(DEBUG) << "Dev patch found at patch slot #" << i << ".";
                safety_loaded   = 1;
                mUsedPatches[i] = PID_SAFETY;
            }
        }

        if (safety_loaded == 0)
        {
            if (patch(pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't write safety patch.");
            }

            mLOG(DEBUG) << "Safety patch applied.";
        }
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      apply SP upload patch
//!
//! @param[in]  chanNo  The number of channels
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::applySpPatch(int chanNo)
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return NETMDERR_NOT_SUPPORTED;
    }

    PatchId         patch0 = PID_UNUSED;
    uint32_t        addr   = 0;
    NetMDByteVector data;

    try
    {
        mLOG(DEBUG) << "Try to get device code ...";
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode == SDI(UNKNOWN)) || (devcode == SDI(NO_SUPPORT)))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }

        mLOG(DEBUG) << "Apply safety patch ...";
        if (safetyPatch() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable safety patch!");
        }

        if ((devcode == SDI(S1100)) || (devcode == SDI(S1200)))
        {
            patch0 = PID_PATCH_0_B;
        }
        if ((devcode >= SDI(S_START)) && (devcode <= SDI(S_END)))
        {
            if ((addr = patchAddress(devcode, PID_DEVTYPE)) != 0)
            {
                if ((mNetMd.cleanRead(addr, 1, data) == NETMDERR_NO_ERROR) && !data.empty())
                {
                    patch0 = (data.at(0) == 1) ? PID_PATCH_0_B : PID_PATCH_0_A;
                }
            }
        }

        if (patch0 == PID_UNUSED)
        {
            mNetMdThrow(NETMDERR_USB, "Can't find out patch 0!");
        }

        PatchComplect pc;

        mLOG(DEBUG) << "=== Apply patch 0 ===";
        if (fillPatchComplect(patch0, devcode, pc, PID_PATCH_0) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data patch 0!");
        }
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch 0!");
        }

        mLOG(DEBUG) << "=== Apply patch common 1 ===";
        if (fillPatchComplect(PID_PATCH_CMN_1, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data patch common 1!");
        }
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch common 1!");
        }

        mLOG(DEBUG) << "=== Apply patch common 2 ===";
        if (fillPatchComplect(PID_PATCH_CMN_2, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data patch common 2!");
        }
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch common 2!");
        }

        mLOG(DEBUG) << "=== Apply prep patch ===";
        if (fillPatchComplect(PID_PREP_PATCH, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data prep patch!");
        }
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply prep patch!");
        }

        mLOG(DEBUG) << "=== Apply track type patch ===";
        if (fillPatchComplect(PID_TRACK_TYPE, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data track type!");
        }
        pc.mPatchData[1] = (chanNo == 1) ? 4 : 6; // mono or stereo
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't track type patch!");
        }
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      undo the SP upload patch
//--------------------------------------------------------------------------
void CNetMdPatch::undoSpPatch()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return;
    }

    mLOG(DEBUG) << "=== Undo patch 0 ===";
    static_cast<void>(unpatch(PID_PATCH_0));

    mLOG(DEBUG) << "=== Undo patch common 1 ===";
    static_cast<void>(unpatch(PID_PATCH_CMN_1));

    mLOG(DEBUG) << "=== Undo patch common 2 ===";
    static_cast<void>(unpatch(PID_PATCH_CMN_2));

    mLOG(DEBUG) << "=== Undo prep patch ===";
    static_cast<void>(unpatch(PID_PREP_PATCH));

    mLOG(DEBUG) << "=== Undo track type patch ===";
    static_cast<void>(unpatch(PID_TRACK_TYPE));
}

//--------------------------------------------------------------------------
//! @brief      apply the PCM to mono patch
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::applyPCM2MonoPatch()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return NETMDERR_NOT_SUPPORTED;
    }

    PatchId         patch0 = PID_UNUSED;
    uint32_t        addr   = 0;
    NetMDByteVector data;
    SonyDevInfo     devcode;

    try
    {
        mLOG(DEBUG) << "Try to get device code ...";
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode == SDI(UNKNOWN)) || (devcode == SDI(NO_SUPPORT)))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }

        mLOG(DEBUG) << "Apply safety patch ...";
        if (safetyPatch() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable safety patch!");
        }

        mLOG(DEBUG) << "Check if patch is already active ...";
        if (checkPatch(PID_PCM_TO_MONO, devcode) == 1)
        {
            mNetMdThrow(NETMDERR_NO_ERROR, "PCM to Mono Patch already active!");
        }

        PatchComplect pc;

        mLOG(DEBUG) << "=== Apply PCM to mono patch ===";
        if (fillPatchComplect(PID_PCM_TO_MONO, devcode, pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data for PCM to mono patch!");
        }
        if (patch(pc) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply PCM to mono patch");
        }
        mLOG(DEBUG) << "=== PCM to mono patch applied successfully ===";
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      undo the PCM to mono patch
//--------------------------------------------------------------------------
void CNetMdPatch::undoPCM2MonoPatch()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return;
    }

    mLOG(DEBUG) << "=== Undo PCM to mono patch ===";
    static_cast<void>(unpatch(PID_PCM_TO_MONO));
}

//--------------------------------------------------------------------------
//! @brief      check if device supports SP upload
//!
//! @return     true -> supports; false -> doesn't support
//--------------------------------------------------------------------------
bool CNetMdPatch::supportsSpUpload()
{
    mLOG(DEBUG);
    bool ret = false;

    // only available on Sony devices!
    if (mNetMd.isMaybePatchable())
    {
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode >= SDI(S_START)) && (devcode <= SDI(S_END)))
        {
            mLOG(DEBUG) << "Supported device!";
            ret = true;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      is TOC manipulation supported?
//!
//! @return     true if supported, false if not
//--------------------------------------------------------------------------
bool CNetMdPatch::tocManipSupported()
{
    mLOG(DEBUG);
    bool ret = false;

    // only available on Sony devices!
    if (mNetMd.isMaybePatchable())
    {
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode != SDI(UNKNOWN)) && (devcode != SDI(NO_SUPPORT)))
        {
            mLOG(DEBUG) << "Supported device!";
            ret = true;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      is PCM speedup supportd
//!
//! @return     true if supported, false if not
//--------------------------------------------------------------------------
bool CNetMdPatch::pcmSpeedupSupported()
{
    mLOG(DEBUG);
    bool ret = false;

    // only available on Sony devices!
    if (mNetMd.isMaybePatchable())
    {
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if ((devcode >= SDI(S_START)) && (devcode <= SDI(S_END)))
        {
            mLOG(DEBUG) << "Supported device!";
            ret = true;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      apply PCM speedup patch
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::applyPCMSpeedupPatch()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return NETMDERR_NOT_SUPPORTED;
    }

    try
    {
        mLOG(DEBUG) << "Try to get device code ...";
        SonyDevInfo devcode = mNetMd.sonyDevCode();
        if (!((devcode >= SDI(S_START)) && (devcode <= SDI(S_END))))
        {
            mNetMdThrow(NETMDERR_OTHER, "Unknown or unsupported NetMD device!");
        }
        
        mLOG(DEBUG) << "Apply safety patch ...";
        if (safetyPatch() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable safety patch!");
        }

        PatchComplect pc;
        
        // check, if patch is already active ...
        if (checkPatch(PID_PCM_SPEEDUP_1, devcode) == 0)
        {
            // patch ...
            mLOG(DEBUG) << "=== Apply PCM Speedup Patch #1 ===";
            if (fillPatchComplect(PID_PCM_SPEEDUP_1, devcode, pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data PCM Speedup Patch #1!");
            }
            if (patch(pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't apply PCM Speedup Patch #1!");
            }
        }

        // check, if patch is already active ...
        if (checkPatch(PID_PCM_SPEEDUP_2, devcode) == 0)
        {
            // patch ...
            mLOG(DEBUG) << "=== Apply PCM Speedup Patch #2 ===";
            if (fillPatchComplect(PID_PCM_SPEEDUP_2, devcode, pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_NOT_SUPPORTED, "Can't find patch data PCM Speedup Patch #2!");
            }
            if (patch(pc) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't apply PCM Speedup Patch #2!");
            }
        }
    }
    catch(const ThrownData& e)
    {
        if (e.mErr == NETMDERR_NO_ERROR)
        {
            LOG(DEBUG) << e.mErrDescr;
        }
        else
        {
            LOG(CRITICAL) << e.mErrDescr;
        }
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while patching NetMD Device!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      apply PCM speedup patch
//--------------------------------------------------------------------------
void CNetMdPatch::undoPCMSpeedupPatch()
{
    mLOG(DEBUG);
    if (!mNetMd.isMaybePatchable())
    {
        return;
    }

    mLOG(DEBUG) << "=== Undo PCM Speedup Patch #1 ===";
    static_cast<void>(unpatch(PID_PCM_SPEEDUP_1));

    mLOG(DEBUG) << "=== Undo PCM Speedup Patch #2 ===";
    static_cast<void>(unpatch(PID_PCM_SPEEDUP_2));
}

} // ~namespace
