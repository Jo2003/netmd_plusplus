/*
 * test.cpp
 *
 * This file is part of netmd++, a library for accessing NetMD devices.
 *
 * It makes use of knowledge / code collected by Marc Britten and
 * Alexander Sulfrian for the Linux Minidisc project.
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
#include <fstream>
#include <iostream>
#include <netmd++.h>
#include <cstring>

using namespace netmd;

//------------------------------------------------------------------------------
//! @brief      add bytes to byte vector
//!
//! @param      vec     The vector
//! @param[in]  data    The data
//! @param[in]  dataSz  The data size
//------------------------------------------------------------------------------
void addArrayData(NetMDByteVector& vec, const uint8_t* data, size_t dataSz)
{
    for (size_t i = 0; i < dataSz; i++)
    {
        vec.push_back(data[i]);
    }
}

int main (int argc, char* argv[])
{
    if ((argc == 3) && !strcmp("printToc", argv[1]))
    {
        std::ifstream toc(argv[2], std::ios_base::in | std::ios_base::binary);
        if (toc)
        {
            // get length of file:
            toc.seekg (0, toc.end);
            int length = toc.tellg();
            toc.seekg (0, toc.beg);

            char * buffer = new char [length];

            if (buffer)
            {
                std::cout << "Reading " << length << " characters... ";
                // read data as a block:
                toc.read(buffer, length);

                CNetMdTOC utoc(3, 459'000, reinterpret_cast<uint8_t*>(buffer));

                std::cout << utoc.discInfo() << std::endl;

                for (int i = 1; i <= utoc.trackCount(); i++)
                {
                    std::cout << utoc.trackInfo(i) << std::endl;
                }

                delete [] buffer;
            }

        }
        return 0;
    }
    netmd_pp* pNetMD = new netmd_pp();

    if (pNetMD)
    {
        pNetMD->setLogLevel(INFO);
        pNetMD->initHotPlug();

        if (pNetMD->initDevice() == NETMDERR_NO_ERROR)
        {
            std::cout << "NetMD device initialized!" << std::endl;

            // pNetMD->eraseDisc();
            pNetMD->sendAudioFile("./funky.wav", "Funky track", DiskFormat::NO_ONTHEFLY_CONVERSION);

            /*
            std::string s;

            if (pNetMD->discTitle(s) == NETMDERR_NO_ERROR)
            {
                std::cout << "Title: " << s << std::endl;
            }

            int i;

            if ((i = pNetMD->trackCount()) > -1)
            {
                std::cout << "Track Count: " << i << std::endl;

                for (int j = 0; j < i; j++)
                {
                    TrackTime tt;
                    pNetMD->trackTime(j, tt);

                    if (pNetMD->trackTitle(j, s) == NETMDERR_NO_ERROR)
                    {
                        std::cout << "Track " << (j + 1) << ": " << s << std::endl;
                    }
                }
            }


            if ((i = pNetMD->discFlags()) > -1)
            {
                std::cout << "Disc flags: " << std::hex << i << "h" << std::dec << std::endl;
            }

            s = pNetMD->getDeviceName();
            std::cout << "Device name: " << s << std::endl;

            if (argc > 1)
            {
                pNetMD->setDiscTitle(argv[1]);
                if (pNetMD->discTitle(s) == NETMDERR_NO_ERROR)
                {
                    std::cout << "Title: " << s << std::endl;
                }
            }

            bool support = pNetMD->spUploadSupported();

            std::cout << "Supports SP upload: " << support << std::endl;
            */
            if (pNetMD->prepareTOCManip() == NETMDERR_NO_ERROR)
            {
                NetMDByteVector toc;
                for (int x = 0; x < 3; x++)
                {
                    toc += pNetMD->readUTOCSector(static_cast<UTOCSector>(x));
                }

                if (toc.size() == (3 * 2352))
                {
                    std::cout << "TOC data read!" << std::endl;
                    uint8_t *pData = new uint8_t[toc.size()];

                    for(size_t i = 0; i < toc.size(); i++)
                    {
                        pData[i] = toc.at(i);
                    }

                    std::ofstream org("./org.bin", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                    if (org)
                    {
                        org.write(reinterpret_cast<const char*>(pData), 2352 * 3);
                        org.flush();
                    }

                    CNetMdTOC utoc(8, 459'000, pData);

                    int i = utoc.trackCount();

                    std::cout << "TOC tracks: " << i << std::endl;
                    std::cout << "TOC disc info:  " << utoc.discInfo() << std::endl;

                    for (int j = 1; j <= i; j++)
                    {
                        std::cout << "TOC track #" << j << ": " << utoc.trackInfo(j);
                    }

                    std::time_t now = 0;
                    time(&now);

                    utoc.addTrack(1, 60'000, "Funky Track One Minute Part #1", now);
                    utoc.addTrack(2, 60'000, "Funky Track One Minute Part #2", now);
                    utoc.addTrack(3, 60'000, "Funky Track One Minute Part #3", now);
                    utoc.addTrack(4, 60'000, "Funky Track One Minute Part #4", now);
                    utoc.addTrack(5, 60'000, "Funky Track One Minute Part #5", now);
                    utoc.addTrack(6, 60'000, "Funky Track One Minute Part #6", now);
                    utoc.addTrack(7, 60'000, "Funky Track One Minute Part #7", now);
                    utoc.addTrack(8, 39'000, "Funky Track Less Than One Minute Part #8", now);
                    utoc.setDiscTitle("8 Funky tracks on gapless NetMD");
                    
                    /*
                    utoc.addTrack(2, 261'490, "What Do You Want From Me");
                    utoc.addTrack(3, 424'560, "Poles Apart");
                    utoc.addTrack(4, 328'290, "Marooned");
                    utoc.addTrack(5, 257'070, "A Great Day For Freedom");
                    utoc.addTrack(6, 407'440, "Wearing The Inside Out");
                    utoc.addTrack(7, 372'270, "Take It Back");
                    utoc.addTrack(8, 379'450, "Coming Back To Life");
                    utoc.addTrack(9, 369'470, "Keep Talking");
                    utoc.addTrack(10, 314'730, "Lost For Words");
                    utoc.addTrack(11, 511'520, "High Hopes");
                    utoc.setDiscTitle("Pink Floyd - The Division Bell (7243 8 28984 2 9)");
                    */

                    i = utoc.trackCount();

                    std::cout << "TOC tracks: " << i << std::endl;
                    std::cout << "TOC disc info:  " << utoc.discInfo() << std::endl;

                    for (int j = 1; j <= i; j++)
                    {
                        std::cout << "TOC track #" << j << ": " << utoc.trackInfo(j);
                    }

                    // time to write TOC ...
                    bool doit = true;
                    for (int x = 0; x < 3; x++)
                    {
                        toc.clear();
                        addArrayData(toc, &pData[2352 * x], 2352);
                        if (pNetMD->writeUTOCSector(static_cast<UTOCSector>(x), toc) == NETMDERR_NO_ERROR)
                        {
                            std::cout << "TOC sector " << x << " written!" << std::endl;
                        }
                        else
                        {
                            doit = false;
                        }
                    }

                    if (doit)
                    {
                        pNetMD->finalizeTOC();
                    }

                    std::ofstream out("./toc.bin", std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                    if (out)
                    {
                        out.write(reinterpret_cast<const char*>(pData), 2352 * 3);
                        out.flush();
                    }

                    delete [] pData;

                    std::string s;

                    if (pNetMD->discTitle(s) == NETMDERR_NO_ERROR)
                    {
                        std::cout << "Title: " << s << std::endl;
                    }

                    if ((i = pNetMD->trackCount()) > -1)
                    {
                        std::cout << "Track Count: " << i << std::endl;

                        for (int j = 0; j < i; j++)
                        {
                            TrackTime tt;
                            pNetMD->trackTime(j, tt);

                            if (pNetMD->trackTitle(j, s) == NETMDERR_NO_ERROR)
                            {
                                std::cout << "Track " << (j + 1) << ": " << s << "(" << tt << ")" << std::endl;
                            }
                        }
                    }
                }
                else
                {
                    std::cout << "read " << toc.size() << " TOC bytes" << std::endl;
                }
            }
            else
            {
                std::cout << "Can't prepare TOC manip!" << std::endl;
            }
        }

        delete pNetMD;
    }

    return 0;
}
