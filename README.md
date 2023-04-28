# netmd++
This C++ API was written to ease the handling of NetMD devices. It is a synchronous API. So, function calls might block your program flow. If you want to use this API in an GUI app, better put the API calls into a background thread.

## Namespace
This API uses the namespace *netmd*.

## Usage

 - include the header file into your project:
```c++
#include "path/to/CNetMdApi.h"
```

 - create an instance of the API:
```c++
netmd::netmd_pp* pNetMd = new netmd::netmd_pp();
```

 - initialize the first found NetMD device:
```c++
if ((pNetMd != nullptr) && (pNetMd->initDevice() == netmd::NETMDERR_NO_ERROR))
{
    pNetMd->initDiscHeader();
}
```

 - If you change or re-plug the device, simply run above code (init) again!

## Examples
### Track transfer
Check for on-the-fly support and transfer a WAVE file to NetMD with on-the-fly encoding (LP2) or w/o encoding (SP).

```c++
#include <CNetMdApi.h>

int main()
{
    netmd::netmd_pp* pNetMd = new netmd::netmd_pp();

    if ((pNetMd != nullptr) && (pNetMd->initDevice() == netmd::NETMDERR_NO_ERROR))
    {
        pNetMd->initDiscHeader();

        if (pNetMd->otfEncodeSupported())
        {
            pNetMd->sendAudioFile("/path/to/nice/audio.wav", 
                                  "Very nice Audio file (LP2)", 
                                  netmd::NETMD_DISKFORMAT_LP2);
        }
        else
        {
            pNetMd->sendAudioFile("/path/to/nice/audio.wav", 
                                  "Very nice Audio file (SP)", 
                                  netmd::NO_ONTHEFLY_CONVERSION);
        }
    }
    return 0;
}
```

### Erase disc and set new title
```c++
#include <CNetMdApi.h>

int main()
{
    netmd::netmd_pp* pNetMd = new netmd::netmd_pp();

    if ((pNetMd != nullptr) && (pNetMd->initDevice() == netmd::NETMDERR_NO_ERROR))
    {
        pNetMd->eraseDisc();
        pNetMd->initDiscHeader();
        pNetMd->setDiscTitle("Amazing MD");

    }
    return 0;
}
```