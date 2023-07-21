# netmd++
This C++ API was written to ease the handling of NetMD devices. It is a synchronous API. So, function calls might block your program flow. If you want to use this API in an GUI app, better put the API calls into a background thread.

## Supported Devices
|Manufacturer | Manufacturer ID | Device ID | Name                      | Type  |
| ----------- | --------------- | --------- | ------------------------- | ----- |
|**Sony**     | 0x054c          | 0x0034    | Sony PCLK-XX              | NetMD |
|             | 0x054c          | 0x0036    | Sony NetMD Walkman        | NetMD |
|             | 0x054c          | 0x006F    | Sony NW-E7                | NetMD |
|             | 0x054c          | 0x0075    | Sony MZ-N1                | NetMD |
|             | 0x054c          | 0x007c    | Sony NetMD Walkman        | NetMD |
|             | 0x054c          | 0x0080    | Sony LAM-1                | NetMD |
|             | 0x054c          | 0x0081    | Sony MDS-JE780/JB980      | NetMD |
|             | 0x054c          | 0x0084    | Sony MZ-N505              | NetMD |
|             | 0x054c          | 0x0085    | Sony MZ-S1                | NetMD |
|             | 0x054c          | 0x0086    | Sony MZ-N707              | NetMD |
|             | 0x054c          | 0x008e    | Sony CMT-C7NT             | NetMD |
|             | 0x054c          | 0x0097    | Sony PCGA-MDN1            | NetMD |
|             | 0x054c          | 0x00ad    | Sony CMT-L7HD             | NetMD |
|             | 0x054c          | 0x00c6    | Sony MZ-N10               | NetMD |
|             | 0x054c          | 0x00c7    | Sony MZ-N910              | NetMD |
|             | 0x054c          | 0x00c8    | Sony MZ-N710/NE810/NF810  | NetMD |
|             | 0x054c          | 0x00c9    | Sony MZ-N510/NF610        | NetMD |
|             | 0x054c          | 0x00ca    | Sony MZ-NE410/DN430/NF520 | NetMD |
|             | 0x054c          | 0x00e7    | Sony CMT-M333NT/M373NT    | NetMD |
|             | 0x054c          | 0x00eb    | Sony MZ-NE810/NE910       | NetMD |
|             | 0x054c          | 0x0101    | Sony LAM                  | NetMD |
|**Aiwa**     | 0x054c          | 0x0113    | Aiwa AM-NX1               | NetMD |
|**Sony**     | 0x054c          | 0x011a    | Sony CMT-SE7              | NetMD |
|             | 0x054c          | 0x0119    | Sony CMT-SE9              | NetMD |
|             | 0x054c          | 0x013f    | Sony MDS-S500             | NetMD |
|             | 0x054c          | 0x0148    | Sony MDS-A1               | NetMD |
|**Aiwa**     | 0x054c          | 0x014c    | Aiwa AM-NX9               | NetMD |
|**Sony**     | 0x054c          | 0x017e    | Sony MZ-NH1               | HiMD  |
|             | 0x054c          | 0x0180    | Sony MZ-NH3D              | HiMD  |
|             | 0x054c          | 0x0182    | Sony MZ-NH900             | HiMD  |
|             | 0x054c          | 0x0184    | Sony MZ-NH700/800         | HiMD  |
|             | 0x054c          | 0x0186    | Sony MZ-NH600             | HiMD  |
|             | 0x054c          | 0x0187    | Sony MZ-NH600D            | HiMD  |
|             | 0x054c          | 0x0188    | Sony MZ-N920              | NetMD |
|             | 0x054c          | 0x018a    | Sony LAM-3                | NetMD |
|             | 0x054c          | 0x01e9    | Sony MZ-DH10P             | HiMD  |
|             | 0x054c          | 0x0219    | Sony MZ-RH10              | HiMD  |
|             | 0x054c          | 0x021b    | Sony MZ-RH910             | HiMD  |
|             | 0x054c          | 0x021d    | Sony CMT-AH10             | HiMD  |
|             | 0x054c          | 0x022c    | Sony CMT-AH10             | HiMD  |
|             | 0x054c          | 0x023c    | Sony DS-HMD1              | HiMD  |
|             | 0x054c          | 0x0286    | Sony MZ-RH1               | HiMD  |
|**Sharp**    | 0x04dd          | 0x7202    | Sharp IM-MT880H/MT899H    | NetMD |
|             | 0x04dd          | 0x9013    | Sharp IM-DR400/DR410      | NetMD |
|             | 0x04dd          | 0x9014    | Sharp IM-DR80/DR420/DR580 | NetMD |
|**Panasonic**| 0x04da          | 0x23b3    | Panasonic SJ-MR250        | NetMD |
|             | 0x04da          | 0x23b6    | Panasonic SJ-MR270        | NetMD |
|**Kenwood**  | 0x0b28          | 0x1004    | Kenwood MDX-J9            | NetMD |

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