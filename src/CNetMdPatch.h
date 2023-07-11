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
#include "netmd_defines.h"
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
    static constexpr uint8_t  MAX_PATCH       = 16; ///< HiMD supports up to 16

    /// device info flags
    enum SonyDevInfo : uint32_t
    {
        SDI_R1000   = (1ul <<  0),    //!< R1.000 version
        SDI_R1100   = (1ul <<  1),    //!< R1.100 version
        SDI_R1200   = (1ul <<  2),    //!< R1.200 version
        SDI_R1300   = (1ul <<  3),    //!< R1.300 version
        SDI_R1400   = (1ul <<  4),    //!< R1.400 version
        SDI_R_START = SDI_R1000,
        SDI_R_END   = SDI_R1400,
        SDI_S1000   = (1ul <<  5),    //!< S1.000 version
        SDI_S1100   = (1ul <<  6),    //!< S1.100 version
        SDI_S1200   = (1ul <<  7),    //!< S1.200 version
        SDI_S1300   = (1ul <<  8),    //!< S1.300 version
        SDI_S1400   = (1ul <<  9),    //!< S1.400 version
        SDI_S1500   = (1ul << 10),    //!< S1.500 version
        SDI_S1600   = (1ul << 11),    //!< S1.600 version
        SDI_S_START = SDI_S1000,
        SDI_S_END   = SDI_S1600,
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
        PID_USB_EXE,
        PID_PCM_SPEEDUP_1,
        PID_PCM_SPEEDUP_2,
    };

    /// exploit ID
    enum ExploitId : uint8_t
    {
        EID_LOWER_HEAD,
        EID_RAISE_HEAD,
        EID_TRIGGER,
        EID_DEV_RESET,
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
    using PatchAddr         = std::map<SonyDevInfo, uint32_t>;
    using PatchAdrrTab      = std::map<PatchId, PatchAddr>;
    using PatchPayloadTab   = std::map<PatchId, std::vector<PayLoad>>;
    using ExploitPayload    = std::map<SonyDevInfo, NetMDByteVector>;
    using ExploitPayloadTab = std::map<ExploitId, ExploitPayload>;
    using ExploitCmds       = std::map<uint32_t, uint8_t>;

    /// structure packed with patch information
    struct PatchComplect
    {
        SonyDevInfo mDev;           ///< device
        PatchId mPid;               ///< patch id
        uint32_t mAddr;             ///< patch address
        NetMDByteVector mPatchData; ///< patch data
        int mNextFreePatch;         ///< next free patch slot
    };

    //--------------------------------------------------------------------------
    //! @brief      print helper for SonyDevInfo
    //!
    //! @param[in, out] os ref. to ostream
    //! @param[in]      dinfo device info
    //!
    //! @returns       ref. to os
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const SonyDevInfo& dinfo);

    //--------------------------------------------------------------------------
    //! @brief      print helper for PatchId
    //!
    //! @param[in, out] os ref. to ostream
    //! @param[in]      pid patch id
    //!
    //! @returns       ref. to os
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const PatchId& pid);

    //--------------------------------------------------------------------------
    //! @brief      print helper for NetMDByteVector
    //!
    //! @param[in, out] os ref. to ostream
    //! @param[in]      pdata patch data
    //!
    //! @returns       ref. to os
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const NetMDByteVector& pdata);

    //--------------------------------------------------------------------------
    //! @brief      print helper for PatchComplect
    //!
    //! @param[in, out] os ref. to ostream
    //! @param[in]      patch patch complect
    //!
    //! @returns       ref. to os
    //--------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const PatchComplect& patch);

    /// patch addresses with for devices
    static const PatchAdrrTab    smPatchAddrTab;

    /// patch payload for devices
    static const PatchPayloadTab smPatchPayloadTab;

    /// exploit payload tab
    static const ExploitPayloadTab smExplPayloadTab;

    /// exmploit command lookup
    static const ExploitCmds smExploidCmds;

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param      netMd  The net md device reference
    //--------------------------------------------------------------------------
    CNetMdPatch(CNetMdDev& netMd); 

    //--------------------------------------------------------------------------
    //! @brief      get number of max patches
    //!
    //! @return     -1 -> error; else max number of patches
    //--------------------------------------------------------------------------
    int maxPatches() const;

    //--------------------------------------------------------------------------
    //! @brief      get next pree patch index
    //!
    //! @param[in]  pid   The pid
    //!
    //! @return     -1 -> no more free | > -1 -> free patch index
    //--------------------------------------------------------------------------
    int nextFreePatch(PatchId pid);

    //--------------------------------------------------------------------------
    //! @brief      mark patch as unused
    //!
    //! @param[in]  pid   The pid
    //!
    //! @return     -1 -> not found | > -1 -> last used patch index
    //--------------------------------------------------------------------------
    int patchUnused(PatchId pid);

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

    //--------------------------------------------------------------------------
    //! @brief      get exploit command
    //!
    //! @param[in]  devinfo  The devinfo
    //!
    //! @return     the exploit command; 0 on error
    //--------------------------------------------------------------------------
    static uint8_t exploitCmd(SonyDevInfo devinfo);

    //--------------------------------------------------------------------------
    //! @brief      get exploit data
    //!
    //! @param[in]  devinfo  The device info
    //! @param[in]  eid      The exploit id
    //!
    //! @return     on error: empty byte vector
    //--------------------------------------------------------------------------
    static NetMDByteVector exploitData(SonyDevInfo devinfo, ExploitId eid);

    //--------------------------------------------------------------------------
    //! @brief      check if patch is active
    //!
    //! @param[in]  pid      The patch id
    //! @param[in]  devinfo  The device info
    //!
    //! @return     1 -> patch is active; 0 -> not active
    //--------------------------------------------------------------------------
    int checkPatch(PatchId pid, SonyDevInfo devinfo);

    //--------------------------------------------------------------------------
    //! @brief      Reads metadata peripheral.
    //!
    //! @param[in]  sector  The sector
    //! @param[in]  offset  The offset
    //! @param[in]  length  The length
    //!
    //! @return     NetMDByteVector
    //--------------------------------------------------------------------------
    NetMDByteVector readMetadataPeripheral(uint16_t sector, uint16_t offset, uint8_t length);

    //--------------------------------------------------------------------------
    //! @brief      Writes metadata peripheral.
    //!
    //! @param[in]  sector  The sector
    //! @param[in]  offset  The offset
    //! @param[in]  data    The data
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int writeMetadataPeripheral(uint16_t sector, uint16_t offset, const NetMDByteVector& data);

    //--------------------------------------------------------------------------
    //! @brief      Reads an utoc sector.
    //!
    //! @param[in]  s     sector name
    //!
    //! @return     TOC sector data. (error if empty)
    //--------------------------------------------------------------------------
    NetMDByteVector readUTOCSector(UTOCSector s);

    //--------------------------------------------------------------------------
    //! @brief      Writes an utoc sector.
    //!
    //! @param[in]  s     sector names
    //! @param[in]  data  The data to be written
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int writeUTOCSector(UTOCSector s, const NetMDByteVector& data);

    //--------------------------------------------------------------------------
    //! @brief      prepare TOC manipulation
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int prepareTOCManip();

    //--------------------------------------------------------------------------
    //! @brief      finalize TOC though exploit
    //!
    //! @param[in]  reset  do device reset if true
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int finalizeTOC(bool reset);

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
    int USBExecute(SonyDevInfo devInfo, const NetMDByteVector& execData,
                   NetMDResp* pResp = nullptr, bool sendOnly = false);

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
    //! @brief      do one patch
    //!
    //! @param[in]  pc    The patch complect
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int patch(const PatchComplect& pc);

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

    //--------------------------------------------------------------------------
    //! @brief      is TOC manipulation supported?
    //!
    //! @return     true if supported, false if not
    //--------------------------------------------------------------------------
    bool tocManipSupported();

    //--------------------------------------------------------------------------
    //! @brief      check if USB execution works
    //!
    //! @param[in]  devcode   The device information
    //!
    //! @return     true if it works, false if not
    //--------------------------------------------------------------------------
    bool checkUSBExec(const SonyDevInfo& devcode);

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
    int fillPatchComplect(PatchId pid, SonyDevInfo dev, PatchComplect& patch, PatchId plpid = PID_UNUSED);

    //--------------------------------------------------------------------------
    //! @brief      is PCM speedup supportd
    //!
    //! @return     true if supported, false if not
    //--------------------------------------------------------------------------
    bool pcmSpeedupSupported();

    //--------------------------------------------------------------------------
    //! @brief      apply PCM speedup patch
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int applyPCMSpeedupPatch();

    CNetMdDev& mNetMd;

    /// device info
    SonyDevInfo mDevInfo;

    //! @brief patch areas used
    PatchId mUsedPatches[MAX_PATCH];
};

} // ~namespace
