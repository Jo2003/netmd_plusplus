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
#include "netmd_utils.h"
#include <sstream>

namespace netmd {

/// patch addresses
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

/// patch payload
const CNetMdPatch::PatchPayloadTab CNetMdPatch::smPatchPayloadTab = {
    {PID_PATCH_0    , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x00,0x00,0xa0,0xe1}}},
    {PID_PREP_PATCH , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x0D,0x31,0x01,0x60}}},
    {PID_PATCH_CMN_1, {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x80,0x80,0x03}}},
    {PID_PATCH_CMN_2, {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x14,0x90,0x80,0x03}}},
    {PID_TRACK_TYPE , {SDI_S1200 | SDI_S1300 | SDI_S1400 | SDI_S1500 | SDI_S1600, {0x06,0x02,0x00,0x04}}},
    {PID_SAFETY     , {                        SDI_S1400 | SDI_S1500 | SDI_S1600, {0xdc,0xff,0xff,0xea}}}, //< anti brick patch
};

/// used patches
CNetMdPatch::PatchId CNetMdPatch::smUsedPatches[MAX_PATCH] = {PID_UNUSED,};

//--------------------------------------------------------------------------
//! @brief      get next pree patch index
//!
//! @param[in]  pid   The pid
//!
//! @return     -1 -> no more free | > -1 -> free patch index
//--------------------------------------------------------------------------
int CNetMdPatch::nextFreePatch(PatchId pid)
{
    for (int i = 0; i < MAX_PATCH; i++)
    {
        if (smUsedPatches[i] == PID_UNUSED)
        {
            smUsedPatches[i] = pid;
            return i;
        }
    }
    return -1;
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
        if (devinfo & pl->second.mDevs)
        {
            return pl->second.mPtData;
        }
    }

    return NetMDByteVector{};
}

//------------------------------------------------------------------------------
//! @brief      write patch data
//!
//! @param[in]  addr      address
//! @param[in]  data      data to write
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdPatch::patchWrite(uint32_t addr, const NetMDByteVector& data)
{
    mLOG(DEBUG);
    int ret;
    const char* format = "00 1822 ff 00 %<d %b 0000 %* %<w";

    NetMDResp query;

    if (((ret = formatQuery(format, {{addr}, {mBYTE(data.size())}, {data}, calculateChecksum(data)}, query)) >= 15)
        && (query != nullptr))
    {
        if ((ret = mNetMd.exchange(query.get(), ret, nullptr, true)) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      read patch data
//!
//! @param[in]  addr      address
//! @param[in]  size      size to read
//! @param[in]  data      read data
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdPatch::patchRead(uint32_t addr, uint8_t size, NetMDByteVector& data)
{
    mLOG(DEBUG);
    int ret;
    const char* format  = "00 1821 ff 00 %<d %b";
    const char* capture = "%? 1821 00 %? %?%?%?%? %? %?%? %*";

    NetMDResp query, response;
    NetMDParams params;

    if (((ret = formatQuery(format, {{addr}, {size}}, query)) == 10) && (query != nullptr))
    {
        if (((ret = mNetMd.exchange(query.get(), ret, &response, true)) > 0) && (response != nullptr))
        {
            if ((ret = scanQuery(response.get(), ret, capture, params)) == NETMDERR_NO_ERROR)
            {
                if (params.at(0).index() == BYTE_VECTOR)
                {
                    data = simple_get<NetMDByteVector>(params.at(0));

                    // ignore last two bytes (checksum)
                    data.pop_back();
                    data.pop_back();

                    ret = NETMDERR_NO_ERROR;
                }
            }
        }
    }

    if (ret != NETMDERR_NO_ERROR)
    {
        ret = NETMDERR_OTHER;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      change memory access state
//!
//! @param[in]  addr  The address
//! @param[in]  size  The size
//! @param[in]  acc   The memory access acc
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::changeMemState(uint32_t addr, uint8_t size, MemAcc acc)
{
    mLOG(DEBUG);
    int ret;
    const char* format = "00 1820 ff 00 %<d %b %b 00";

    NetMDResp query;

    if (((ret = formatQuery(format, {{addr}, {size}, {mBYTE(acc)}}, query)) == 12)
        && (query != nullptr))
    {
        if ((ret = mNetMd.exchange(query.get(), ret, nullptr, true)) > 0)
        {
            ret = NETMDERR_NO_ERROR;
        }
    }

    if (ret != NETMDERR_NO_ERROR)
    {
        ret = NETMDERR_OTHER;
    }

    return ret;
}

//------------------------------------------------------------------------------
//! @brief      open for read, read, close
//!
//! @param[in]  addr     address
//! @param[in]  sz       size of data to read
//! @param[out] data     read data
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdPatch::cleanRead(uint32_t addr, uint8_t sz, NetMDByteVector& data)
{
    mLOG(DEBUG);
    int ret;
    static_cast<void>(changeMemState(addr, sz, MemAcc::NETMD_MEM_READ));
    ret = patchRead(addr, sz, data);
    static_cast<void>(changeMemState(addr, sz, MemAcc::NETMD_MEM_CLOSE));
    return ret;
}

//------------------------------------------------------------------------------
//! @brief      open for write, write, close
//!
//! @param[in]  addr     address
//! @param[in]  data     data to write
//!
//! @return     NetMdErr
//! @see        NetMdErr
//------------------------------------------------------------------------------
int CNetMdPatch::cleanWrite(uint32_t addr, const NetMDByteVector& data)
{
    mLOG(DEBUG);
    int ret;
    static_cast<void>(changeMemState(addr, data.size(), MemAcc::NETMD_MEM_WRITE));
    ret = patchWrite(addr, data);
    static_cast<void>(changeMemState(addr, data.size(), MemAcc::NETMD_MEM_CLOSE));
    return ret;
}

//--------------------------------------------------------------------------
//! @brief      get device code
//!
//! @return     SonyDevInfo
//! @see        SonyDevInfo
//--------------------------------------------------------------------------
CNetMdPatch::SonyDevInfo CNetMdPatch::devCodeEx()
{
    mLOG(DEBUG);
    SonyDevInfo ret = SonyDevInfo::SDI_UNKNOWN;
    uint8_t query[] = {0x00, 0x18, 0x12, 0xff};
    uint8_t chip    = 255, hwid = 255, version = 255;

    std::ostringstream code;

    NetMDResp respone;

    if ((mNetMd.exchange(query, sizeof(query), &respone, true) >= 8) && (respone != nullptr))
    {
        chip    = respone[4];
        hwid    = respone[5];
        version = respone[7];

        if ((chip != 255) || (hwid != 255) || (version != 255))
        {
            switch (chip)
            {
            case 0x20:
                code << "R";
                break;
            case 0x21:
                code << "S";
                break;
            case 0x24:
                code << "Hi";
                break;
            default:
                code << "0x" << std::hex << std::setw(2) << std::setfill('0')
                     << static_cast<int>(chip) << std::dec;
                break;
            }

            code << static_cast<int>(version >> 4) << "." << static_cast<int>(version & 0xf) << "00";

            mLOG(DEBUG) << "Found device info: " << code.str();

            if (code.str() == "S1.600")
            {
                ret = SonyDevInfo::SDI_S1600;
            }
            else if (code.str() == "S1.200")
            {
                ret = SonyDevInfo::SDI_S1200;
            }
            else if (code.str() == "S1.300")
            {
                ret = SonyDevInfo::SDI_S1300;
            }
            else if (code.str() == "S1.400")
            {
                ret = SonyDevInfo::SDI_S1400;
            }
            else if (code.str() == "S1.500")
            {
                ret = SonyDevInfo::SDI_S1500;
            }
        }
    }

    return ret;
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

    if ((cleanRead(base + 4, 4, reply) == NETMDERR_NO_ERROR) && (reply.size() == 4))
    {
        addr = fromLittleEndianByteVector<uint32_t>(reply);

        if ((cleanRead(base + 8, 4, patch) != NETMDERR_NO_ERROR) || (patch.size() != 4))
        {
            ret = NETMDERR_USB;
        }
    }

    return ret;
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

        const uint32_t base    = PERIPHERAL_BASE + patchNo   * 0x10;
        const uint32_t control = PERIPHERAL_BASE + MAX_PATCH * 0x10;

        NetMDByteVector reply;

        // Write 5, 12 to main control
        if (cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #1.");
        }

        if (cleanWrite(control, {12}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #2.");
        }

        // AND 0xFE with patch control
        if ((cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #1.");
        }

        reply[0] &= 0xfe;

        if (cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #1.");
        }

        // AND 0xFD with patch control
        if ((cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #2.");
        }

        reply[0] &= 0xfd;

        if (cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #2.");
        }

        if ((reply = toLittleEndianByteVector(addr)).size() != sizeof(addr))
        {
            mNetMdThrow(NETMDERR_USB, "Error converting address into little endian byte vector.");
        }

        // Write patch address
        if (cleanWrite(base + 4, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch address.");
        }

        // Write patch VALUE
        if (cleanWrite(base + 8, data) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch data.");
        }

        // OR 1 with patch control
        if ((cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #3.");
        }

        reply[0] |= 0x01;

        if (cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #3.");
        }

        // write 5, 9 to main control
        if (cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #3.");
        }

        if (cleanWrite(control, {9}) != NETMDERR_NO_ERROR)
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
        int patch_number = -1;

        for (int i = 0; i < MAX_PATCH; i++)
        {
            if (smUsedPatches[i] == pid)
            {
                smUsedPatches[i] = PID_UNUSED;
                patch_number = i;
            }
        }

        if (patch_number == -1)
        {
            mNetMdThrow(NETMDERR_PARAM, "Can't find patch to undo: " << static_cast<int>(pid));
        }

        const uint32_t base    = PERIPHERAL_BASE + patch_number  * 0x10;
        const uint32_t control = PERIPHERAL_BASE + MAX_PATCH     * 0x10;

        NetMDByteVector reply;

        // Write 5, 12 to main control
        if (cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #1.");
        }

        if (cleanWrite(control, {12}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #2.");
        }

        // AND 0xFE with patch control
        if ((cleanRead(base, 4, reply) != NETMDERR_NO_ERROR) || (reply.size() != 4))
        {
            mNetMdThrow(NETMDERR_USB, "Error while reading patch control #1.");
        }

        reply[0] &= 0xfe;

        if (cleanWrite(base, reply) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing patch control #1.");
        }

        // write 5, 9 to main control
        if (cleanWrite(control, {5}) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while writing main control #3.");
        }

        if (cleanWrite(control, {9}) != NETMDERR_NO_ERROR)
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
    SonyDevInfo     devcode   = devCodeEx();
    uint32_t        addr      = patchAddress(devcode, PID_SAFETY);
    NetMDByteVector patch_cnt = patchPayload(devcode, PID_SAFETY);

    try
    {
        NetMDByteVector cur_patch_data;
        uint32_t        cur_patch_addr = 0;
        int             safety_loaded = 0;

        if ((addr == 0) || patch_cnt.empty())
        {
            mNetMdThrow(NETMDERR_NO_ERROR, "Safety patch data not found!");
        }

        for (int i = 0; i < MAX_PATCH; i++)
        {
            if (readPatchData(i, cur_patch_addr, cur_patch_data) != NETMDERR_NO_ERROR)
            {
                mNetMdThrow(NETMDERR_USB, "Can't read patch data fro patch #" << i << ".");
            }

            if ((cur_patch_addr == addr) && (cur_patch_data == patch_cnt))
            {
                mLOG(DEBUG) << "Safety patch found at patch slot #" << i << ".";
                safety_loaded   = 1;
                smUsedPatches[i] = PID_SAFETY;
            }

            // developer device
            if ((cur_patch_addr == 0xe6c0) || (cur_patch_addr == 0xe69c))
            {
                mLOG(DEBUG) << "Dev patch found at patch slot #" << i << ".";
                safety_loaded   = 1;
                smUsedPatches[i] = PID_SAFETY;
            }
        }

        if (safety_loaded == 0)
        {
            if (patch(addr, patch_cnt, nextFreePatch(PID_SAFETY)) != NETMDERR_NO_ERROR)
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
//! @brief      Enables the factory mode.
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdPatch::enableFactory()
{
    mLOG(DEBUG);
    int ret = NETMDERR_NO_ERROR;
    uint8_t p1[]  = {0x00, 0x18, 0x09, 0x00, 0xff, 0x00, 0x00, 0x00,
                     0x00, 0x00};

    uint8_t p2[]  = {0x00, 0x18, 0x01, 0xff, 0x0e, 0x4e, 0x65, 0x74,
                     0x20, 0x4d, 0x44, 0x20, 0x57, 0x61, 0x6c, 0x6b,
                     0x6d, 0x61, 0x6e};

    if (mNetMd.changeDscrtState(CNetMdDev::Descriptor::discSubunitIndentifier,
                                CNetMdDev::DscrtAction::openread) != NETMDERR_NO_ERROR)
    {
        ret = NETMDERR_USB;
    }

    if (mNetMd.exchange(p1, sizeof(p1)) <= 0)
    {
        ret = NETMDERR_USB;
    }

    if (mNetMd.exchange(p2, sizeof(p2), nullptr, true) <= 0)
    {
        ret = NETMDERR_USB;
    }

    return ret;
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
    SonyDevInfo     devcode;

    try
    {
        mLOG(DEBUG) << "Enable factory ...";
        if (enableFactory() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable factory mode!");
        }

        mLOG(DEBUG) << "Apply safety patch ...";
        if (safetyPatch() != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't enable safety patch!");
        }

        mLOG(DEBUG) << "Try to get device code ...";
        if ((devcode = devCodeEx()) == SDI_S1200)
        {
            patch0 = PID_PATCH_0_B;
        }
        else if (devcode != SDI_UNKNOWN)
        {
            if ((addr = patchAddress(devcode, PID_DEVTYPE)) != 0)
            {
                if ((cleanRead(addr, 1, data) == NETMDERR_NO_ERROR) && !data.empty())
                {
                    patch0 = (data.at(0) == 1) ? PID_PATCH_0_B : PID_PATCH_0_A;
                }
            }
        }

        if (patch0 == PID_UNUSED)
        {
            mNetMdThrow(NETMDERR_USB, "Can't find out patch 0!");
        }

        mLOG(DEBUG) << "=== Apply patch 0 ===";
        if (patch(patchAddress(devcode, patch0),
                  patchPayload(devcode, PID_PATCH_0),
                  nextFreePatch(PID_PATCH_0)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch 0!");
        }

        mLOG(DEBUG) << "=== Apply patch common 1 ===";
        if (patch(patchAddress(devcode, PID_PATCH_CMN_1),
                  patchPayload(devcode, PID_PATCH_CMN_1),
                  nextFreePatch(PID_PATCH_CMN_1)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch common 1!");
        }

        mLOG(DEBUG) << "=== Apply patch common 2 ===";
        if (patch(patchAddress(devcode, PID_PATCH_CMN_2),
                  patchPayload(devcode, PID_PATCH_CMN_2),
                  nextFreePatch(PID_PATCH_CMN_2)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply patch common 2!");
        }

        mLOG(DEBUG) << "=== Apply prep patch ===";
        if (patch(patchAddress(devcode, PID_PREP_PATCH),
                  patchPayload(devcode, PID_PREP_PATCH),
                  nextFreePatch(PID_PREP_PATCH)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Can't apply prep patch!");
        }

        mLOG(DEBUG) << "=== Apply track type patch ===";
        data    = patchPayload(devcode, PID_TRACK_TYPE);
        data[1] = (chanNo == 1) ? 4 : 6; // mono or stereo
        if (patch(patchAddress(devcode, PID_TRACK_TYPE),
                  data,
                  nextFreePatch(PID_TRACK_TYPE)) != NETMDERR_NO_ERROR)
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
        mLOG(DEBUG) << "Enable factory ...";
        if (enableFactory() == NETMDERR_NO_ERROR)
        {
            mLOG(DEBUG) << "Get extended device info!";
            if (devCodeEx() != SDI_UNKNOWN)
            {
                mLOG(DEBUG) << "Supported device!";
                ret = true;
            }
        }
    }

    return ret;
}

} // ~namespace
