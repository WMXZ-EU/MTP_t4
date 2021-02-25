#include "Arduino.h"

#include "SD.h"
#include <MTP.h>

#define USE_SD  1         // SDFAT based SDIO and SPI
#ifdef ARDUINO_TEENSY41
#define USE_LFS_RAM 1     // T4.1 PSRAM (or RAM)
#else
#define USE_LFS_RAM 0     // T4.1 PSRAM (or RAM)
#endif
#define USE_LFS_QSPI 1    // T4.1 QSPI
#define USE_LFS_PROGM 1   // T4.4 Progam Flash
#define USE_LFS_SPI 1     // SPI Flash
#define USE_LFS_NAND 1
#define USE_LFS_QSPI_NAND 0
#define USE_LFS_FRAM 0
#define USE_MSC_FAT 2     // set to > 0 experiment with MTP (USBHost.t36 + mscFS)

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
#elif defined(ARDUINO_TEENSY40)
  #define _SD_DAT3 38
#elif defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
  #define _SD_DAT3 62
#endif
#endif

//=============================================================================
//LittleFS classes
//=============================================================================

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

// start off with one controller. 
msController msDrive[USE_MSC_FAT](myusb);
MSCClass msc[USE_MSC_FAT];
char *nmsc_str[USE_MSC_FAT] = {nullptr};
uint32_t msc_storage_index[USE_MSC_FAT] = {(uint32_t)-1};
char msc_volume_names[USE_MSC_FAT][20]= {"", ""};
extern bool mbrDmp(UsbFs *myMsc);
extern bool getUSBPartitionVolumeLabel(UsbFs *myMsc, uint8_t part, char *pszVolName, uint16_t cb);

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
#if USE_LFS_NAND == 1
  for(int ii=0; ii<nspi_nsd;ii++) {
    pinMode(nspi_cs[ii],OUTPUT); digitalWriteFast(nspi_cs[ii],HIGH);
    if(!nspifs[ii].begin(nspi_cs[ii], SPI)) 
    { Serial.printf("SPIFlash NAND Storage %d %d %s failed or missing",ii,nspi_cs[ii],nspi_str[ii]); Serial.println();
    }
    else
    {
      storage.addFilesystem(nspifs[ii], nspi_str[ii]);

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
      storage.addFilesystem(qnspifs[ii], qnspi_str[ii]);

      uint64_t totalSize = qnspifs[ii].totalSize();
      uint64_t usedSize  = qnspifs[ii].usedSize();
      Serial.printf("Storage %d %s ",ii,qnspi_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
  }
#endif

// Start USBHost_t36, HUB(s) and USB devices.
#if USE_MSC_FAT > 0
  char msc_drive_name[8];  // probably don't need more than 5 MSC1<cr>
  myusb.begin();

  Serial.println("\nInitializing USB MSC drives...");

  for (int ii = 0; ii < USE_MSC_FAT; ii++) {
    int cb = snprintf(msc_drive_name, sizeof(msc_drive_name), "MSC%d", ii);
    nmsc_str[ii] = (char*)malloc (cb+1);
    strcpy(nmsc_str[ii], msc_drive_name);

    msc_storage_index[ii] = (uint32_t)-1;
    if (msDrive[ii]) 
    {
      if (!msc[ii].begin(&msDrive[ii]))
        { Serial.printf("MSC Storage %s failed or missing", nmsc_str[ii]); Serial.println();
        }
        else
        {
          mbrDmp(&msc[ii].mscfs);
          if (getUSBPartitionVolumeLabel(&msc[ii].mscfs,1, msc_volume_names[ii], sizeof(msc_volume_names[ii]))) {
            Serial.printf(">> USB partition 0 valume ID: %s\n", msc_volume_names[ii]);
          }

          msc_storage_index[ii] = storage.addFilesystem(msc[ii], nmsc_str[ii], msc_volume_names[ii]);
          uint64_t totalSize = msc[ii].totalSize();
          uint64_t usedSize  = msc[ii].usedSize();
          Serial.printf("Storage %i %s ", ii, nmsc_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
        }
      }
  }
#endif

// Test to add missing SDCard to end of list if the card is missing.  We will update it later... 
#if defined(BUILTIN_SDCARD) && defined(_SD_DAT3)
  if (BUILTIN_SDCARD_missing_index != -1)
  {
      storage.addFilesystem(sdx[BUILTIN_SDCARD_missing_index], sd_str[BUILTIN_SDCARD_missing_index]);
  }
#endif

}

//=============================================================================
// try to get the right FS for this store and then call it's format if we have one...
bool mtpd_format_cb(uint32_t store, uint32_t p2, bool post_process)
{
// lets try to map the store to the actual FS object.
Serial.printf("Format Callback: store: %u p2:%u\n", store, p2);
  FS *store_fs = storage.getStoreFS(store);
  LittleFS *lfs = nullptr;
  SDClass *sfs = nullptr;
#if USE_SD==1
  for (int ii = 0; ii < nsd; ii++)
  {
    if (store_fs == &sdx[ii] )
    { sfs = &sdx[ii];
      break;
    }
  }
#endif

#if USE_LFS_RAM==1
  for (int ii = 0; ii < nfs_ram; ii++)
  {
    if (store_fs == &ramfs[ii] )
    { lfs = &ramfs[ii];
      break;
    }
  }
#endif

#if USE_LFS_PROGM==1
  for (int ii = 0; ii < nfs_progm; ii++)
  {
    if (store_fs == &progmfs[ii] )
    { lfs = &progmfs[ii];
      break;
    }
  }
#endif

#if USE_LFS_QSPI==1
  for (int ii = 0; ii < nfs_qspi; ii++)
  {
    if (store_fs == &qspifs[ii] )
    { lfs = &qspifs[ii];
      break;
    }
  }
#endif

#if USE_LFS_SPI==1
  for (int ii = 0; ii < nfs_spi; ii++)
  {
    if (store_fs == &spifs[ii] )
    { lfs = &spifs[ii];
      break;
    }
  }
#endif
#if USE_LFS_NAND == 1
  for (int ii = 0; ii < nspi_nsd; ii++)
  {
    if (store_fs == &nspifs[ii] )
    { lfs = &nspifs[ii];
      break;
    }
  }
#endif
  // see if we have an lfs
  if (lfs)
  { 
    if (g_lowLevelFormat)
    {
      Serial.printf("Low Level Format: %s post: %u\n", storage.getStoreName(store), post_process);
      if (post_process)return lfs->lowLevelFormat('.');
      else return true; // do it on the post process
    }
    else 
    {
      Serial.printf("Quick Format: %s\n", storage.getStoreName(store));
      if (!post_process) return lfs->quickFormat();
      return true; // 
    }
  }
  else if (sfs)
  { Serial.printf("Format of SD types not implemented yet\n");

  }
  else Serial.println("Did not find Store in list???");
  return false;

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
  // setup debug pins.
  for (uint8_t pin = 20; pin < 24; pin++) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }

#if defined(USB_MTPDISK_SERIAL)
  while (!Serial); // comment if you do not want to wait for terminal
#else
  //while(!Serial.available()); // comment if you do not want to wait for terminal (otherwise press any key to continue)
  while (!Serial.available() && millis() < 5000); // or third option to wait up to 5 seconds and then continue
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

  // WIP - try format callback code.
  mtpd.setFormatCB(&mtpd_format_cb);

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
bool getUSBPartitionVolumeLabel(UsbFs *myMsc, uint8_t part, char *pszVolName, uint16_t cb) {
  MbrSector_t mbr;
  uint8_t buf[512];
  if (!pszVolName || (cb < 12)) return false; // don't want to deal with it
  myMsc->usbDrive()->readSector(0, (uint8_t*)&mbr);
  MbrPart_t *pt = &mbr.part[part - 1];
  switch (pt->type) {
    case 4:
    case 6:
    case 0xe:
      {
        FatVolume partVol;
        Serial.print("FAT16:\t");

        partVol.begin(myMsc->usbDrive(), true, part);
        partVol.chvol();
        myMsc->usbDrive()->readSector(partVol.rootDirStart(), buf);

        size_t i;
        for (i = 0; i < 11; i++) {
          pszVolName[i]  = buf[i];
        }
        while ((i > 0) && (pszVolName[i - 1] == ' ')) i--; // trim off trailing blanks
        pszVolName[i] = 0;
      }
      break;
    case 11:
    case 12:
      {
        FatVolume partVol;
        Serial.print("FAT32:\t");
        partVol.begin(myMsc->usbDrive(), true, part);
        partVol.chvol();
        myMsc->usbDrive()->readSector(partVol.dataStartSector(), buf);

        size_t i;
        for (i = 0; i < 11; i++) {
          pszVolName[i]  = buf[i];
        }
        while ((i > 0) && (pszVolName[i - 1] == ' ')) i--; // trim off trailing blanks
        pszVolName[i] = 0;
      }
      break;
    case 7:
      {
        Serial.print("exFAT:\t");
        ExFatFile root;
        DirLabel_t *dir;

        ExFatVolume expartVol;

        expartVol.begin(myMsc->usbDrive(), true, part);
        expartVol.chvol();
        if (!root.openRoot(&expartVol)) {
          Serial.println("openRoot failed");
          return false;
        }
        root.read(buf, 32);
        dir = reinterpret_cast<DirLabel_t*>(buf);

        size_t i;
        for (i = 0; i < dir->labelLength; i++) {
          pszVolName[i] = dir->unicode[2 * i];
        }
        pszVolName[i] = 0;
      }

      break;
    default:
      return false;
  }
  return true;
}

bool mbrDmp(UsbFs *myMsc) {
  MbrSector_t mbr;
  // bool valid = true;
  if (!myMsc->usbDrive()->readSector(0, (uint8_t*)&mbr)) {
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

//------------------------------------------------------------------------------
// Check for a connected USB drive and try to mount if not mounted.
bool driveAvailable(msController *pDrive,MSCClass *mscVol) {
  if(pDrive->checkConnectedInitialized()) {
    return false; // No USB Drive connected, give up!
  }
  if(!mscVol->mscfs.fatType()) {  // USB drive present try mount it.
    if (!mscVol->mscfs.begin(pDrive)) {
      mscVol->mscfs.initErrorPrint(&Serial); // Could not mount it print reason.
      return false;
    }
  }

  return true;
}



#endif

void checkUSBandSDIOStatus() {
#if USE_MSC_FAT > 0
  bool usb_drive_changed_state = false;
  myusb.Task(); // make sure we are up to date.
  for (int ii = 0; ii < USE_MSC_FAT; ii++)
  {
    USBDriver *pdriver = &msDrive[ii];
    if (*pdriver) 
    {
      if (msc_storage_index[ii] == (uint32_t)-1) 
      {
        // lets try to add this storage...
        Serial.println("USB Drive Inserted");
        if (msc[ii].begin(&msDrive[ii]))
        {
          // experiment to see if we can get some additional information. 
          mbrDmp(&msc[ii].mscfs);
          if (getUSBPartitionVolumeLabel(&msc[ii].mscfs,1, msc_volume_names[ii], sizeof(msc_volume_names[ii]))) {
            Serial.printf(">> USB partition 0 valume ID: %s\n", msc_volume_names[ii]);
          }

          msc_storage_index[ii] = storage.addFilesystem(msc[ii], nmsc_str[ii], msc_volume_names[ii]);
          elapsedMicros emmicro = 0;
          uint64_t totalSize = msc[ii].totalSize();
          uint32_t elapsed_totalSize = emmicro;
          uint64_t usedSize  = msc[ii].usedSize();
          Serial.printf("new Storage %d %s %llu(%u) %llu(%u)\n", ii, nmsc_str[ii], totalSize, elapsed_totalSize, usedSize, (uint32_t)emmicro - elapsed_totalSize); 
          usb_drive_changed_state = true;
          mtpd.send_StoreAddedEvent(msc_storage_index[ii]);
        }
      }

    } else if (msc_storage_index[ii] != (uint32_t)-1) {
        mtpd.send_StoreRemovedEvent(msc_storage_index[ii]);
        storage.removeFilesystem(msc_storage_index[ii]);
        msc_storage_index[ii] = (uint32_t)-1;
        usb_drive_changed_state = true;

    }
  }

  if (usb_drive_changed_state) {
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

  checkUSBandSDIOStatus();

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
            Serial.printf("store:%u name:%s fs:%x\n", ii, storage.getStoreName(ii), (uint32_t)storage.getStoreFS(ii));
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
