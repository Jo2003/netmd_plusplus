#include "src/netmd++.h"

int main ()
{
    CNetMDpp* pNetMD = new CNetMDpp();

    if (pNetMD)
    {

        pNetMD->initDevice();

        std::string s;

        if (pNetMD->discTitle(s) == CNetMDpp::NETMDERR_NO_ERROR)
        {
            std::cout << "Title: " << s << std::endl;
        }

        int i;

        if ((i = pNetMD->trackCount()) > -1)
        {
            std::cout << "Track Count: " << i << std::endl;

            for (int j = 0; j < i; j++)
            {
                CNetMDpp::TrackTime tt;
                pNetMD->trackTime(j, tt);
            }
        }


        if ((i = pNetMD->discFlags()) > -1)
        {
            std::cout << "Disc flags: " << std::hex << i << "h" << std::dec << std::endl;
        }

        s = pNetMD->getDeviceName();
        std::cout << "Device name: " << s << std::endl;

        pNetMD->writeDiscHeader("Das ist eine Disc!");
        if (pNetMD->discTitle(s) == CNetMDpp::NETMDERR_NO_ERROR)
        {
            std::cout << "Title: " << s << std::endl;
        }


        delete pNetMD;
    }

    return 0;
}
