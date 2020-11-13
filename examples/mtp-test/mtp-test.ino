#include "Arduino.h"

#include "MTP.h"
#include "usb1_mtp.h"

  #define SD_MOSI 11
  #define SD_MISO 12
  #define SD_SCK  13

  const char *sd_str[]={"sdio","sd1","sd2","sd3","sd4","sd5","sd6"}; // edit to rflect configuration
  const int cs[] = {BUILTIN_SDCARD,34,33,35,36,37,38}; // edit to reflect your configuration
  const int nsd = sizeof(cs)/sizeof(int);

SDClass sdx[nsd];

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

void storage_configure(MTPStorage_SD *storage, const char **sd_str, const int *cs, SDClass *sdx, int num)
{
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    storage->setStorageNumbers(sd_str,nsd);

    for(int ii=0; ii<nsd; ii++)
    { if(cs[ii] == BUILTIN_SDCARD)
      {
        if(!sdx[ii].sdfs.begin(SdioConfig(FIFO_SDIO))){Serial.println("No storage"); while(1);};
        uint32_t volCount  = sdx[ii].sdfs.clusterCount();
        uint32_t volFree  = sdx[ii].sdfs.freeClusterCount();
        uint32_t volClust = sdx[ii].sdfs.sectorsPerCluster();
        Serial.printf("Storage %d %d %d %d %d\n",ii,cs[ii],volCount,volFree,volClust);
      }
      else
      {
        pinMode(cs[ii],OUTPUT); digitalWriteFast(cs[ii],HIGH);
        if(!sdx[ii].sdfs.begin(SdSpiConfig(cs[ii], SHARED_SPI, SD_SCK_MHZ(33)))) {Serial.println("No storage"); while(1);}
        uint32_t volCount  = sdx[ii].sdfs.clusterCount();
        uint32_t volFree  = sdx[ii].sdfs.freeClusterCount();
        uint32_t volClust = sdx[ii].sdfs.sectorsPerCluster();
        Serial.printf("Storage %d %d %d %d %d\n",ii,cs[ii],volCount,volFree,volClust);
      }
    }
}

void setup()
{ 
  while(!Serial && millis()<3000); 
  Serial.println("MTP_test");
  
  usb_mtp_configure();
  storage_configure(&storage, sd_str,cs, sdx, nsd);

  Serial.println("Setup done");
  Serial.flush();
}

void loop()
{ 
  mtpd.loop();

  //logg(1000,"loop");
  //asm("wfi"); // may wait forever on T4.x
}
