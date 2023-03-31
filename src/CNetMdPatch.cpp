/*
 * CNetMdAPatch.cpp
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
#include "CNetMdPatch.h"

namespace netmd {

const CNetMdPatch::PatchAdrrTab CNetMdPatch::smPatchAddrTab = {
    {PID_DEVTYPE    , {{SDI_S1600, 0x02003fcf},{SDI_S1500, 0x02003fc7},{SDI_S1400, 0x03000220},{SDI_S1300, 0x02003e97}}},
    {PID_PATCH_0_A  , {{SDI_S1600, 0x0007f408},{SDI_S1500, 0x0007e988},{SDI_S1400, 0x0007e2c8},{SDI_S1300, 0x0007aa00}}},
    {PID_PATCH_0_B  , {{SDI_S1600, 0x0007efec},{SDI_S1500, 0x0007e56c},{SDI_S1400, 0x0007deac},{SDI_S1300, 0x0007a5e4},{SDI_S1200, 0x00078dcc}}},
    {PID_PREP_PATCH , {{SDI_S1600, 0x00077c04},{SDI_S1500, 0x0007720c},{SDI_S1400, 0x00076b38},{SDI_S1300, 0x00073488},{SDI_S1200, 0x00071e5c}}},
    {PID_PATCH_CMN_1, {{SDI_S1600, 0x0007f4e8},{SDI_S1500, 0x0007ea68},{SDI_S1400, 0x0007e3a8},{SDI_S1300, 0x0007aae0},{SDI_S1200, 0x00078eac}}},
    {PID_PATCH_CMN_2, {{SDI_S1600, 0x0007f4ec},{SDI_S1500, 0x0007ea6c},{SDI_S1400, 0x0007e3ac},{SDI_S1300, 0x0007aae4},{SDI_S1200, 0x00078eb0}}},
    {PID_TRACK_TYPE , {{SDI_S1600, 0x000852b0},{SDI_S1500, 0x00084820},{SDI_S1400, 0x00084160},{SDI_S1300, 0x00080798},{SDI_S1200, 0x0007ea9c}}},
    {PID_SAFETY     , {{SDI_S1600, 0x000000c4},{SDI_S1500, 0x000000c4},{SDI_S1400, 0x000000c4},{SDI_S1300, 0x000000c4}}}, //< anti brick patch
};

const CNetMdPatch::PatchPayloadTab CNetMdPatch::smPatchPayloadTab = {
    {PID_PATCH_0    , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x00,0x00,0xa0,0xe1}}},
    {PID_PREP_PATCH , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x0D,0x31,0x01,0x60}}},
    {PID_PATCH_CMN_1, {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x80,0x80,0x03}}},
    {PID_PATCH_CMN_2, {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x90,0x80,0x03}}},
    {PID_TRACK_TYPE , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x06,0x02,0x00,0x04}}},
    {PID_SAFETY     , {                        SDI_S1400 | SDI_S1500 | SDI_S1600, {0xdc,0xff,0xff,0xea}}}, //< anti brick patch
};


} // ~namespace