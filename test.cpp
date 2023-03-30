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
        }


        if ((i = pNetMD->discFlags()) > -1)
        {
            std::cout << "Disc flags: " << std::hex << i << "h" << std::dec << std::endl;
        }

        s = pNetMD->getDeviceName();
        std::cout << "Device name: " << s << std::endl;

        delete pNetMD;
    }

    return 0;
}
