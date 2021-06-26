#include "Arduino.h"

#include "SD.h"
#include "MTP.h"

#define USE_SD  1         // SDFAT based SDIO and SPI
#define USE_LFS_RAM 0     // T4.1 PSRAM (or RAM)
#define USE_LFS_QSPI 0    // T4.1 QSPI
#define USE_LFS_PROGM 0   // T4.1 Progam Flash
#define USE_LFS_SPI 0     // SPI Flash

#if USE_EVENTS==1
  extern "C" int usb_init_events(void);
#else
  int usb_init_events(void) {}
#endif

#if USE_LFS_RAM==1 ||  USE_LFS_PROGM==1 || USE_LFS_QSPI==1 || USE_LFS_SPI==1
  #include "LittleFS.h"
#endif

#if defined(__IMXRT1062__)
  // following only as long usb_mtp is not included in cores
  #if !__has_include("usb_mtp.h")
    #include "usb1_mtp.h"
  #endif
#else
  #ifndef BUILTIN_SDCARD 
    #define BUILTIN_SDCARD 254
  #endif
  void usb_mtp_configure(void) {}
#endif


/****  Start device specific change area  ****/
// SDClasses 
#if USE_SD==1
  // edit SPI to reflect your configuration (following is for T4.1)
  #define SD_MOSI 11
  #define SD_MISO 12
  #define SD_SCK  13

  #define SPI_SPEED SD_SCK_MHZ(33)  // adjust to sd card 

  #if defined (BUILTIN_SDCARD)
    const char *sd_str[]={"sdio","sd1"}; // edit to reflect your configuration
    const int cs[] = {BUILTIN_SDCARD,10}; // edit to reflect your configuration
  #else
    const char *sd_str[]={"sd1"}; // edit to reflect your configuration
    const int cs[] = {10}; // edit to reflect your configuration
  #endif
  const int nsd = sizeof(sd_str)/sizeof(const char *);

SDClass sdx[nsd];
#endif

//LittleFS classes
#if USE_LFS_RAM==1
  const char *lfs_ram_str[]={"RAM1","RAM2"};     // edit to reflect your configuration
  const int lfs_ram_size[] = {2'000'000,4'000'000}; // edit to reflect your configuration
  const int nfs_ram = sizeof(lfs_ram_str)/sizeof(const char *);

  LittleFS_RAM ramfs[nfs_ram]; 
#endif

#if USE_LFS_QSPI==1
  const char *lfs_qspi_str[]={"QSPI"};     // edit to reflect your configuration
  const int nfs_qspi = sizeof(lfs_qspi_str)/sizeof(const char *);

  LittleFS_QSPIFlash qspifs[nfs_qspi]; 
#endif

#if USE_LFS_PROGM==1
  const char *lfs_progm_str[]={"PROGM"};     // edit to reflect your configuration
  const int lfs_progm_size[] = {1'000'000}; // edit to reflect your configuration
  const int nfs_progm = sizeof(lfs_progm_str)/sizeof(const char *);

  LittleFS_Program progmfs[nfs_progm]; 
#endif

#if USE_LFS_SPI==1
  const char *lfs_spi_str[]={"nand1","nand2","nand3","nand4"}; // edit to reflect your configuration
  const int lfs_cs[] = {3,4,5,6}; // edit to reflect your configuration
  const int nfs_spi = sizeof(lfs_spi_str)/sizeof(const char *);

LittleFS_SPIFlash spifs[nfs_spi];
#endif


MTPStorage_SD storage;
MTPD    mtpd(&storage);

void storage_configure()
{
  #if USE_SD==1
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    for(int ii=0; ii<nsd; ii++)
    { 
      #if defined(BUILTIN_SDCARD)
        if(cs[ii] == BUILTIN_SDCARD)
        {
          if(!sdx[ii].sdfs.begin(SdioConfig(FIFO_SDIO))) 
          { Serial.printf("SDIO Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
          }
          else
          {
            storage.addFilesystem(sdx[ii], sd_str[ii]);
            uint64_t totalSize = sdx[ii].totalSize();
            uint64_t usedSize  = sdx[ii].usedSize();
            Serial.printf("SDIO Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
            Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
          }
        }
        else if(cs[ii]<BUILTIN_SDCARD)
      #endif
      {
        pinMode(cs[ii],OUTPUT); digitalWriteFast(cs[ii],HIGH);
        if(!sdx[ii].sdfs.begin(SdSpiConfig(cs[ii], SHARED_SPI, SPI_SPEED))) 
        { Serial.printf("SD Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
        }
        else
        {
          storage.addFilesystem(sdx[ii], sd_str[ii]);
          uint64_t totalSize = sdx[ii].totalSize();
          uint64_t usedSize  = sdx[ii].usedSize();
          Serial.printf("SD Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
          Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
        }
      }
    }
    #endif

    #if USE_LFS_RAM==1
    for(int ii=0; ii<nfs_ram;ii++)
    {
      if(!ramfs[ii].begin(lfs_ram_size[ii])) 
      { Serial.printf("Ram Storage %d %s failed or missing",ii,lfs_ram_str[ii]); Serial.println();
      }
      else
      {
        storage.addFilesystem(ramfs[ii], lfs_ram_str[ii]);
        uint64_t totalSize = ramfs[ii].totalSize();
        uint64_t usedSize  = ramfs[ii].usedSize();
        Serial.printf("RAM Storage %d %s ",ii,lfs_ram_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
      }
    }
    #endif

    #if USE_LFS_PROGM==1
    for(int ii=0; ii<nfs_progm;ii++)
    {
      if(!progmfs[ii].begin(lfs_progm_size[ii])) 
      { Serial.printf("Program Storage %d %s failed or missing",ii,lfs_progm_str[ii]); Serial.println();
      }
      else
      {
        storage.addFilesystem(progmfs[ii], lfs_progm_str[ii]);
        uint64_t totalSize = progmfs[ii].totalSize();
        uint64_t usedSize  = progmfs[ii].usedSize();
        Serial.printf("Program Storage %d %s ",ii,lfs_progm_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
      }
    }
    #endif

    #if USE_LFS_QSPI==1
    for(int ii=0; ii<nfs_qspi;ii++)
    {
      if(!qspifs[ii].begin()) 
      { Serial.printf("QSPI Storage %d %s failed or missing",ii,lfs_qspi_str[ii]); Serial.println();
      }
      else
      {
        storage.addFilesystem(qspifs[ii], lfs_qspi_str[ii]);
        uint64_t totalSize = qspifs[ii].totalSize();
        uint64_t usedSize  = qspifs[ii].usedSize();
        Serial.printf("QSPI Storage %d %s ",ii,lfs_qspi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
      }
    }
    #endif

    #if USE_LFS_SPI==1
    for(int ii=0; ii<nfs_spi;ii++)
    {
      if(!spifs[ii].begin(lfs_cs[ii])) 
      { Serial.printf("SPIFlash Storage %d %d %s failed or missing",ii,lfs_cs[ii],lfs_spi_str[ii]); Serial.println();
      }
      else
      {
        storage.addFilesystem(spifs[ii], lfs_spi_str[ii]);
        uint64_t totalSize = spifs[ii].totalSize();
        uint64_t usedSize  = spifs[ii].usedSize();
        Serial.printf("SPIFlash Storage %d %d %s ",ii,lfs_cs[ii],lfs_spi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
      }
    }
    #endif
}
/****  End of device specific change area  ****/

  #if USE_SD==1
    // Call back for file timestamps.  Only called for file create and sync(). needed by SDFat-beta
    #include "TimeLib.h"
    void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
    { *date = FS_DATE(year(), month(), day());
      *time = FS_TIME(hour(), minute(), second());
      *ms10 = second() & 1 ? 100 : 0;
    }
  #endif

void setup()
{ 
  #if defined(USB_MTPDISK_SERIAL) 
    while(!Serial); // comment if you do not want to wait for terminal
  #else
    while(!Serial.available()); // comment if you do not want to wait for terminal (otherwise press any key to continue)
  #endif
  Serial.println("MTP_test");

  #if USE_EVENTS==1
    usb_init_events();
  #endif

  #if !__has_include("usb_mtp.h")
    usb_mtp_configure();
  #endif
  storage_configure();

  #if USE_SD==1
  // Set Time callback // needed for SDFat
  FsDateTime::callback = dateTime;

  if(1){
    const char *str = "/test1.txt";
    if(sdx[0].exists(str)) sdx[0].remove(str);
    File file=sdx[0].open(str,FILE_WRITE_BEGIN);
        file.println("This is a test line");
    file.close();

    Serial.println("\n**** dir of sd[0] ****");
    sdx[0].sdfs.ls();
  }

  #endif
  #if USE_LFS_RAM==1
    for(int ii=0; ii<10;ii++)
    { char filename[80];
      sprintf(filename,"/test_%d.txt",ii);
      File file=ramfs[0].open(filename,FILE_WRITE_BEGIN);
        file.println("This is a test line");
      file.close();
    }
    ramfs[0].mkdir("Dir0");
    for(int ii=0; ii<10;ii++)
    { char filename[80];
      sprintf(filename,"/Dir0/test_%d.txt",ii);
      File file=ramfs[0].open(filename,FILE_WRITE_BEGIN);
        file.println("This is a test line");
      file.close();
    }
    ramfs[0].mkdir("Dir0/dir1");
    for(int ii=0; ii<10;ii++)
    { char filename[80];
      sprintf(filename,"/Dir0/dir1/test_%d.txt",ii);
      File file=ramfs[0].open(filename,FILE_WRITE_BEGIN);
        file.println("This is a test line");
      file.close();
    }
    uint32_t buffer[256];
    File file = ramfs[1].open("LargeFile.bin",FILE_WRITE_BEGIN);
    for(int ii=0;ii<3000;ii++)
    { memset(buffer,ii%256,1024);
      file.write(buffer,1024);
    }
    file.close();

  #endif

  Serial.println("\nSetup done");
}

void loop()
{ 
  mtpd.loop();

#if USE_EVENTS==1
  if(Serial.available())
  {
    char ch=Serial.read();
    Serial.println(ch);
    if(ch=='r') 
    {
      Serial.println("Reset");
      mtpd.send_DeviceResetEvent();
    }
    #if USE_LFS_RAM==1
      if(ch=='a') 
      {
        Serial.println("Add Files");
        static int count=100;
        for(int ii=0; ii<10;ii++)
        { char filename[80];
          sprintf(filename,"/test_%d.txt",count++);
          Serial.println(filename);
          File file=ramfs[0].open(filename,FILE_WRITE_BEGIN);
            file.println("This is a test line");
          file.close();
        }
        // attempt to notify PC on added files (does not work yet)
        uint32_t store = storage.getStoreID("RAM1");
        Serial.print("Store "); Serial.println(store);
        mtpd.send_StorageInfoChangedEvent(store);
      }
    #endif
  }
#endif
}
