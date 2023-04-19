/*
 * CNetMdPatch.h
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
#include "CNetMdDev.hpp"
#include <cstdint>

namespace netmd {

class CNetMdSecure;

//------------------------------------------------------------------------------
//! @brief      This class describes a net md patch.
//------------------------------------------------------------------------------
class CNetMdPatch
{
    // make friends
    friend CNetMdSecure;

    static constexpr uint32_t PERIPHERAL_BASE = 0x03802000;
    static constexpr uint8_t  MAX_PATCH       = 8;

    /// device info flags
    enum SonyDevInfo : uint32_t
    {
        SDI_S1200   = (1ul <<  0),    //!< S1.200 version
        SDI_S1300   = (1ul <<  1),    //!< S1.300 version
        SDI_S1400   = (1ul <<  2),    //!< S1.400 version
        SDI_S1500   = (1ul <<  3),    //!< S1.500 version
        SDI_S1600   = (1ul <<  4),    //!< S1.600 version
        SDI_UNKNOWN = (1ul << 31),    //!< unsupported or unknown
    };

    /// patch IDs
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

    /// memory access
    enum MemAcc : uint8_t
    {
        NETMD_MEM_CLOSE      = 0x0,
        NETMD_MEM_READ       = 0x1,
        NETMD_MEM_WRITE      = 0x2,
        NETMD_MEM_READ_WRITE = 0x3,
    };

    /// payload structure
    struct PayLoad
    {
        uint32_t mDevs;
        NetMDByteVector mPtData;
    };

    /// type defines
    using PatchAddr       = std::map<SonyDevInfo, uint32_t>;
    using PatchAdrrTab    = std::map<PatchId, PatchAddr>;
    using PatchPayloadTab = std::map<PatchId, PayLoad>;

    /// patch addresses with for devices
    static const PatchAdrrTab    smPatchAddrTab;

    /// patch payload for devices
    static const PatchPayloadTab smPatchPayloadTab;

    //! @brief patch areas used
    static PatchId smUsedPatches[MAX_PATCH];

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param      netMd  The net md device reference
    //--------------------------------------------------------------------------
    CNetMdPatch(CNetMdDev& netMd) : mNetMd(netMd)
    {}

    //--------------------------------------------------------------------------
    //! @brief      get next pree patch index
    //!
    //! @param[in]  pid   The pid
    //!
    //! @return     -1 -> no more free | > -1 -> free patch index
    //--------------------------------------------------------------------------
    static int nextFreePatch(PatchId pid);

    //------------------------------------------------------------------------------
    //! @brief      get patch address by name and device info
    //!
    //! @param[in]  devinfo    device info
    //! @param[in]  pid        patch id
    //!
    //! @return     0 -> error | > 0 -> address
    //------------------------------------------------------------------------------
    static uint32_t patchAddress(SonyDevInfo devinfo, PatchId pid);

    //------------------------------------------------------------------------------
    //! @brief      get patch payload by name and device info
    //!
    //! @param[in]  devinfo    device info
    //! @param[in]  pid        patch id
    //!
    //! @return     empty vector -> error; else -> patch content
    //------------------------------------------------------------------------------
    static NetMDByteVector patchPayload(SonyDevInfo devinfo, PatchId pid);

    //------------------------------------------------------------------------------
    //! @brief      write patch data
    //!
    //! @param[in]  addr      address
    //! @param[in]  data      data to write
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //------------------------------------------------------------------------------
    int patchWrite(uint32_t addr, const NetMDByteVector& data);

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
    int patchRead(uint32_t addr, uint8_t size, NetMDByteVector& data);

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
    int changeMemState(uint32_t addr, uint8_t size, MemAcc acc);

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
    int cleanRead(uint32_t addr, uint8_t sz, NetMDByteVector& data);

    //------------------------------------------------------------------------------
    //! @brief      open for write, write, close
    //!
    //! @param[in]  addr     address
    //! @param[in]  data     data to write
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //------------------------------------------------------------------------------
    int cleanWrite(uint32_t addr, const NetMDByteVector& data);

    //--------------------------------------------------------------------------
    //! @brief      get device code
    //!
    //! @return     SonyDevInfo
    //! @see        SonyDevInfo
    //--------------------------------------------------------------------------
    SonyDevInfo devCodeEx();

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
    int readPatchData(int patchNo, uint32_t& addr, NetMDByteVector& patch);

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
    int patch(uint32_t addr, const NetMDByteVector& data, int patchNo);

    //--------------------------------------------------------------------------
    //! @brief      unpatch a patch
    //!
    //! @param[in]  pid   patch id of patch to undo
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int unpatch(PatchId pid);

    //--------------------------------------------------------------------------
    //! @brief      do safety patch of needed
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int safetyPatch();

    //--------------------------------------------------------------------------
    //! @brief      Enables the factory mode.
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int enableFactory();

    //--------------------------------------------------------------------------
    //! @brief      apply SP upload patch
    //!
    //! @param[in]  chanNo  The number of channels
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int applySpPatch(int chanNo);

    //--------------------------------------------------------------------------
    //! @brief      undo the SP upload patch
    //--------------------------------------------------------------------------
    void undoSpPatch();

    //--------------------------------------------------------------------------
    //! @brief      check if device supports SP upload
    //!
    //! @return     true -> supports; false -> doesn't support
    //--------------------------------------------------------------------------
    bool supportsSpUpload();

    CNetMdDev& mNetMd;
};

} // ~namespace