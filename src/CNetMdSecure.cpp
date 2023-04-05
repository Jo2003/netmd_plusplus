/*
 * CNetMdSecure.cpp
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

#include <gcrypt.h>
#include <cmath>
#include "CNetMdSecure.h"
#include "CNetMdDev.hpp"
#include "netmd_defines.h"
#include "netmd_utils.h"
#include <cstdint>
#include <fstream>
#include <ios>

namespace netmd {

//--------------------------------------------------------------------------
//! @brief      check status
//!
//! @param[in]  status    The current status
//! @param[in]  expected  The expected status
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::checkStatus(uint8_t status, uint8_t expected)
{
    if (status == expected)
    {
        return NETMDERR_NO_ERROR;
    }

    switch (status)
    {
    case CNetMdDev::NETMD_STATUS_NOT_IMPLEMENTED:
        return NETMDERR_NOT_SUPPORTED;

    case CNetMdDev::NETMD_STATUS_REJECTED:
        return NETMDERR_CMD_FAILED;

    default:
        break;
    }

    return NETMDERR_OTHER;
}

//--------------------------------------------------------------------------
//! @brief      get payload position in response
//!
//! @param[in]  dataLen  The data length (optional)
//!
//! @return     offset
//--------------------------------------------------------------------------
int CNetMdSecure::payloadOffset(size_t dataLen)
{
    return 12 + dataLen;
}

//--------------------------------------------------------------------------
//! @brief      get keychain length
//!
//! @param[in]  chain  The chain
//!
//! @return     length
//--------------------------------------------------------------------------
int CNetMdSecure::chainLength(Keychain* chain)
{
    int len = 0;

    while (chain != nullptr)
    {
        len++;
        chain = chain->mpNext;
    }
    return len;
}

//--------------------------------------------------------------------------
//! @brief      Builds a send key data command.
//!
//! @param[in]  ekb    The enabling key block
//! @param[out] query  The query
//!
//! @return     > 0 -> query size; else NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::buildSendKeyDataCmd(const Ekb& ekb,
                                      NetMDResp& query)
{
    uint16_t chainLen   = chainLength(ekb.chain);
    uint16_t expQuerySz = 22 + (chainLen * 16) + 24 ;
    uint16_t dataBytes  = expQuerySz - 6;

    const char* format   = "%>w 00 00 %>w 00 00 %>w %>d %>d 00 00 00 00 %*";
    NetMDParams params   =
    {
        {dataBytes },
        {dataBytes },
        {chainLen  },
        {ekb.depth },
        {ekb.id    }
    };

    NetMDByteVector data;
    Keychain* chain = ekb.chain;

    while (chain != nullptr)
    {
        for(int i = 0; i < 16; i++)
        {
            data.push_back(chain->mKey[i]);
        }

        chain = chain->mpNext;
    }

    for (int i = 0; i < 24; i++)
    {
        data.push_back(ekb.signature[i]);
    }

    params.push_back(data);

    if (formatQuery(format, params, query) == static_cast<int>(expQuerySz))
    {
        return expQuerySz;
    }
    else
    {
        return NETMDERR_PARAM;
    }
}

//--------------------------------------------------------------------------
//! @brief      get frame size for wire format
//!
//! @param[in]  wf    wire format
//!
//! @return     frame size
//--------------------------------------------------------------------------
uint16_t CNetMdSecure::frameSize(WireFormat wf)
{
    uint16_t ret = 0;
    switch (wf)
    {
    case NETMD_WIREFORMAT_PCM:
        ret = 2048;
        break;
    case NETMD_WIREFORMAT_LP2:
        ret = 192;
        break;
    case NETMD_WIREFORMAT_105KBPS:
        ret = 152;
        break;
    case NETMD_WIREFORMAT_LP4:
        ret = 96;
        break;
    default:
        break;
    }
    return ret;
}

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
int CNetMdSecure::preparePackets(uint8_t* data, size_t dataLen, TrackPackets** packets,
                                 uint32_t* packetCount,  uint32_t* frames, uint8_t channels,
                                 uint32_t* packetLen, uint8_t kek[8], WireFormat wf)
{
    // Limit chunksize to multiple of 16384 bytes
    // (incl. 24 byte header data for first packet).
    // Large sizes cause instability in some players
    // especially with ATRAC3 files.

    size_t position = 0;
    size_t chunksize, packet_data_length, first_chunk = 0x00100000U;
    size_t frame_size = frameSize(wf);
    size_t frame_padding = 0;
    TrackPackets *last = nullptr;
    TrackPackets *next = nullptr;

    gcry_cipher_hd_t key_handle;
    gcry_cipher_hd_t data_handle;

    // We have no use for "security" (= DRM) so just use constant IV.
    // However, the key has to be randomized, because the device apparently checks
    // during track commit that the same key is not re-used during a single session.
    unsigned char iv[8]      = {0,};
    unsigned char raw_key[8] = {0,}; // data encryption key
    unsigned char key[8]     = {0,}; // data encryption key wrapped with session key

    if(channels == NETMD_CHANNELS_MONO)
    {
        frame_size /= 2;
    }

    gcry_cipher_open(&key_handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    gcry_cipher_open(&data_handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_CBC, 0);
    gcry_cipher_setkey(key_handle, kek, 8);

    // generate key, use same key for all packets
    gcry_randomize(raw_key, sizeof(raw_key), GCRY_STRONG_RANDOM);
    gcry_cipher_decrypt(key_handle, key, 8, raw_key, sizeof(raw_key));
    gcry_cipher_setkey(data_handle, raw_key, sizeof(raw_key));

    *packetCount = 0;
    while (position < dataLen)
    {
        // Decrease chunksize by 24 (length, iv and key) for 1st packet
        // to keep packet size constant.
        if ((*packetCount) > 0)
        {
            chunksize = first_chunk;
        }
        else
        {
            chunksize = first_chunk - 24U;
        }

        packet_data_length = chunksize;

        if ((dataLen - position) < chunksize)
        {
            // last packet
            packet_data_length = dataLen - position;

            // If input data is not an even multiple of the frame size, pad to frame size.
            // Since all frame sizes are divisible by 8, cipher padding is a non-issue.
            // Under rare circumstances the padding may lead to the last packet being slightly
            // larger than first_chunk; this should not matter.
            if((dataLen % frame_size) != 0)
            {
                frame_padding = frame_size - (dataLen % frame_size);
            }

            chunksize = packet_data_length + frame_padding;
            mLOG(DEBUG) << "last packet: packet_data_length=" << packet_data_length
                        << " + frame_padding=" << frame_padding << " = chunksize="
                        << chunksize;
        }

        // alloc memory
        next = new TrackPackets;
        next->length = chunksize;
        next->data = new uint8_t[next->length];
        memset(next->data, 0, next->length);
        next->next = nullptr;

        // link list
        if (last != nullptr)
        {
            last->next = next;
        }
        else
        {
            *packets = next;
        }

        // crypt data
        memcpy(next->iv, iv, 8);
        memcpy(next->key, key, 8);
        gcry_cipher_setiv(data_handle, iv, 8);

        if (chunksize > packet_data_length)
        {
            // If last frame is padded, copy plaintext to chunk buffer and encrypt in place.
            // This avoids calling gcry_cipher_encrypt() with outsize > insize, which leads
            // to noise at end of track.
            memcpy(next->data, data + position, packet_data_length);
            gcry_cipher_encrypt(data_handle, next->data, chunksize, NULL, 0);
        }
        else
        {
            gcry_cipher_encrypt(data_handle, next->data, chunksize, data + position, packet_data_length);
        }

        // use last encrypted block as iv for the next packet so we keep
        // on Cipher Block Chaining
        memcpy(iv, next->data + chunksize - 8, 8);

        // next packet
        position = position + chunksize;
        (*packetCount)++;
        last = next;
        mLOG(DEBUG) << "generating packet " << *packetCount << " : " << chunksize << " bytes";
    }

    gcry_cipher_close(key_handle);
    gcry_cipher_close(data_handle);

    *frames = position / frame_size;
    *packetLen = position;

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      free allocated memory
//!
//! @param      packets  The packets
//--------------------------------------------------------------------------
void CNetMdSecure::cleanupPackets(TrackPackets** packets)
{
    TrackPackets *current = *packets;
    TrackPackets *last;

    while (current != nullptr)
    {
        last = current;
        current = last->next;

        delete [] last->data;
        delete last;
        last = nullptr;
    }
}

//--------------------------------------------------------------------------
//! @brief      check if audio is supported
//!
//! @param[in]  fileContent The audio file content
//! @param[in]  fSize        The data size
//! @param      wf          wire format
//! @param      df          disc format
//! @param      patch       The patch
//! @param      channels    The channels
//! @param      headerSz    The header size
//!
//! @return     false -> not supported; true -> supported
//--------------------------------------------------------------------------
bool CNetMdSecure::audioSupported(const uint8_t* fileContent, size_t fSize, WireFormat& wf,
                                  DiskFormat& df, AudioPatch& patch, uint8_t& channels,
                                  uint32_t& headerSz)
{
    if ((strncmp("RIFF", reinterpret_cast<const char*>(fileContent), 4) != 0)
        || (strncmp("WAVE", reinterpret_cast<const char*>(fileContent + 8), 4) != 0)
        || (strncmp("fmt ", reinterpret_cast<const char*>(fileContent + 12), 4) != 0))
    {
        // no wave format, look for preencoded ATRAC1 (SP).
        // I know the test is vague!
        if ((fileContent[1] == 8) && (fSize > 2048))
        {
            channels   = (fileContent[264] == 2) ? NETMD_CHANNELS_STEREO  : NETMD_CHANNELS_MONO;
            df = NETMD_DISKFORMAT_LP2;
            wf = NETMD_WIREFORMAT_105KBPS;
            headerSz = 2048;
            patch = SP;
            return true;
        }
        else
        {
            // no valid WAV file or fmt chunk missing
            return false;
        }
    }

    // PCM
    if (fromLittleEndianArray<uint16_t>(fileContent + 20) == 1)
    {
        // needs conversion (byte swapping) for pcm raw data from wav file
        patch = WAVE;
        wf    = NETMD_WIREFORMAT_PCM;
        if (fromLittleEndianArray<uint32_t>(fileContent + 24) != 44100)
        {
            // sample rate not 44k1
            return false;
        }

        if (fromLittleEndianArray<uint16_t>(fileContent + 34) != 16)
        {
            // bitrate not 16bit
            return false;
        }

        if (fromLittleEndianArray<uint16_t>(fileContent + 22) == 2)
        {
            // channels = 2, stereo
            channels = NETMD_CHANNELS_STEREO;
            df       = NETMD_DISKFORMAT_SP_STEREO;
        }
        else if (fromLittleEndianArray<uint16_t>(fileContent + 22) == 1)
        {
            // channels = 1, mono
            channels = NETMD_CHANNELS_MONO;
            df       = NETMD_DISKFORMAT_SP_MONO;
        }
        else
        {
            return false;
        }

        headerSz = 20 + fromLittleEndianArray<uint32_t>(fileContent + 16);

        return true;
    }

    // ATRAC3
    if (fromLittleEndianArray<uint16_t>(fileContent + 20) == NETMD_RIFF_FORMAT_TAG_ATRAC3)
    {
        // conversion not needed
        patch = NO_PATCH;
        if (fromLittleEndianArray<uint32_t>(fileContent + 24) != 44100)
        {
            // sample rate not 44k1
            return false;
        }

        if(fromLittleEndianArray<uint16_t>(fileContent + 32) == NETMD_DATA_BLOCK_SIZE_LP2)
        {
            // data block size LP2
            wf = NETMD_WIREFORMAT_LP2;
            df = NETMD_DISKFORMAT_LP2;
        }
        else if(fromLittleEndianArray<uint16_t>(fileContent + 32) == NETMD_DATA_BLOCK_SIZE_LP4)
        {
            // data block size LP4
            wf = NETMD_WIREFORMAT_LP4;
            df = NETMD_DISKFORMAT_LP4;
        }
        else
        {
            return false;
        }

        headerSz = 20 + fromLittleEndianArray<uint32_t>(fileContent + 16);
        channels = NETMD_CHANNELS_STEREO;
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------
//! @brief      get retail MAC
//!
//! @param      rootkey     The rootkey
//! @param      hostnonce   The hostnonce
//! @param      devnonce    The devnonce
//! @param      sessionkey  The sessionkey
//-------------------------------------------------------------------------
void CNetMdSecure::retailMAC(uint8_t rootkey[8], uint8_t hostnonce[8],
                             uint8_t devnonce[8], uint8_t sessionkey[8])
{
    gcry_cipher_hd_t handle1;
    gcry_cipher_hd_t handle2;

    uint8_t des3_key[24] = {0,};
    uint8_t iv[8]        = {0,};

    gcry_cipher_open(&handle1, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    gcry_cipher_setkey(handle1, rootkey, 8);
    gcry_cipher_encrypt(handle1, iv, 8, hostnonce, 8);

    memcpy(des3_key, rootkey, 16);
    memcpy(des3_key+16, rootkey, 8);
    gcry_cipher_open(&handle2, GCRY_CIPHER_3DES, GCRY_CIPHER_MODE_CBC, 0);
    gcry_cipher_setkey(handle2, des3_key, 24);
    gcry_cipher_setiv(handle2, iv, 8);
    gcry_cipher_encrypt(handle2, sessionkey, 8, devnonce, 8);

    gcry_cipher_close(handle1);
    gcry_cipher_close(handle2);
}

//--------------------------------------------------------------------------
//! @brief      prepare SP audio for transfer
//!
//! @param      adata   The audio data
//! @param      dataSz  The data size
//!
//! @return     { description_of_the_return_value }
//--------------------------------------------------------------------------
int CNetMdSecure::prepareSpAudio(uint8_t** adata, size_t& dataSz)
{
    const uint8_t padding[SP_PAD_SZ] = {0,};
    size_t   in_sz   = dataSz;
    uint8_t* in_data = *adata;
    size_t   out_idx = 0, in_idx = 0;
    uint8_t* out_data;
    uint8_t* sector;
    size_t sector_sz;

    // mind the header of 2048 bytes
    in_sz   -= 2048;
    in_data += 2048;

    // get final memory size incl. padding
    double temp_sz = in_sz;
    size_t new_sz = std::ceil(temp_sz / 2332.0) * 100 + temp_sz;
    if((out_data = new uint8_t[new_sz]) != nullptr)
    {
        for (in_idx = 0; in_idx < in_sz; in_idx += 2332)
        {
            // sector size might be less than 2332 bytes
            sector_sz = ((in_sz - in_idx) >= 2332) ? 2332 : in_sz - in_idx;
            sector = &out_data[out_idx];
            memcpy(sector, &in_data[in_idx], sector_sz);
            out_idx += sector_sz;

            // Rewrite Block Size Mode and the number of Block Floating Units
            // This mitigates an issue with atracdenc where it doesn't write
            // the bytes at the end of each frame.
            for(size_t j = 0; j < sector_sz; j += 212)
            {
                sector[j + 212 - 1] = sector[j + 0];
                sector[j + 212 - 2] = sector[j + 1];
            }

            memcpy(&out_data[out_idx], padding, SP_PAD_SZ);
            out_idx += SP_PAD_SZ;
        }

        delete [] *adata;
        dataSz = out_idx;
        *adata = out_data;

        return NETMDERR_NO_ERROR;
    }

    return NETMDERR_OTHER;
}

//--------------------------------------------------------------------------
//! @brief      get WAVE data position
//!
//! @param[in]  data    The data
//! @param[in]  offset  The offset
//! @param[in]  len     The length
//!
//! @return     position (offset)
//--------------------------------------------------------------------------
size_t CNetMdSecure::waveDataPosition(const uint8_t* data, size_t offset, size_t len)
{
    size_t i = offset, pos = 0;

    while (i < len - 4)
    {
        if(strncmp("data", reinterpret_cast<const char*>(data + i), 4) == 0)
        {
            pos = i;
            break;
        }
        i += 2;
    }

    return pos;
}

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
int CNetMdSecure::secureExchange(uint8_t cmd, unsigned char* data,
                                 size_t dataLen, NetMDResp* response,
                                 NetMdStatus expected)
{
    int ret;
    NetMDByteVector vdata;
    NetMDResp       query, locResp;
    NetMDResp      *pResp = (response != nullptr) ? response : &locResp;

    const char*  format;
    NetMDParams  params = {{cmd}};

    // Do we have data to send?
    if ((dataLen > 0) && (data != nullptr))
    {
        format = "00 1800 08 00 46 f0 03 01 03 %b ff %*";

        for (size_t i = 0; i < dataLen; i++)
        {
            vdata.push_back(data[i]);
        }

        params.push_back(vdata);
    }
    else
    {
        format = "00 1800 08 00 46 f0 03 01 03 %b ff";
    }

    if (((ret = formatQuery(format, params, query)) > 0) && (query != nullptr))
    {
        if((ret = mNetMd.exchange(query.get(), ret, pResp, false, expected)) >= 11)
        {
            // check secure header and command
            if ((memcmp(&(*pResp)[1], &query[1], 9) != 0) || ((*pResp)[10] != cmd))
            {
                ret = NETMDERR_OTHER;
            }
        }
        else
        {
            ret = NETMDERR_OTHER;
        }
    }
    else
    {
        ret = NETMDERR_PARAM;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      receive secure message
//!
//! @param[in]  cmd       The command
//! @param      response  The response
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::secureReceive(uint8_t cmd, NetMDResp* response)
{
    int ret;
    NetMDResp tmpResp;
    NetMDResp& resp = (response != nullptr) ? *response : tmpResp;
    uint8_t secHead[] = {0x18, 0x00, 0x08, 0x00, 0x46, 0xf0, 0x03, 0x01, 0x03};

    if ((ret = mNetMd.getResponse(resp)) < 1)
    {
        ret = NETMDERR_USB;
    }
    else if ((ret >= 11) && (resp != nullptr))
    {
        if ((memcmp(secHead, &resp[1], sizeof(secHead)) != 0) || (resp[10] != cmd))
        {
            ret = NETMDERR_OTHER;
        }
    }
    else
    {
        ret = NETMDERR_OTHER;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Enters secure session.
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::enterSession()
{
    return secureExchange(0x80);
}

//--------------------------------------------------------------------------
//! @brief      Leaves secure session.
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::leaveSession()
{
    return secureExchange(0x81);
}

//--------------------------------------------------------------------------
//! @brief      get leaf id
//!
//! @param      playerId  The player identifier
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::leafId(uint64_t& playerId)
{
    NetMDResp resp;
    int ret    = secureExchange(0x11, nullptr, 0, &resp);
    int offset = payloadOffset();

    if (ret >= static_cast<int>(offset + sizeof(uint64_t)))
    {
        playerId = fromBigEndian(*reinterpret_cast<uint64_t*>(&resp[offset]));
        ret = NETMDERR_NO_ERROR;
    }
    else
    {
        ret = NETMDERR_CMD_FAILED;
    }

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Sends the key data.
//!
//! @param[in]  ekb   The enabling key block
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::sendKeyData(const Ekb& ekb)
{
    NetMDResp query, response;
    int size   = buildSendKeyDataCmd(ekb, query);
    int respSz = 0;


    if (size > 0)
    {
        int offset   = payloadOffset();
        if (((respSz = secureExchange(0x12, query.get(), size, &response)) > 0) && (response != nullptr))
        {
            if (respSz >= (offset + 6))
            {
                if ((fromBigEndianArray<uint16_t>(&response[offset]) == (size - 6))
                    && (fromBigEndianArray<uint16_t>(&response[offset + 4]) == (size - 6)))
                {
                    return NETMDERR_NO_ERROR;
                }
            }
        }
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      do session key exchange
//!
//! @param      randIn   The random in
//! @param      randOut  The random out
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::sessionKeyExchange(uint8_t randIn[8], uint8_t randOut[8])
{
    int ret;

    NetMDResp resp;

    uint8_t cmd[11] = {0x00, 0x00, 0x00};

    for (int i = 0; i < 8; i++)
    {
        cmd[i + 3] = randIn[i];
    }

    if (((ret = secureExchange(0x20, cmd, 11, &resp)) > 0) && (resp != nullptr))
    {
        int offset = payloadOffset();
        if (ret >= (offset + 11))
        {
            if ((resp[offset] == 0x00) && (resp[offset + 1] == 0x00) && (resp[offset + 2] == 0x00))
            {
                offset += 3;
                for (int i = 0; i < 8; i++)
                {
                    randOut[i] = resp[offset + i];
                }
                return NETMDERR_NO_ERROR;
            }
        }
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      forget the session key
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::sessionKeyForget()
{
    int ret;

    NetMDResp resp;

    uint8_t cmd[] = {0x00, 0x00, 0x00};

    if (((ret = secureExchange(0x20, cmd, 3, &resp)) > 0) && (resp != nullptr))
    {
        int offset = payloadOffset();
        if (ret >= (offset + 3))
        {
            if ((resp[offset] == 0x00) && (resp[offset + 1] == 0x00) && (resp[offset + 2] == 0x00))
            {
                return NETMDERR_NO_ERROR;
            }
        }
    }

    return NETMDERR_CMD_FAILED;
}

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
int CNetMdSecure::setupDownload(const uint8_t contentId[20], const uint8_t kek[8], const uint8_t sessionKey[8])
{
    int ret;

    unsigned char cmdhdr[] = { 0x00, 0x00 };
    unsigned char data[32] = { 0x01, 0x01, 0x01, 0x01 /* ... */};
    unsigned char cmd[sizeof(cmdhdr) + sizeof(data)];

    unsigned char iv[8] = { 0 };
    gcry_cipher_hd_t handle;

    NetMDResp response;

    memcpy(data + 4, contentId, 20);
    memcpy(data + 24, kek, 8);

    gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_CBC, 0);
    gcry_cipher_setkey(handle, sessionKey, 8);
    gcry_cipher_setiv(handle, iv, 8);
    gcry_cipher_encrypt(handle, data, sizeof(data), nullptr, 0);
    gcry_cipher_close(handle);

    memcpy(cmd, cmdhdr, sizeof(cmdhdr));
    memcpy(cmd + sizeof(cmdhdr), data, 32);

    if (((ret = secureExchange(0x22, cmd, sizeof(cmd), &response)) > 0) && (response != nullptr))
    {
        int offset = payloadOffset();
        if (ret >= (offset + 2))
        {
            if ((response[offset] == 0x00) && (response[offset + 1] == 0x00))
            {
                return NETMDERR_NO_ERROR;
            }
        }
    }

    return NETMDERR_CMD_FAILED;
}

//--------------------------------------------------------------------------
//! @brief      tranfer a list of song packets
//!
//! @param      packets     The packets
//! @param[in]  fullLength  The full length
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::transferSongPackets(TrackPackets* packets, size_t fullLength)
{
    int ret;
    TrackPackets *p;
    int packet_size;
    size_t total_transferred = 0, display_length = fullLength + 24;
    int transferred = 0;
    int first_packet = 1;
    time_t start_time = time(NULL), duration;

    p = packets;
    while (p != NULL)
    {
        // length + key + iv + data
        if(first_packet)
        {
            // length, key and iv in first packet only
            NetMDResp query;
            NetMDByteVector data;

            packet_size = 8 + 8 + 8 + p->length;

            NetMDParams params = {{mQWORD(fullLength)}};
            addArrayData(data, p->key, 8);
            addArrayData(data, p->iv, 8);
            addArrayData(data, p->data, p->length);
            params.push_back(data);
            if (((ret = formatQuery("%>q %*", params, query)) > 0) && (query != nullptr))
            {
                if ((transferred = mNetMd.bulkTransfer(query.get(), ret, 80'000)) == packet_size)
                {
                    total_transferred += static_cast<size_t>(transferred);
                    ret = NETMDERR_NO_ERROR;
                }
                else
                {
                    ret = NETMDERR_USB;
                }
            }
            else
            {
                ret = NETMDERR_PARAM;
            }

            first_packet = 0;
        }
        else
        {
            packet_size = p->length;
            if ((transferred = mNetMd.bulkTransfer(p->data, p->length, 80'000)) == packet_size)
            {
                total_transferred += static_cast<size_t>(transferred);
                ret = NETMDERR_NO_ERROR;
            }
            else
            {
                ret = NETMDERR_USB;
            }
        }

        if (ret == NETMDERR_NO_ERROR)
        {
            mLOG(DEBUG) << total_transferred << " of " << display_length << " bytes ("
                        << (total_transferred * 100 / display_length) << "%) transferred ("
                        << transferred << " of " << packet_size << " bytes in packet)";

            p = p->next;
        }
        else
        {
            break;
        }
    }

    // report statistics on successful transfer
    duration = time(NULL) - start_time;
    if ((ret == NETMDERR_NO_ERROR) && (duration > 0))
    {
        mLOG(DEBUG) << "transfer took " << duration << " seconds ("
                    << (display_length / static_cast<size_t>(duration / 1024))
                    << " kB/sec)";
    }

    return ret;
}

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
int CNetMdSecure::sendTrack(WireFormat wf, DiskFormat df, uint32_t frames,
                            TrackPackets* packets, uint32_t packetLen,
                            uint8_t sessionKey[8], uint16_t* track,
                            uint8_t uuid[8], uint8_t contentId[20])
{
    try
    {
        int ret;
        const char* format = "00 01 00 10 01 FF FF 00 %b %b %>d %>d";
        NetMDResp   query, resp;
        uint32_t    totalbytes = frameSize(wf) * frames + 24;

        mLOG(DEBUG) << "total transfer size : " << totalbytes << " bytes, "
                    << frames << " frames of " << frameSize(wf) << " bytes.";

        if (((ret = formatQuery(format, {{wf}, {df}, {frames}, {totalbytes}}, query)) < 0) || (query == nullptr))
        {
            mNetMdThrow(NETMDERR_PARAM, "Error while formatting query!");
        }

        if (((ret = secureExchange(0x28, query.get(), ret, &resp, NetMdStatus::NETMD_STATUS_INTERIM)) < 1)
            || (resp == nullptr))
        {
            mNetMdThrow(NETMDERR_USB, "Error while secureExchange()!");
        }

        int offset = payloadOffset();
        if (ret < (offset + 8))
        {
            mNetMdThrow(NETMDERR_USB, "Response to short. Have: " << ret << " bytes, expecting: " << (offset + 8) << " bytes!");
        }

        if ((memcmp(query.get(), &resp[offset], 5) != 0) || (resp[offset + 7] != 0x00))
        {
            mNetMdThrow(NETMDERR_USB, "Response doesn't include expected data!");
        }

        if (transferSongPackets(packets, packetLen) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error transferring track packets!");
        }

        // free last response
        resp = nullptr;

        if (((ret = secureReceive(0x28, &resp)) < 1) || (resp == nullptr))
        {
            mNetMdThrow(NETMDERR_USB, "Error while secureReceive()!");
        }

        NetMDParams capParams;
        const char* capture = "00 01 00 10 01 %>w 00 %?%?%?%?%?%?%?%?%?%? %*";

        if ((ret = scanQuery(resp.get() + offset, ret - offset, capture, capParams)) != NETMDERR_NO_ERROR)
        {
            mNetMdThrow(NETMDERR_USB, "Error while scanQuery()!");
        }

        if (capParams.size() != 2)
        {
            mNetMdThrow(NETMDERR_USB, "Can't capture all needed information!");
        }

        if (capParams.at(0).index() != UINT16_T)
        {
            mNetMdThrow(NETMDERR_USB, "1st captured data isn't uint16_t!");
        }

        *track = std::get<uint16_t>(capParams.at(0));

        if (capParams.at(1).index() != BYTE_VECTOR)
        {
            mNetMdThrow(NETMDERR_USB, "2nd captured data isn't NetMDByteVector!");
        }

        uint8_t encryptedreply[32] = {0,};
        NetMDByteVector ba = std::get<NetMDByteVector>(capParams.at(1));
        if (ba.size() < sizeof(encryptedreply))
        {
            mNetMdThrow(NETMDERR_USB, "2nd captured data isn't long enough. Have: "
                << ba.size() << " bytes, need: " << sizeof(encryptedreply) << " bytes.");
        }

        for (size_t i = 0; i < sizeof(encryptedreply); i++)
        {
            encryptedreply[i] = ba.at(i);
        }

        gcry_cipher_hd_t handle;
        uint8_t iv[8] = {0,};
        gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_CBC, 0);
        gcry_cipher_setiv(handle, iv, 8);
        gcry_cipher_setkey(handle, sessionKey, 8);
        gcry_cipher_decrypt(handle, encryptedreply, sizeof(encryptedreply), nullptr, 0);
        gcry_cipher_close(handle);

        memcpy(uuid, encryptedreply, 8);
        memcpy(contentId, encryptedreply + 12, 20);
    }
    catch(const ThrownData& e)
    {
        mLOG(CRITICAL) << e.mErrDescr;
        return e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while sending track!";
        return NETMDERR_OTHER;
    }

    return NETMDERR_NO_ERROR;
}

//--------------------------------------------------------------------------
//! @brief      Commits a track.
//!
//! @param[in]  track       The track
//! @param      sessionKey  The session key
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::commitTrack(uint16_t track, uint8_t sessionKey[8])
{
    int ret;
    const char* format = "00 10 01 %>w %*";

    gcry_cipher_hd_t handle;
    uint8_t hashIn[8]  = {0,};
    uint8_t hashOut[8] = {0,};

    gcry_cipher_open(&handle, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    gcry_cipher_setkey(handle, sessionKey, 8);
    gcry_cipher_encrypt(handle, hashOut, sizeof(hashOut), hashIn, sizeof(hashIn));
    gcry_cipher_close(handle);

    NetMDByteVector ba;
    addArrayData(ba, hashOut, sizeof(hashOut));

    NetMDResp query, resp;

    try
    {
        if (((ret = formatQuery(format, {{track}, {ba}}, query)) != 13) || (query == nullptr))
        {
            mNetMdThrow(NETMDERR_PARAM, "Error while formatting query!");
        }

        // Make sure that the device is well and truly finished with
        // what it was doing. Fixes USB interface crashes on at least
        // MZ-N420D.
        mNetMd.waitForSync();

        if ((ret = secureExchange(0x48, query.get(), ret, &resp)) < 1)
        {
            mNetMdThrow(NETMDERR_USB, "Error while secureExchange()!");
        }

        int offset = payloadOffset();
        if (ret < (offset + 5))
        {
            mNetMdThrow(NETMDERR_USB, "Response to short. Have: " << ret << " bytes, expecting: " << (offset + 5) << " bytes!");
        }

        if ((memcmp(&resp[offset], query.get(), 3) != 0)
            || (fromBigEndianArray<uint16_t>(resp.get() + offset + 3) != track))
        {
            mNetMdThrow(NETMDERR_USB, "Error sanity check for response data!");
        }

        ret = NETMDERR_NO_ERROR;
    }
    catch(const ThrownData& e)
    {
        mLOG(CRITICAL) << e.mErrDescr;
        ret = e.mErr;
    }
    catch(...)
    {
        mLOG(CRITICAL) << "Unknown error while committing track!";
        ret = NETMDERR_OTHER;
    }

    mNetMd.waitForSync();

    return ret;
}

//--------------------------------------------------------------------------
//! @brief      Sets the track protection.
//!
//! @param[in]  val   The new value
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::setTrackProtection(uint8_t val)
{
    int ret;
    uint8_t cmd[] = {0x00, 0x01, 0x00, 0x00, 0x00};
    cmd[sizeof(cmd) - 1] = val;

    NetMDResp resp;

    if (((ret = secureExchange(0x2b, cmd, sizeof(cmd), &resp)) > 0) && (resp != nullptr))
    {
        int offset = payloadOffset();

        if ((ret >= (offset + 4)) && (memcmp(cmd, resp.get() + offset, 4) == 0))
        {
            return NETMDERR_NO_ERROR;
        }
    }

    return NETMDERR_USB;
}

//--------------------------------------------------------------------------
//! @brief      Sends an audio track
//!
//! @param[in]  filename  The filename
//! @param[in]  title     The title
//! @param[in]  otf       The disk format
//!
//! @return     NetMdErr
//! @see        NetMdErr
//--------------------------------------------------------------------------
int CNetMdSecure::sendAudioTrack(const std::string& filename, const std::string& title, DiskFormat otf)
{
    int ret;
    Ekb ekb;
    uint8_t chain[] =
    {
        0x25, 0x45, 0x06, 0x4d, 0xea, 0xca,
        0x14, 0xf9, 0x96, 0xbd, 0xc8, 0xa4,
        0x06, 0xc2, 0x2b, 0x81, 0x49, 0xba,
        0xf0, 0xdf, 0x26, 0x9d, 0xb7, 0x1d,
        0x49, 0xba, 0xf0, 0xdf, 0x26, 0x9d,
        0xb7, 0x1d
    };

    uint8_t signature[] =
    {
        0xe8, 0xef, 0x73, 0x45, 0x8d, 0x5b,
        0x8b, 0xf8, 0xe8, 0xef, 0x73, 0x45,
        0x8d, 0x5b, 0x8b, 0xf8, 0x38, 0x5b,
        0x49, 0x36, 0x7b, 0x42, 0x0c, 0x58
    };

    uint8_t rootkey[] =
    {
        0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
        0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
        0x13, 0x37, 0x13, 0x37
    };

    Keychain *keychain;
    Keychain *next;
    size_t done;
    uint8_t hostnonce[8] = {0,};
    uint8_t devnonce[8] = {0,};
    uint8_t sessionkey[8] = {0,};

    uint8_t kek[] =
    {
        0x14, 0xe3, 0x83, 0x4e, 0xe2, 0xd3,
        0xcc, 0xa5
    };

    uint8_t contentid[] =
    {
        0x01, 0x0F, 0x50, 0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x48, 0xA2, 0x8D,
        0x3E, 0x1A, 0x3B, 0x0C, 0x44, 0xAF,
        0x2f, 0xa0
    };

    TrackPackets *packets = NULL;
    size_t packet_count = 0;
    size_t packet_length = 0;
    uint8_t *data = nullptr;
    size_t data_size = 0;

    uint16_t track;
    uint8_t uuid[8] = {0,};
    uint8_t new_contentid[20] = {0,};
    char ctitle[256] = {0,};

    uint32_t headersize;
    uint8_t channels;
    uint32_t frames, override_frames = 0;
    size_t data_position, audio_data_position, audio_data_size, i;
    AudioPatch audio_patch = NO_PATCH;
    uint8_t* audio_data;
    WireFormat wf;
    DiskFormat df;

    std::ifstream audioFile(filename, std::ios_base::in | std::ios_base::binary);

    if (audioFile)
    {
        // get pointer to associated buffer object
        std::filebuf* pbuf = audioFile.rdbuf();

        // get file size using buffer's members
        data_size = pbuf->pubseekoff (0, audioFile.end, audioFile.in);
        pbuf->pubseekpos(0, audioFile.in);

        // allocate memory to contain file data plus some buffer for e.g. padding
        data = new uint8_t[data_size + 2048];

        if (data == nullptr)
        {
            mLOG(CRITICAL) << "error allocating memory for file input";
            return NETMDERR_OTHER;
        }

        // get file data
        pbuf->sgetn(reinterpret_cast<char*>(data), data_size);

        audioFile.close();
    }
    else
    {
        mLOG(CRITICAL) << "cannot open audio file";
        return NETMDERR_OTHER;
    }

    // read source
    if (data_size  < MIN_WAV_LENGTH)
    {
        delete [] data;
        mLOG(CRITICAL) << "audio file too small (corrupt or not supported)";
        return NETMDERR_NOT_SUPPORTED;
    }

    mLOG(DEBUG) << "audio file size : " << data_size << " bytes.";

    // check contents
    if (!audioSupported(data, data_size, wf, df, audio_patch, channels, headersize))
    {
        mLOG(CRITICAL) << "audio format unknown or not supported";
        delete [] data;
        return NETMDERR_NOT_SUPPORTED;
    }
    else
    {
        mLOG(DEBUG) << "supported audio file detected";

        if (audio_patch == SP)
        {
            if (!mPatch.supportsSpUpload())
            {
                mLOG(CRITICAL) << "device doesn't support SP upload!";
                delete [] data;
                return NETMDERR_NOT_SUPPORTED;
            }

            override_frames = (data_size - 2048) / 212;
            if (prepareSpAudio(&data, data_size) != NETMDERR_NO_ERROR)
            {
                mLOG(CRITICAL) << "cannot prepare ATRAC1 audio data for SP transfer!";
                delete [] data;
                return NETMDERR_NOT_SUPPORTED;
            }
            else
            {
                // data returned by prepare function has no header
                audio_data = data;
                audio_data_size = data_size;
                mLOG(DEBUG) << "prepared audio data size: " << audio_data_size << " bytes";
            }
        }
        else if ((data_position = waveDataPosition(data, headersize, data_size)) == 0)
        {
            mLOG(CRITICAL) << "cannot locate audio data in file!";
            delete [] data;
            return NETMDERR_NOT_SUPPORTED;
        }
        else
        {
            mLOG(DEBUG) << "data chunk position at " << data_position;
            audio_data_position = data_position + 8;
            audio_data = data + audio_data_position;
            audio_data_size = fromLittleEndianArray<uint32_t>(data + data_position + 4);
            mLOG(DEBUG) << "audio data size read from file :           " << audio_data_size << " bytes";
            mLOG(DEBUG) << "audio data size calculated from file size: " << (data_size - audio_data_position) << " bytes";
        }
    }

    // acquire device - needed by Sharp devices, may fail on Sony devices
    static_cast<void>(mNetMd.aquireDev());

    if (audio_patch == SP)
    {
        if (mPatch.applySpPatch((channels == NETMD_CHANNELS_STEREO) ? 2 : 1) != NETMDERR_NO_ERROR)
        {
            mLOG(CRITICAL) << "Can't patch NetMD device for SP transfer!";
            delete [] data;
            mPatch.undoSpPatch();
            static_cast<void>(mNetMd.releaseDev());
            return NETMDERR_NOT_SUPPORTED;
        }
    }

    if (leaveSession() != NETMDERR_NO_ERROR)
    {
        mLOG(DEBUG) << "leaveSession() failed.";
    }

    if (setTrackProtection(0x01) != NETMDERR_NO_ERROR)
    {
        mLOG(DEBUG) << "setTrackProtection() failed.";
    }

    if (enterSession() != NETMDERR_NO_ERROR)
    {
        mLOG(DEBUG) << "enterSession() failed.";
    }

    // build ekb
    ekb.id = 0x26422642;
    ekb.depth = 9;
    memcpy(ekb.signature, signature, sizeof(signature));

    // build ekb key chain
    ekb.chain = nullptr;
    for (done = 0; done < sizeof(chain); done += 16U)
    {
        next = new Keychain;

        if (ekb.chain == nullptr)
        {
            ekb.chain = next;
        }
        else
        {
            keychain->mpNext = next;
        }

        next->mpNext = nullptr;
        memcpy(next->mKey, chain + done, 16);

        keychain = next;
    }

    error = netmd_secure_send_key_data(devh, &ekb);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_send_key_data : %s\n", netmd_strerror(error));

    /* cleanup */
    free(ekb.signature);
    keychain = ekb.chain;
    while (keychain != NULL) {
        next = keychain->next;
        free(keychain->key);
        free(keychain);
        keychain = next;
    }

    /* exchange nonces */
    gcry_create_nonce(hostnonce, sizeof(hostnonce));
    error = netmd_secure_session_key_exchange(devh, hostnonce, devnonce);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_session_key_exchange : %s\n", netmd_strerror(error));

    /* calculate session key */
    retailmac(rootkey, hostnonce, devnonce, sessionkey);

    error = netmd_secure_setup_download(devh, contentid, kek, sessionkey);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_setup_download : %s\n", netmd_strerror(error));

    /* conversion (byte swapping) for pcm raw data from wav file if needed */
    if (audio_patch == apt_wave)
    {
        for (i = 0; i < audio_data_size; i += 2)
        {
            unsigned char first = audio_data[i];
            audio_data[i] = audio_data[i + 1];
            audio_data[i + 1] = first;
        }
    }

    /* number of frames will be calculated by netmd_prepare_packets() depending on the wire format and channels */
    error = netmd_prepare_packets(audio_data, audio_data_size, &packets, &packet_count, &frames, channels, &packet_length, kek, wireformat);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_prepare_packets : %s\n", netmd_strerror(error));

    if ((discformat == NETMD_DISKFORMAT_SP_STEREO) && (otf != NO_ONTHEFLY_CONVERSION))
    {
        discformat = otf;
    }

    if(override_frames)
        frames = override_frames;

    /* send to device */
    error = netmd_secure_send_track(devh, wireformat,
        discformat,
        frames, packets,
        packet_length, sessionkey,
        &track, uuid, new_contentid);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_send_track : %s\n", netmd_strerror(error));

    /* cleanup */
    netmd_cleanup_packets(&packets);
    free(data);
    audio_data = NULL;

    if (error == NETMD_NO_ERROR) {
        char *titlep = title;

        /* set title, use either user-specified title or filename */
        if (in_title != NULL)
            strncpy(title, in_title, sizeof(title) - 1);
        else {
            strncpy(title, filename, sizeof(title) - 1);

            /* eliminate file extension */
            char *ext_dot = strrchr(title, '.');
            if (ext_dot != NULL)
                *ext_dot = '\0';

            /* eliminate path */
            char *title_slash = strrchr(title, '/');
            if (title_slash != NULL)
                titlep = title_slash + 1;
        }

        netmd_log(NETMD_LOG_VERBOSE, "New Track: %d\n", track);
        netmd_cache_toc(devh);
        netmd_set_title(devh, track, titlep);
        netmd_sync_toc(devh);

        /* commit track */
        error = netmd_secure_commit_track(devh, track, sessionkey);
        if (error == NETMD_NO_ERROR)
            netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_commit_track : %s\n", netmd_strerror(error));
        else
            netmd_log(NETMD_LOG_ERROR, "netmd_secure_commit_track failed : %s\n", netmd_strerror(error));
    }
    else {
        netmd_log(NETMD_LOG_ERROR, "netmd_secure_send_track failed : %s\n", netmd_strerror(error));
    }

    /* forget key */
    netmd_error cleanup_error = netmd_secure_session_key_forget(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_session_key_forget : %s\n", netmd_strerror(cleanup_error));

    /* leave session */
    cleanup_error = netmd_secure_leave_session(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(cleanup_error));

    if (audio_patch == apt_sp)
    {
        netmd_undo_sp_patch(devh);
    }

    /* release device - needed by Sharp devices, may fail on Sony devices */
    cleanup_error = netmd_release_dev(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_release_dev : %s\n", netmd_strerror(cleanup_error));

    return error; /* return error code from the "business logic" */
}

} // ~namespace
