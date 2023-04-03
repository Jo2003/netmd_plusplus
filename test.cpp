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
#include "src/CNetMdApi.h"

using namespace netmd;

int main (int argc, char* argv[])
{
    CNetMdApi* pNetMD = new CNetMdApi();

    if (pNetMD)
    {

        pNetMD->initDevice();

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
                CNetMdApi::TrackTime tt;
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
            pNetMD->writeDiscHeader(argv[1]);
            if (pNetMD->discTitle(s) == NETMDERR_NO_ERROR)
            {
                std::cout << "Title: " << s << std::endl;
            }
        }

        bool support = pNetMD->spUploadSupported();

        std::cout << "Supports SP upload: " << support << std::endl;

        delete pNetMD;
    }

    return 0;
}
