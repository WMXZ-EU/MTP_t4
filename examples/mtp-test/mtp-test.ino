#include "Arduino.h"

#include "SD.h"
#include <MTP.h>

#define USE_SD  1         // SDFAT based SDIO and SPI
#ifdef ARDUINO_TEENSY41
#define USE_LFS_RAM 1     // T4.1 PSRAM (or RAM)
#else
#define USE_LFS_RAM 0     // T4.1 PSRAM (or RAM)
#endif
#ifdef ARDUINO_TEENSY_MICROMOD
#define USE_LFS_QSPI 0    // T4.1 QSPI
#define USE_LFS_PROGM 1   // T4.4 Progam Flash
#define USE_LFS_SPI 0     // SPI Flash
#define USE_LFS_NAND 0
#define USE_LFS_QSPI_NAND 0
#define USE_LFS_FRAM 0
#else
#define USE_LFS_QSPI 1    // T4.1 QSPI
#define USE_LFS_PROGM 1   // T4.4 Progam Flash
#define USE_LFS_SPI 1     // SPI Flash
#define USE_LFS_NAND 1
#define USE_LFS_QSPI_NAND 0
#define USE_LFS_FRAM 0
#endif
#define USE_MSC_FAT 3    // set to > 0 experiment with MTP (USBHost.t36 + mscFS)
#define USE_MSC_FAT_VOL 8 // Max MSC FAT Volumes. 

extern "C" {
  extern uint8_t external_psram_size;
}

bool g_lowLevelFormat = true;

#if USE_EVENTS==1
extern "C" int usb_init_events(void);
#else
int usb_init_events(void) {}
#endif

#if USE_LFS_RAM==1 ||  USE_LFS_PROGM==1 || USE_LFS_QSPI==1 || USE_LFS_SPI==1
#include "LittleFS.h"
#endif
#if USE_LFS_NAND==1 ||  USE_LFS_QSPI_NAND==1
#include "LittleFS_NAND.h"
#endif

#if defined(__IMXRT1062__)
// following only as long usb_mtp is not included in cores
#if !__has_include("usb_mtp.h")
#include "usb1_mtp.h"
#endif
#else
#ifndef BUILTIN_SCCARD
#define BUILTIN_SDCARD 254
#endif
void usb_mtp_configure(void) {}
#endif


/****  Start device specific change area  ****/
//=============================================================================
// SDClasses
//=============================================================================
#if USE_SD==1
// edit SPI to reflect your configuration (following is for T4.1)
#define SD_MOSI 11
#define SD_MISO 12
#define SD_SCK  13



#define SPI_SPEED SD_SCK_MHZ(33)  // adjust to sd card 

#if defined (BUILTIN_SDCARD)
const char *sd_str[] = {"sdio", "sd1"}; // edit to reflect your configuration
const int cs[] = {BUILTIN_SDCARD, 10}; // edit to reflect your configuration
#else
const char *sd_str[] = {"sd1"}; // edit to reflect your configuration
const int cs[] = {10}; // edit to reflect your configuration
#endif
const int nsd = sizeof(sd_str) / sizeof(const char *);

SDClass sdx[nsd];

// Code to detect if SD Card is inserted. 
int  BUILTIN_SDCARD_missing_index = -1;
#if defined(ARDUINO_TEENSY41)
  #define _SD_DAT3 46
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD)
  #define _SD_DAT3 38
#elif defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
  #define _SD_DAT3 62
#endif

class SDMTPCB : public MTPStorageInterfaceCB {
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
};
SDMTPCB sdsmtpcb;

#endif

//=============================================================================
//LittleFS classes
//=============================================================================
// Setup a callback class for Littlefs storages..
class LittleFSMTPCB : public MTPStorageInterfaceCB {
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
};
LittleFSMTPCB lfsmtpcb;

#if USE_LFS_RAM==1
const char *lfs_ram_str[] = {"RAM1", "RAM2"};  // edit to reflect your configuration
const int lfs_ram_size[] = {200'000,4'000'000}; // edit to reflect your configuration
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
                            const char *lfs_spi_str[]={"sflash5","sflash6","prop"}; // edit to reflect your configuration
                            const int lfs_cs[] = {5,6, 7}; // edit to reflect your configuration
                            const int nfs_spi = sizeof(lfs_spi_str)/sizeof(const char *);

                            LittleFS_SPIFlash spifs[nfs_spi];
#endif
#if USE_LFS_NAND == 1
                            const char *nspi_str[]={"WINBOND1G", "WINBOND2G"};     // edit to reflect your configuration
                            const int nspi_cs[] = {3,4}; // edit to reflect your configuration
                            const int nspi_nsd = sizeof(nspi_cs)/sizeof(int);
                            LittleFS_SPINAND nspifs[nspi_nsd]; // needs to be declared if LittleFS is used in storage.h
#endif

//=============================================================================
// MSC FAT classes
//=============================================================================
#if USE_MSC_FAT > 0
#if defined(__IMXRT1062__) || defined(ARDUINO_TEENSY36)
#include <mscFS.h>
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);


#ifndef USE_MSC_FAT_VOL 
#define USE_MSC_FAT_VOL USE_MSC_FAT
#endif


// start off with one controller. 
msController msDrive[USE_MSC_FAT](myusb);
bool msDrive_previous[USE_MSC_FAT]; // Was this drive there the previous time through?
MSCClass msc[USE_MSC_FAT_VOL];
char  nmsc_str[USE_MSC_FAT_VOL][20];

uint16_t msc_storage_index[USE_MSC_FAT_VOL];
uint8_t msc_drive_index[USE_MSC_FAT_VOL]; // probably can find easy way not to need this.

extern bool mbrDmp(msController *pdrv);
extern void checkUSBandSDIOStatus(bool fInit);

class MSCMTPCB : public MTPStorageInterfaceCB {
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
  uint64_t usedSizeCB(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token) {
    Serial.printf("\n\n}}}}}}}}} MSCMTPCB::usedSizeCB called %x %u %u\n", (uint32_t)mtpstorage, store, user_token);
    if (msc[user_token].mscfs.fatType() == FAT_TYPE_FAT32) {
        Serial.printf("MSCMTPCB::usedSizeCB called for Fat32\n");  
    }
    return msc[user_token].usedSize();

  }
};
// start off with one of these...
MSCMTPCB mscmtpcb;

#else 
// Only those Teensy support USB
#warning "Only Teensy 3.6 and 4.x support MSC"
#define USE_MSC_FAT 0
#endif
#endif
//=============================================================================
// Global defines
//=============================================================================


                            MTPStorage_SD storage;
                            MTPD    mtpd(&storage);



//=============================================================================
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
      BUILTIN_SDCARD_missing_index = ii;
      #ifdef _SD_DAT3
      // for now lets leave it in PULLDOWN state...
      pinMode(_SD_DAT3, INPUT_PULLDOWN);
      // What happens if we add it and it failed to open?
      //storage.addFilesystem(sdx[ii], sd_str[ii]);
      #endif
    }
    else
    {
      storage.addFilesystem(sdx[ii], sd_str[ii], &sdsmtpcb, ii);
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
      storage.addFilesystem(sdx[ii], sd_str[ii], &sdsmtpcb, ii);
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
      storage.addFilesystem(ramfs[ii], lfs_ram_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&ramfs[ii]);
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
      storage.addFilesystem(progmfs[ii], lfs_progm_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&progmfs[ii]);
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
      storage.addFilesystem(qspifs[ii], lfs_qspi_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&qspifs[ii]);
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
      storage.addFilesystem(spifs[ii], lfs_spi_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&spifs[ii]);
      uint64_t totalSize = spifs[ii].totalSize();
      uint64_t usedSize  = spifs[ii].usedSize();
      Serial.printf("SPIFlash Storage %d %d %s ",ii,lfs_cs[ii],lfs_spi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
    }
  }
#endif
#if USE_LFS_NAND == 1
  for(int ii=0; ii<nspi_nsd;ii++) {
    pinMode(nspi_cs[ii],OUTPUT); digitalWriteFast(nspi_cs[ii],HIGH);
    if(!nspifs[ii].begin(nspi_cs[ii], SPI)) 
    { Serial.printf("SPIFlash NAND Storage %d %d %s failed or missing",ii,nspi_cs[ii],nspi_str[ii]); Serial.println();
    }
    else
    {
      storage.addFilesystem(nspifs[ii], nspi_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&nspifs[ii]);

      uint64_t totalSize = nspifs[ii].totalSize();
      uint64_t usedSize  = nspifs[ii].usedSize();
      Serial.printf("Storage %d %d %s ",ii,nspi_cs[ii],nspi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
    }
  }
#endif

#if USE_LFS_QSPI_NAND == 1
  for(int ii=0; ii<qnspi_nsd;ii++) {
    if(!qnspifs[ii].begin()) 
    { Serial.printf("QSPI NAND Storage %d %s failed or missing",ii,qnspi_str[ii]); Serial.println();
    }
    else
    {
      storage.addFilesystem(qnspifs[ii], qnspi_str[ii], &lfsmtpcb, (uint32_t)(LittleFS*)&qnspi_str[ii]);

      uint64_t totalSize = qnspifs[ii].totalSize();
      uint64_t usedSize  = qnspifs[ii].usedSize();
      Serial.printf("Storage %d %s ",ii,qnspi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
  }
#endif

// Start USBHost_t36, HUB(s) and USB devices.
#if USE_MSC_FAT > 0
  Serial.println("\nInitializing USB MSC drives...");
  checkUSBandSDIOStatus(true);
#endif

// Test to add missing SDCard to end of list if the card is missing.  We will update it later... 
#if defined(BUILTIN_SDCARD) && defined(_SD_DAT3)
  if (BUILTIN_SDCARD_missing_index != -1)
  {
      storage.addFilesystem(sdx[BUILTIN_SDCARD_missing_index], sd_str[BUILTIN_SDCARD_missing_index], &sdsmtpcb, BUILTIN_SDCARD_missing_index);
  }
#endif

}

//=============================================================================
// try to get the right FS for this store and then call it's format if we have one...
// Here is for LittleFS...
#if USE_LFS_RAM==1 ||  USE_LFS_PROGM==1 || USE_LFS_QSPI==1 || USE_LFS_SPI==1
uint8_t LittleFSMTPCB::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
{
  // Lets map the user_token back to oint to our object...
  Serial.printf("Format Callback: user_token:%x store: %u p2:%u post:%u \n", user_token, store, p2, post_process);
  LittleFS *lfs = (LittleFS*)user_token;
  // see if we have an lfs
  if (lfs)
  { 
    if (g_lowLevelFormat)
    {
      Serial.printf("Low Level Format: %s post: %u\n", storage.getStoreName(store), post_process);
      if (!post_process) return MTPStorageInterfaceCB::FORMAT_NEEDS_CALLBACK;
      
      if (lfs->lowLevelFormat('.')) return MTPStorageInterfaceCB::FORMAT_SUCCESSFUL;
    }
    else 
    {
      Serial.printf("Quick Format: %s\n", storage.getStoreName(store));
      if (lfs->quickFormat()) return MTPStorageInterfaceCB::FORMAT_SUCCESSFUL;
    }
  }
  else 
    Serial.println("littleFS not set in user_token");
  return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
}
#endif

//=============================================================================
// try to get the right FS for this store and then call it's format if we have one...
// Here is for MSC Drives (SDFat)
#if USE_MSC_FAT > 0
PFsLib pfsLIB;

uint8_t  sectorBuffer[512];
uint8_t volName[32];


uint8_t MSCMTPCB::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
{
  // Lets map the user_token back to oint to our object...
  Serial.printf("Format Callback: user_token:%x store: %u p2:%u post:%u \n", user_token, store, p2, post_process);


  if (msc[user_token].mscfs.fatType() == FAT_TYPE_FAT12) {
    Serial.printf("    Fat12 not supported\n");  
    return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  }

  // For all of these the fat ones will do on post_process
  if (!post_process) return MTPStorageInterfaceCB::FORMAT_NEEDS_CALLBACK;

  bool success = pfsLIB.formatter(msc[user_token].mscfs); 

  return success ? MTPStorageInterfaceCB::FORMAT_SUCCESSFUL : MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
}
#endif

#if USE_SD==1

// Callbacks for SD 
uint8_t SDMTPCB::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
{
  // Lets map the user_token back to oint to our object...
  Serial.printf("Format Callback: user_token:%x store: %u p2:%u post:%u \n", user_token, store, p2, post_process);

  // note the code for the pfs... is currently in the MSC library...
#if USE_MSC_FAT > 0

  if (sdx[user_token].sdfs.fatType() == FAT_TYPE_FAT12) {
    Serial.printf("    Fat12 not supported\n");  
    return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  }

  // For all of these the fat ones will do on post_process
  if (!post_process) return MTPStorageInterfaceCB::FORMAT_NEEDS_CALLBACK;

  PFsVolume partVol;
  if (!partVol.begin(sdx[user_token].sdfs.card(), true, 1)) return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  bool success = pfsLIB.formatter(partVol); 
  return success ? MTPStorageInterfaceCB::FORMAT_SUCCESSFUL : MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
#else
  return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
#endif
}
#endif


/****  End of device specific change area  ****/

//#if USE_SD==1
// Call back for file timestamps.  Only called for file create and sync(). needed by SDFat-beta
#include "TimeLib.h"
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10)
{ *date = FS_DATE(year(), month(), day());
  *time = FS_TIME(hour(), minute(), second());
  *ms10 = second() & 1 ? 100 : 0;
}
//#endif

void setup()
{
  // setup debug pins.
#if USE_MSC_FAT > 0
  // let msusb stuff startup as soon as possible
  myusb.begin();
#endif 
  for (uint8_t pin = 20; pin < 24; pin++) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }



#if defined(USB_MTPDISK_SERIAL)
  while (!Serial); // comment if you do not want to wait for terminal
#else
  //while(!Serial.available()); // comment if you do not want to wait for terminal (otherwise press any key to continue)
  while (!Serial && !Serial.available() && millis() < 5000) myusb.Task(); // or third option to wait up to 5 seconds and then continue
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

  {
    const char *str = "test1.txt";
    if (sdx[0].exists(str)) sdx[0].remove(str);
    File file = sdx[0].open(str, FILE_WRITE_BEGIN);
    file.println("This is a test line");
    file.close();

    Serial.println("\n**** dir of sd[0] ****");
    sdx[0].sdfs.ls();
  }

#endif
#if USE_LFS_RAM==1
  for (int ii = 0; ii < 10; ii++)
  { char filename[80];
    snprintf(filename, sizeof(filename), "/test_%d.txt", ii);
    File file = ramfs[0].open(filename, FILE_WRITE_BEGIN);
    file.println("This is a test line");
    file.close();
  }
  ramfs[0].mkdir("Dir0");
  for (int ii = 0; ii < 10; ii++)
  { char filename[80];
    snprintf(filename, sizeof(filename), "/Dir0/test_%d.txt", ii);
    File file = ramfs[0].open(filename, FILE_WRITE_BEGIN);
    file.println("This is a test line");
    file.close();
  }
  ramfs[0].mkdir("Dir0/dir1");
  for (int ii = 0; ii < 10; ii++)
  { char filename[80];
    snprintf(filename, sizeof(filename), "/Dir0/dir1/test_%d.txt", ii);
    File file = ramfs[0].open(filename, FILE_WRITE_BEGIN);
    file.println("This is a test line");
    file.close();
  }
  uint32_t buffer[256];
  File file = ramfs[1].open("LargeFile.bin", FILE_WRITE_BEGIN);
  for (int ii = 0; ii < 3000; ii++)
  { memset(buffer, ii % 256, 1024);
    file.write(buffer, 1024);
  }
  file.close();

#endif


  Serial.println("\nSetup done");
}

int ReadAndEchoSerialChar() {
  int ch = Serial.read();
  if (ch >= ' ') Serial.write(ch);
  return ch;
}

uint32_t last_storage_index = (uint32_t)-1;
#define DEFAULT_FILESIZE 1024

//Checks if a Card is present:
//- only when it is not in use! - 
#if USE_MSC_FAT > 0
bool mbrDmp(msController *pdrv) {
  MbrSector_t mbr;
  // bool valid = true;
  UsbFs msc;
  if (!msc.begin(pdrv)) return false;
  
  //Serial.printf("mbrDMP called %x\n", (uint32_t)pdrv); Serial.flush();
  if (pdrv->msReadBlocks(0, 1, 512, (uint8_t*)&mbr)) {
    Serial.print("\nread MBR failed.\n");
    //errorPrint();
    return false;
  }
  Serial.print("\nmsc # Partition Table\n");
  Serial.print("\tpart,boot,bgnCHS[3],type,endCHS[3],start,length\n");
  for (uint8_t ip = 1; ip < 5; ip++) {
    MbrPart_t *pt = &mbr.part[ip - 1];
    //    if ((pt->boot != 0 && pt->boot != 0X80) ||
    //        getLe32(pt->relativeSectors) > sdCardCapacity(&m_csd)) {
    //      valid = false;
    //    }
    switch (pt->type) {
      case 4:
      case 6:
      case 0xe:
        Serial.print("FAT16:\t");
        break;
      case 11:
      case 12:
        Serial.print("FAT32:\t");
        break;
      case 7:
        Serial.print("exFAT:\t");
        break;
      default:
        Serial.print("pt_#");
        Serial.print(pt->type);
        Serial.print(":\t");
        break;
    }
    Serial.print( int(ip)); Serial.print( ',');
    Serial.print(int(pt->boot), HEX); Serial.print( ',');
    for (int i = 0; i < 3; i++ ) {
      Serial.print("0x"); Serial.print(int(pt->beginCHS[i]), HEX); Serial.print( ',');
    }
    Serial.print("0x"); Serial.print(int(pt->type), HEX); Serial.print( ',');
    for (int i = 0; i < 3; i++ ) {
      Serial.print("0x"); Serial.print(int(pt->endCHS[i]), HEX); Serial.print( ',');
    }
    Serial.print(getLe32(pt->relativeSectors), DEC); Serial.print(',');
    Serial.println(getLe32(pt->totalSectors));
  }
  return true;
}


#endif

void checkUSBandSDIOStatus(bool fInit) {
#if USE_MSC_FAT > 0
  bool usb_drive_changed_state = false;
  myusb.Task(); // make sure we are up to date.
  if (fInit) {
    // make sure all of the indexes are -1..
    for (int ii = 0; ii < USE_MSC_FAT_VOL; ii++) {msc_storage_index[ii] = (uint16_t)-1; msc_drive_index[ii] = -1;}
    for (int ii = 0; ii < USE_MSC_FAT; ii++)  msDrive_previous[ii] = false;
  }

  for (int index_usb_drive = 0; index_usb_drive < USE_MSC_FAT; index_usb_drive++)
  {
    msController *pdriver = &msDrive[index_usb_drive];
    if (*pdriver != msDrive_previous[index_usb_drive]) {
      // Drive status changed.
      msDrive_previous[index_usb_drive] = *pdriver;
      usb_drive_changed_state = true; // something changed
      if (*pdriver) 
      {
        Serial.println("USB Drive Inserted");
        mbrDmp(pdriver);

        // Now lets see if we can iterate over all of the possible parititions of this drive
        for (int index_drive_partition=1; index_drive_partition < 5; index_drive_partition++) {
          // lets see if we can find an available msc object to use... 
          for (int index_msc = 0; index_msc < USE_MSC_FAT_VOL; index_msc++) {
            if (msc_storage_index[index_msc] == (uint16_t)-1) 
            {
              // lets try to open a partition.
              Serial.printf("  Try Partiton:%d on MSC Index:%d\n", index_drive_partition, index_msc);
              if (msc[index_msc].begin(pdriver, false, index_drive_partition))
              {
                Serial.println("    ** SUCCEEDED **");
                // now see if we can get the volume label.  
                char volName[20];
                if (msc[index_msc].mscfs.getVolumeLabel(volName, sizeof(volName))) {
                  Serial.printf(">> USB partition %d volume ID: %s\n", index_drive_partition, volName);
                  snprintf(nmsc_str[index_msc], sizeof(nmsc_str[index_msc]), "MSC%d-%s", index_usb_drive, volName);
                } 
                else snprintf(nmsc_str[index_msc], sizeof(nmsc_str[index_msc]), "MSC%d-%d", index_usb_drive, index_drive_partition);
                msc_drive_index[index_msc] = index_usb_drive;
                msc_storage_index[index_msc] = storage.addFilesystem(msc[index_msc], nmsc_str[index_msc], &mscmtpcb, index_msc);
#if 0

                elapsedMicros emmicro = 0;
                uint64_t totalSize = msc[index_usb_drive].totalSize();
                uint32_t elapsed_totalSize = emmicro;
                uint64_t usedSize  = msc[index_usb_drive].usedSize();
                Serial.printf("new Storage %d %s %llu(%u) %llu(%u)\n", index_msc, nmsc_str[index_msc], totalSize, elapsed_totalSize, usedSize, (uint32_t)emmicro - elapsed_totalSize); 
#endif                
                if (!fInit) mtpd.send_StoreAddedEvent(msc_storage_index[index_msc]);
              }
              break;
            }
          }
        }
      }
      else
      {
        // drive went away...
        for (int index_msc = 0; index_msc < USE_MSC_FAT_VOL; index_msc++) {
          // check for any indexes that were in use that were associated with that drive
          // Don't need to check for fInit here as we wont be removing drives during init...
          if (msc_drive_index[index_msc]== index_usb_drive) {
            mtpd.send_StoreRemovedEvent(msc_storage_index[index_msc]);
            storage.removeFilesystem(msc_storage_index[index_msc]);
            msc_storage_index[index_msc] = (uint16_t)-1;
            msc_drive_index[index_msc] = -1;
        }
        }
      }
    }
  }

  if (usb_drive_changed_state && !fInit) {
    delay(10); // give some time to handle previous one
    mtpd.send_DeviceResetEvent();
  }
#endif

#ifdef _SD_DAT3
  if (BUILTIN_SDCARD_missing_index != -1)
  {
   // delayMicroseconds(5);
    bool r = digitalReadFast(_SD_DAT3);
    if (r)
    {
      // looks like SD Inserted. so disable the pin for now...
      pinMode(_SD_DAT3, INPUT_DISABLE);

      delay(1);
      Serial.printf("\n*** SDIO Card Inserted ***");
      if(!sdx[BUILTIN_SDCARD_missing_index].sdfs.begin(SdioConfig(FIFO_SDIO)))
      { Serial.printf("SDIO Storage %d %d %s failed or missing",BUILTIN_SDCARD_missing_index,cs[BUILTIN_SDCARD_missing_index],sd_str[BUILTIN_SDCARD_missing_index]);  Serial.println();
      }
      else
      {
        // The SD is valid now... 
        uint32_t store = storage.getStoreID(sd_str[BUILTIN_SDCARD_missing_index]);
        if (store != 0xFFFFFFFFUL) 
        {
          mtpd.send_StoreRemovedEvent(store);
          delay(50);
          //mtpd.send_StorageInfoChangedEvent(store);
          mtpd.send_StoreAddedEvent(store);

        } else {
          // not in our list, try adding it
          store = storage.addFilesystem(sdx[BUILTIN_SDCARD_missing_index], sd_str[BUILTIN_SDCARD_missing_index]);
          mtpd.send_StoreAddedEvent(store);
        }


        uint64_t totalSize = sdx[BUILTIN_SDCARD_missing_index].totalSize();
        uint64_t usedSize  = sdx[BUILTIN_SDCARD_missing_index].usedSize();
        Serial.printf("SDIO Storage %d %d %s ",BUILTIN_SDCARD_missing_index,cs[BUILTIN_SDCARD_missing_index],sd_str[BUILTIN_SDCARD_missing_index]);
        Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);

      }
      BUILTIN_SDCARD_missing_index = -1; // only try this once
    }
  }
#endif  
}

void loop()
{

  mtpd.loop();

  checkUSBandSDIOStatus(false);

  if (Serial.available())
  {
    char pathname[MAX_FILENAME_LEN]; 
    uint32_t storage_index = 0;
    uint32_t file_size = 0;

    // Should probably use Serial.parse ...
    Serial.printf("\n *** Command line: ");
    int cmd_char = ReadAndEchoSerialChar();
    int ch = ReadAndEchoSerialChar();
    while (ch == ' ') ch = ReadAndEchoSerialChar();
    if (ch >= '0' && ch <= '9') {
      storage_index = ch - '0';
      ch = ReadAndEchoSerialChar();
      if (ch == 'x') {
        ch = ReadAndEchoSerialChar();
        for(;;) {
          if  (ch >= '0' && ch <= '9') storage_index = storage_index*16 + ch - '0';
          else if  (ch >= 'a' && ch <= 'f') storage_index = storage_index*16 + 10 + ch - 'a';
          else break;
          ch = ReadAndEchoSerialChar();
        }
      }
      else 
      {
        while (ch >= '0' && ch <= '9') {
          storage_index = storage_index*10 + ch - '0';
          ch = ReadAndEchoSerialChar();
        }
      }
      while (ch == ' ') ch = ReadAndEchoSerialChar();
      last_storage_index = storage_index;
    } else {
      storage_index = last_storage_index;
    }
    char *psz = pathname;
    while (ch > ' ') {
      *psz++ = ch;
      ch = ReadAndEchoSerialChar();
    }
    *psz = 0;
    while (ch == ' ') ch = ReadAndEchoSerialChar();
    while (ch >= '0' && ch <= '9') {
      file_size = file_size*10 + ch - '0';
      ch = ReadAndEchoSerialChar();
    }


    while(ReadAndEchoSerialChar() != -1) ;
    Serial.println();
    switch (cmd_char) 
    {
      case'r':
        Serial.println("Reset");
        mtpd.send_DeviceResetEvent();
        break;
      case 'd':
        {
          // first dump list of storages:
          uint32_t fsCount = storage.getFSCount();
          Serial.printf("\nDump Storage list(%u)\n", fsCount);
          for (uint32_t ii = 0; ii < fsCount; ii++) {
            Serial.printf("store:%u storage:%x name:%s fs:%x\n", ii, mtpd.Store2Storage(ii), storage.getStoreName(ii), (uint32_t)storage.getStoreFS(ii));
          }
          Serial.println("\nDump Index List");
          storage.dumpIndexList();
        }
        break;    

      case 'f':
        g_lowLevelFormat = !g_lowLevelFormat;
        if (g_lowLevelFormat) Serial.println("low level format of LittleFS disks selected");
        else Serial.println("Quick format of LittleFS disks selected");
        break;
#if USE_LFS_RAM==1
      case 'a':
        {
          Serial.println("Add Files");
          static int next_file_index_to_add = 100;
          uint32_t store = storage.getStoreID("RAM1");
          for (int ii = 0; ii < 10; ii++)
          { char filename[80];
            snprintf(filename, sizeof(filename),"/test_%d.txt", next_file_index_to_add++);
            Serial.println(filename);
            File file = ramfs[0].open(filename, FILE_WRITE_BEGIN);
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.println("This is a test line");
            file.close();
            mtpd.send_addObjectEvent(store, filename);
          }
          //  Notify PC on added files
          Serial.print("Store "); Serial.println(store);
          mtpd.send_StorageInfoChangedEvent(store);
        }
        break;
      case 'x':
        {
          Serial.println("Delete Files");
          static int next_file_index_to_delete = 100;
          uint32_t store = storage.getStoreID("RAM1");
          for (int ii = 0; ii < 10; ii++)
          { char filename[80];
            snprintf(filename, sizeof(filename), "/test_%d.txt", next_file_index_to_delete++);
            Serial.println(filename);
            if (ramfs[0].remove(filename))
            {
              mtpd.send_removeObjectEvent(store, filename);
            }
          }
          // attempt to notify PC on added files (does not work yet)
          Serial.print("Store "); Serial.println(store);
          mtpd.send_StorageInfoChangedEvent(store);
        }
        break;
#elif USE_SD==1
      case'a':
        {
          Serial.println("Add Files");
          static int next_file_index_to_add = 100;
          uint32_t store = storage.getStoreID("sdio");
          for (int ii = 0; ii < 10; ii++)
          { char filename[80];
            snprintf(filename, sizeof(filename), "/test_%d.txt", next_file_index_to_add++);
            Serial.println(filename);
            File file = sdx[0].open(filename, FILE_WRITE_BEGIN);
            file.println("This is a test line");
            file.close();
            mtpd.send_addObjectEvent(store, filename);
          }
          // attempt to notify PC on added files (does not work yet)
          Serial.print("Store "); Serial.println(store);
          mtpd.send_StorageInfoChangedEvent(store);
        }
        break;
      case 'x':
        {
          Serial.println("Delete Files");
          static int next_file_index_to_delete = 100;
          uint32_t store = storage.getStoreID("sdio");
          for (int ii = 0; ii < 10; ii++)
          { char filename[80];
            snprintf(filename, sizeof(filename), "/test_%d.txt", next_file_index_to_delete++);
            Serial.println(filename);
            if (sdx[0].remove(filename))
            {
              mtpd.send_removeObjectEvent(store, filename);
            }
          }
          // attempt to notify PC on added files (does not work yet)
          Serial.print("Store "); Serial.println(store);
          mtpd.send_StorageInfoChangedEvent(store);
        }
        break;
#endif        
      case 'e':
        Serial.printf("Sending event: %x\n", storage_index);
        mtpd.send_Event(storage_index);
        break;
      default:
        // show list of commands.
        Serial.println("\nCommands");
        Serial.println("  r - Reset mtp connection");
        Serial.println("  d - Dump storage list");
        Serial.println("  a - Add some dummy files");
        Serial.println("  x - delete dummy files");
        Serial.println("  f - toggle storage format type");
        Serial.println("  e <event> - Send some random event");
        break;    

      }
    }
  }

void dump_hexbytes(const void *ptr, int len)
{
  if (ptr == NULL || len <= 0) return;
  const uint8_t *p = (const uint8_t *)ptr;
  while (len) {
    for (uint8_t i = 0; i < 32; i++) {
      if (i > len) break;
      Serial.printf("%02X ", p[i]);
    }
    Serial.print(":");
    for (uint8_t i = 0; i < 32; i++) {
      if (i > len) break;
      Serial.printf("%c", ((p[i] >= ' ') && (p[i] <= '~')) ? p[i] : '.');
    }
    Serial.println();
    p += 32;
    len -= 32;
  }
}
