#include "Arduino.h"

#if __has_include("LittleFS.h")
  #define DO_LITTLEFS 0    // set to zero if not wanted // needs LittleFS installed as library
#else
  #define DO_LITTLEFS 0
#endif

#include "MTP.h"
#include "usb1_mtp.h"


/****  Start device specific change area  ****/

  // edit SPI to reflect your configuration (following is fot T4.1)
  #define SD_MOSI 11
  #define SD_MISO 12
  #define SD_SCK  13

//  const char *sd_str[]={"sdio","sd1","sd2","sd3","sd4","sd5","sd6"}; // WMXZ example
//  const int cs[] = {BUILTIN_SDCARD,34,33,35,36,37,38}; // WMXZ example

//  const char *sd_str[]={"sdio","sd6"}; // WMXZ testing
//  const int cs[] = {BUILTIN_SDCARD,38}; // WMXZ testing
#if DO_LITTLEFS==1
  const char *sd_str[]={"sdio","RAM"};      // edit to reflect your configuration
  const int cs[] = {BUILTIN_SDCARD, 256};  // edit to reflect your configuration
#else
//  const char *sd_str[]={"sdio"};      // edit to reflect your configuration
//  const int cs[] = {BUILTIN_SDCARD};  // edit to reflect your configuration
  const char *sd_str[]={"sd1"};         // edit to reflect your configuration
  const int cs[] = {38};                // edit to reflect your configuration
#endif
  const int nsd = sizeof(cs)/sizeof(int);

SDClass sdx[nsd];
#if __has_include("LittleFS.h")
  LittleFS_RAM ramfs; // needs to be defined if LittleFS.h exists in include path
#endif

void storage_configure(MTPStorage_SD *storage, const char **sd_str, const int *cs, SDClass *sdx, int num)
{
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    storage->setStorageNumbers(sd_str, cs, nsd);

    for(int ii=0; ii<nsd; ii++)
    { if(cs[ii] == BUILTIN_SDCARD)
      {
        if(!sdx[ii].sdfs.begin(SdioConfig(FIFO_SDIO))){Serial.println("No storage"); while(1);};
      }
      else if(cs[ii]<BUILTIN_SDCARD)
      {
        pinMode(cs[ii],OUTPUT); digitalWriteFast(cs[ii],HIGH);
        if(!sdx[ii].sdfs.begin(SdSpiConfig(cs[ii], SHARED_SPI, SD_SCK_MHZ(33)))) {Serial.println("No storage"); while(1);}
      }
      #if DO_LITTLEFS==1
        else if(cs[ii]==256) // LittleFS
        { if(!ramfs.begin(8'000'000)) { Serial.println("No storage"); while(1);}
        }
      #endif
      if(ii<256)
      {
        uint32_t volCount  = sdx[ii].sdfs.clusterCount();
        uint32_t volFree  = sdx[ii].sdfs.freeClusterCount();
        uint32_t volClust = sdx[ii].sdfs.sectorsPerCluster();
        Serial.printf("Storage %d %d %s %d %d %d\n",ii,cs[ii],sd_str[ii],volCount,volFree,volClust);
      }
    }
}
/****  End of device specific change area  ****/

MTPStorage_SD storage;
MTPD       mtpd(&storage);

void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    Serial.println(txt); 
    to=millis();
  }
}

void setup()
{ 
  while(!Serial && millis()<3000); 
  Serial.println("MTP_test");
  
  usb_mtp_configure();
  storage_configure(&storage, sd_str,cs, sdx, nsd);

  #if DO_LITTLEFS==1
  // store some files into disks (but only once)
  for(int ii=0; ii<10;ii++)
  { char filename[80];
    sprintf(filename,"test_%d.txt",ii);
    if(!ramfs.exists(filename))
    {
      File file=ramfs.open(filename,FILE_WRITE_BEGIN);
      file.println("This is a test line");
      file.close();
    }
  }
  #endif

  Serial.println("Setup done");
  Serial.flush();
}

void loop()
{ 
  mtpd.loop();

  //logg(1000,"loop");
  //asm("wfi"); // may wait forever on T4.x
}
