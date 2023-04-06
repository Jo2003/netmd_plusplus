/*
 * CNetMdSecure.h
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
#include "CNetMdPatch.h"
#include <cstdint>

namespace netmd {

class CNetMdApi;

//------------------------------------------------------------------------------
//! @brief      This class describes a net md secure helper
//------------------------------------------------------------------------------
class CNetMdSecure
{
    friend CNetMdApi;

    using NetMdStatus = CNetMdDev::NetMdStatus;

    //! linked list to store a list of 16-byte keys
    struct Keychain
    {
        uint8_t mKey[16];
        Keychain *mpNext;
    };

    //! enabling key block
    struct Ekb
    {
        //! The ID of the EKB.
        uint32_t id;

        //! A chain of encrypted keys. The one end of the chain is the encrypted
        //! root key, the other end is a key encrypted by a key the device has in
        //! it's key set. The direction of the chain is not yet known.
        Keychain *chain;

        //! Selects which key from the devices keyset has to be used to start
        //! decrypting the chain. Each key in the key set corresponds to a specific
        //! depth in the tree of device IDs.
        uint32_t depth;

        //! Signature of the root key (24 byte). Used to verify integrity of the
        //! decrypted root key by the device.
        uint8_t signature[24];
    };

    //! Format of the song data packets, that are transfered over USB.
    enum WireFormat : uint8_t
    {
        NETMD_WIREFORMAT_PCM = 0,
        NETMD_WIREFORMAT_105KBPS = 0x90,
        NETMD_WIREFORMAT_LP2 = 0x94,
        NETMD_WIREFORMAT_LP4 = 0xa8
    };

    //! audio patch marker
    enum AudioPatch : uint8_t
    {
        NO_PATCH, //!< no patch needed
        WAVE,     //!< wave endianess patch
        SP        //!< atrac1 SP padding patch
    };

    static constexpr uint16_t NETMD_RIFF_FORMAT_TAG_ATRAC3 = 0x0270;
    static constexpr uint16_t NETMD_DATA_BLOCK_SIZE_LP2    = 384;
    static constexpr uint16_t NETMD_DATA_BLOCK_SIZE_LP4    = 192;
    static constexpr uint8_t  SP_PAD_SZ                    = 100;
    static constexpr uint8_t  MIN_WAV_LENGTH               = 152;


    //! linked list, storing all information of the single packets, send to the device
    //! while uploading a track
    struct TrackPackets
    {
        //! encrypted key for this packet (8 bytes)
        uint8_t key[8];

        //! IV for the encryption  (8 bytes)
        uint8_t iv[8];

        //! the packet data itself
        uint8_t *data;

        //! length of the data
        size_t length;

        //! next packet to transfer (linked list)
        TrackPackets *next;
    };

    //--------------------------------------------------------------------------
    //! @brief      Constructs a new instance.
    //!
    //! @param      netMd  The net md device reference
    //--------------------------------------------------------------------------
    CNetMdSecure(CNetMdDev& netMd)
        : mNetMd(netMd), mPatch(netMd)
    {}

    //--------------------------------------------------------------------------
    //! @brief      check status
    //!
    //! @param[in]  status    The current status
    //! @param[in]  expected  The expected status
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    static int checkStatus(uint8_t status, uint8_t expected);

    //--------------------------------------------------------------------------
    //! @brief      get payload position in response
    //!
    //! @param[in]  dataLen  The data length (optional)
    //!
    //! @return     offset
    //--------------------------------------------------------------------------
    static int payloadOffset(size_t dataLen = 0);

    //--------------------------------------------------------------------------
    //! @brief      get keychain length
    //!
    //! @param[in]  chain  The chain
    //!
    //! @return     length
    //--------------------------------------------------------------------------
    static int chainLength(Keychain* chain);

    //--------------------------------------------------------------------------
    //! @brief      Builds a send key data command.
    //!
    //! @param[in]  ekb    The enabling key block
    //! @param[out] query  The query
    //!
    //! @return     > 0 -> query size; else NetMdErr
    //--------------------------------------------------------------------------
    static int buildSendKeyDataCmd(const Ekb& ekb,
                                   NetMDResp& query);

    //--------------------------------------------------------------------------
    //! @brief      get frame size for wire format
    //!
    //! @param[in]  wf    wire format
    //!
    //! @return     frame size
    //--------------------------------------------------------------------------
    static uint16_t frameSize(WireFormat wf);

    //--------------------------------------------------------------------------
    //! @brief      prepare track packets
    //!
    //! @param[in]  data         The data
    //! @param[in]  dataLen      The data length
    //! @param[out] packets      The packets pointer
    //! @param[out] packetCount  The packet count
    //! @param[out] frames       The frames
    //! @param[in]  channels     The channels
    //! @param[out] packetLen    The packet length
    //! @param[in]  kek          The kek
    //! @param[in]  wf           wire format
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    static int preparePackets(uint8_t* data, size_t dataLen, TrackPackets** packets,
                              uint32_t& packetCount,  uint32_t& frames, uint8_t channels,
                              uint32_t& packetLen, uint8_t kek[8], WireFormat wf);

    //--------------------------------------------------------------------------
    //! @brief      free allocated memory
    //!
    //! @param      packets  The packets
    //--------------------------------------------------------------------------
    static void cleanupPackets(TrackPackets** packets);

    //--------------------------------------------------------------------------
    //! @brief      check if audio is supported
    //!
    //! @param[in]  fileContent  The audio file content
    //! @param[in]  fSize        The data size
    //! @param      wf           wire format
    //! @param      df           disc format
    //! @param      patch        The patch
    //! @param      channels     The channels
    //! @param      headerSz     The header size
    //!
    //! @return     false -> not supported; true -> supported
    //--------------------------------------------------------------------------
    static bool audioSupported(const uint8_t* fileContent, size_t fSize, WireFormat& wf,
                               DiskFormat& df, AudioPatch& patch, uint8_t& channels, uint32_t& headerSz);

    //-------------------------------------------------------------------------
    //! @brief      get retail MAC
    //!
    //! @param      rootkey     The rootkey
    //! @param      hostnonce   The hostnonce
    //! @param      devnonce    The devnonce
    //! @param      sessionkey  The sessionkey
    //-------------------------------------------------------------------------
    static void retailMAC(uint8_t rootkey[8], uint8_t hostnonce[8],
                          uint8_t devnonce[8], uint8_t sessionkey[8]);

    //--------------------------------------------------------------------------
    //! @brief      prepare SP audio for transfer
    //!
    //! @param      adata   The audio data
    //! @param      dataSz  The data size
    //!
    //! @return     { description_of_the_return_value }
    //--------------------------------------------------------------------------
    static int prepareSpAudio(uint8_t** adata, size_t& dataSz);

    //--------------------------------------------------------------------------
    //! @brief      get WAVE data position
    //!
    //! @param[in]  data    The data
    //! @param[in]  offset  The offset
    //! @param[in]  len     The length
    //!
    //! @return     position (offset)
    //--------------------------------------------------------------------------
    static size_t waveDataPosition(const uint8_t* data, size_t offset, size_t len);

    //--------------------------------------------------------------------------
    //! @brief      do a secure data exchange
    //!
    //! @param[in]  cmd       The command
    //! @param[in]  data      The data to send (optional)
    //! @param[in]  dataLen   The data length (optional)
    //! @param[out] response  The response (optional)
    //! @param[in]  expected  The expected (optional)
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int secureExchange(uint8_t cmd, unsigned char* data = nullptr,
                       size_t dataLen = 0, NetMDResp* response = nullptr,
                       NetMdStatus expected = NetMdStatus::NETMD_STATUS_ACCEPTED);

    //--------------------------------------------------------------------------
    //! @brief      receive secure message
    //!
    //! @param[in]  cmd       The command
    //! @param      response  The response
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int secureReceive(uint8_t cmd, NetMDResp* response);

    //--------------------------------------------------------------------------
    //! @brief      Enters secure session.
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int enterSession();

    //--------------------------------------------------------------------------
    //! @brief      Leaves secure session.
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int leaveSession();

    //--------------------------------------------------------------------------
    //! @brief      get leaf id
    //!
    //! @param      playerId  The player identifier
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int leafId(uint64_t& playerId);

    //--------------------------------------------------------------------------
    //! @brief      Sends the key data.
    //!
    //! @param[in]  ekb   The enabling key block
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sendKeyData(const Ekb& ekb);

    //--------------------------------------------------------------------------
    //! @brief      do session key exchange
    //!
    //! @param      randIn   The random in
    //! @param      randOut  The random out
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sessionKeyExchange(uint8_t randIn[8], uint8_t randOut[8]);

    //--------------------------------------------------------------------------
    //! @brief      forget the session key
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sessionKeyForget();

    //--------------------------------------------------------------------------
    //! @brief      setup download
    //!
    //! @param[in]  contentId   The content identifier
    //! @param      kek         The key encryption key
    //! @param      sessionKey  The session key
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int setupDownload(const uint8_t contentId[20], const uint8_t kek[8], const uint8_t sessionKey[8]);

    //--------------------------------------------------------------------------
    //! @brief      tranfer a list of song packets
    //!
    //! @param      packets     The packets
    //! @param[in]  fullLength  The full length
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int transferSongPackets(TrackPackets* packets, size_t fullLength);

    //--------------------------------------------------------------------------
    //! @brief      Sends a track.
    //!
    //! @param[in]  wf          wire format
    //! @param[in]  df          disk format
    //! @param[in]  frames      The frame count
    //! @param      packets     The packets
    //! @param[in]  packetLen   The packet length
    //! @param      sessionKey  The session key
    //! @param      track       The track number buffer
    //! @param      uuid        The uuid
    //! @param      contentId   The content identifier
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sendTrack(WireFormat wf, DiskFormat df, uint32_t frames,
                  TrackPackets* packets, uint32_t packetLen,
                  uint8_t sessionKey[8], uint16_t& track,
                  uint8_t uuid[8], uint8_t contentId[20]);

    //--------------------------------------------------------------------------
    //! @brief      Commits a track.
    //!
    //! @param[in]  track       The track
    //! @param      sessionKey  The session key
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int commitTrack(uint16_t track, uint8_t sessionKey[8]);

    //--------------------------------------------------------------------------
    //! @brief      Sets the track protection.
    //!
    //! @param[in]  val   The new value
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int setTrackProtection(uint8_t val);

    //--------------------------------------------------------------------------
    //! @brief      Sends an audio track
    //!
    //! @param[in]  filename  The filename
    //! @param[in]  otf       The disk format
    //! @param[out] trackNo   The track no
    //!
    //! @return     NetMdErr
    //! @see        NetMdErr
    //--------------------------------------------------------------------------
    int sendAudioTrack(const std::string& filename, DiskFormat otf, uint16_t& trackNo);

    //--------------------------------------------------------------------------
    //! @brief      is SP upload supported?
    //!
    //! @return     true if yes
    //--------------------------------------------------------------------------
    bool spUploadSupported();

    CNetMdDev& mNetMd;

    /// patch support class
    CNetMdPatch mPatch;

};

} // ~namespace
