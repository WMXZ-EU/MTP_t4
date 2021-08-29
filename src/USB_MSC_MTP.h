#ifndef USB_MSC_MTP_H
#define USB_MSC_MTP_H


#if defined(__IMXRT1062__) || defined(ARDUINO_TEENSY36)
#include <mscFS.h>
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

#define USE_MSC_FAT 3    // set to > 0 experiment with MTP (USBHost.t36 + mscFS)
#define USE_MSC_FAT_VOL 8 // Max MSC FAT Volumes. 


// start off with one controller. 
msController msDrive[USE_MSC_FAT](myusb);
bool msDrive_previous[USE_MSC_FAT]; // Was this drive there the previous time through?
MSCClass msc[USE_MSC_FAT_VOL];
char  nmsc_str[USE_MSC_FAT_VOL][20];

uint16_t msc_storage_index[USE_MSC_FAT_VOL];
uint8_t msc_drive_index[USE_MSC_FAT_VOL]; // probably can find easy way not to need this.

// start off with one of these...
#include <MSC_MTP_Callback.h>
MSC_MTP_CB mscmtpcb;

class USB_MSC_MTP : public MTPStorageInterfaceCB
{
public:
   bool mbrDmp(msController *pdrv);
   void checkUSB(bool fInit);
   void dump_hexbytes(const void *ptr, int len);

private:

  #define DEFAULT_FILESIZE 1024
  
};
  

#else 
// Only those Teensy support USB
#warning "Only Teensy 3.6 and 4.x support MSC"
#endif
#endif

