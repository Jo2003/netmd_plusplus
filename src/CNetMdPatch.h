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
#include <optional>

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
        PID_PCM_TO_MONO, ///< PCM upload, convert to mono on device
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

    /// payload structure
    struct PayLoad
    {
        uint32_t mDevs;
        NetMDByteVector mPtData;
    };

    /// type defines
    using SonyDevInfo       = CNetMdDev::SonyDevInfo;
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

    /// storage structure for installed patches
    struct PatchStorage
    {
        PatchId mPid;               ///< patch id
        uint32_t mAddr;             ///< patch address
        NetMDByteVector mPatchData; ///< patch data
    };

    // const PayLoad PATCH_CLEAN_DATA = {0, {0, 0, 0, 0}};

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
    //! @brief      device was removed
    //--------------------------------------------------------------------------
    void deviceRemoved();

    //--------------------------------------------------------------------------
    //! @brief      get number of max patches
    //!
    //! @return     -1 -> error; else max number of patches
    //--------------------------------------------------------------------------
    int maxPatches() const;

    //--------------------------------------------------------------------------
    //! @brief      get free patch slots
    //
    //! @return     a vector with free patch slots
    //--------------------------------------------------------------------------
    std::vector<int> freePatchSlots();

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
    //! @brief      reverse search for patch id
    //!
    //! @param[in]  devinfo   The device info
    //! @param[in]  addr      The patch address
    //! @param[in]  patch_cnt The patch content
    //!
    //! @return     patch id or nullopt
    //--------------------------------------------------------------------------
    static std::optional<PatchId> reverserSearchPatchId(SonyDevInfo devinfo, uint32_t addr, 
                                                        const NetMDByteVector& patch_cnt); 

    //--------------------------------------------------------------------------
    //! @brief      check if patch is active
    //!
    //! @param[in]  pid      The patch id
    //!
    //! @return     1 -> patch is active; 0 -> not active
    //--------------------------------------------------------------------------
    int checkPatch(PatchId pid);

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
    //! @brief      unpatch up to all installed patches
    //!
    //! @param[in]  pids    patches to do unpatch for, if empty unpatch all.
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int unpatch(const std::vector<PatchId>& pids = {});

    //--------------------------------------------------------------------------
    //! @brief      unpatch a patch related to index
    //!
    //! @param[in]  idx   patch id of patch to undo
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int unpatchIdx(int idx);

    //--------------------------------------------------------------------------
    //! @brief      do safety patch of needed
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int safetyPatch();

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
    //! @brief      apply the PCM to mono patch
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int applyPCM2MonoPatch();

    //--------------------------------------------------------------------------
    //! @brief      undo the PCM to mono patch
    //--------------------------------------------------------------------------
    void undoPCM2MonoPatch();

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

    //--------------------------------------------------------------------------
    //! @brief      apply PCM speedup patch
    //--------------------------------------------------------------------------
    void undoPCMSpeedupPatch();

    //--------------------------------------------------------------------------
    //! @brief      update patch storage
    //--------------------------------------------------------------------------
    void updatePatchStorage();

    CNetMdDev& mNetMd;

    //! @brief patch areas used
    PatchStorage mPatchStorage[MAX_PATCH];

    /// is the patch store valid?
    bool mPatchStoreValid;
};

} // ~namespace
