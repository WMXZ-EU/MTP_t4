#ifndef USB_MSC_MTP_H
#define USB_MSC_MTP_H


#if defined(__IMXRT1062__) || defined(ARDUINO_TEENSY36)
#include <MTP.h>
#include <mscFS.h>

#define USE_MSC_FAT 3    // set to > 0 experiment with MTP (USBHost.t36 + mscFS)
#define USE_MSC_FAT_VOL 8 // Max MSC FAT Volumes. 


class USB_MSC_MTP : public MTPStorageInterfaceCB, public MTPStorage_SD
{
public:
   void begin();
   bool mbrDmp(msController *pdrv);
   void checkUSB(MTPStorage_SD *mtpstorage, bool fInit);
   void dump_hexbytes(const void *ptr, int len);

private:

  #define DEFAULT_FILESIZE 1024
  
};
  

#else 
// Only those Teensy support USB
#warning "Only Teensy 3.6 and 4.x support MSC"
#endif
#endif

