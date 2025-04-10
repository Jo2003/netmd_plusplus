# netmd++
## Namespace
This API uses the namespace *netmd*.

## Usage

 - include the header file into your project:
```c++
#include "path/to/netmd++.h"
```

 - create an instance of the API:
```c++
netmd::netmd_pp* pNetMd = new netmd::netmd_pp();
```

 - initialize the first found NetMD device:
```c++
if (pNetMd != nullptr)
{
    pNetMd->initHotPlug();
}
```

 - If you change or re-plug the device, it should be recognized by the hotplug implementation!

## Examples
### Track transfer
Check for on-the-fly support and transfer a WAVE file to NetMD with on-the-fly encoding (LP2) or w/o encoding (SP).

```c++
#include <netmd++.h>

int main()
{
    netmd::netmd_pp* pNetMd = new netmd::netmd_pp();

    if (pNetMd != nullptr)
    {
        pNetMd->initHotPlug();
        if (pNetMd->initDevice() == netmd::NETMDERR_NO_ERROR)
        {
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
    }
    return 0;
}
```

### Erase disc and set new title
```c++
#include <netmd++.h>

int main()
{
    netmd::netmd_pp* pNetMd = new netmd::netmd_pp();

    if (pNetMd != nullptr)
    {
        pNetMd->initHotPlug();
        if (pNetMd->initDevice() == netmd::NETMDERR_NO_ERROR)
        {
            pNetMd->eraseDisc();
            pNetMd->setDiscTitle("Amazing MD");

        }
    }
    return 0;
}
```

# MDs UTOC
For the UTOC structure please have a look at this great site on [minidisc.org](https://www.minidisc.org/md_toc.html)

## Addressing in UTOC
The disc start and end addresses each consist of a cluster, sector, and sound group, all packed into 3 bytes.
The smallest unit is a sound frame, representing 11.6ms of mono audio (212 bytes), while the smallest
<b>addressable</b> unit is the sound group, containing 2 sound frames. A sector contains 11
sound frames / 5.5 sound groups. Addressing must be done through sound group. Sound groups are numbered
0 ... 10. Sound groups 0 ... 5 are part of the even sector, while sound groups 5 ... 10 are part of the odd sector.
Group 5 overlaps both even and odd sectors and can therefore be addressed on both sectors.
<pre>
+-------------------------------------------+
|                sector pair                |
+---------------------+---------------------+
|   even sector (2n)  |  odd sector (2n+1)  |
+---+---+---+---+---+-+-+---+---+---+---+---+
| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| <- sound groups
+---+---+---+---+---+---+---+---+---+---+---+
</pre>
A cluster is an aggregate of 32 audio sectors (176 sound groups) representing 2.04 seconds of stereo audio; 
it is the smallest unit of data that can be written to a MiniDisc. In the 3 byte packing, there are 14 bits 
allocated to the cluster number, 6 bits to the sector, and 4 bits to the soundgroup; this arrangement allows 
addressing of up to 9.2 hours of stereo audio.

## Modifying the UTOC
 1. download the UTOC sectors 0 ... 2 from NetMD Device:
```c++
pNetMd->prepareTOCManip();
NetMDByteVector tocData;

for (int i = 0; i < 3; i++)
{
    tocData += pNetMd->readUTOCSector(static_cast<UTOCSector>(i));
}
```
 2. create toc class instance and add some track data
```c++
uint8_t *pData = new uint8_t[tocData.size()];
for(size_t i = 0; i < tocData.size(); i++)
{
    pData[i] = toc.at(i);
}

time_t now = 0;
time(&now);

netmd::CNetMdTOC utoc(8, 459'000, pData);
utoc.addTrack(1, 60'000, "Funky Track One Minute Part #1", now);
utoc.addTrack(2, 60'000, "Funky Track One Minute Part #2", now);
```
 3. upload changed TOC data to NetMD
```c++
bool doit = true;
for (int x = 0; x < 3; x++)
{
    tocData.clear();
    addArrayData(tocData, &pData[2352 * x], 2352);
    if (pNetMD->writeUTOCSector(static_cast<UTOCSector>(x), tocData) == NETMDERR_NO_ERROR)
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
delete [] pData;
```
