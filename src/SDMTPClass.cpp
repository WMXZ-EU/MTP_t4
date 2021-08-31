#include <SDMTPClass.h>
#include <mscFS.h>



//=============================================================================
// try to get the right FS for this store and then call it's format if we have one...
// Here is for LittleFS...
PFsLib pfsLIB;
uint8_t SDMTPClass::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
{
  // Lets map the user_token back to oint to our object...
  Serial.printf("Format Callback: user_token:%x store: %u p2:%u post:%u \n", user_token, store, p2, post_process);
  SDClass *psd = (SDClass*)user_token;

  if (psd->sdfs.fatType() == FAT_TYPE_FAT12) {
    Serial.printf("    Fat12 not supported\n");
    return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  }

  // For all of these the fat ones will do on post_process
  if (!post_process) return MTPStorageInterfaceCB::FORMAT_NEEDS_CALLBACK;

  PFsVolume partVol;
  if (!partVol.begin(psd->sdfs.card(), true, 1)) return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  bool success = pfsLIB.formatter(partVol);
  return success ? MTPStorageInterfaceCB::FORMAT_SUCCESSFUL : MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
}

//===================================================
// SD Testing for disk insertion.
#if defined(ARDUINO_TEENSY41)
#define _SD_DAT3 46
#elif defined(ARDUINO_TEENSY40)
#define _SD_DAT3 38
#elif defined(ARDUINO_TEENSY_MICROMOD)
#define _SD_DAT3 38
#elif defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
#define _SD_DAT3 62
#endif


bool  SDMTPClass::init(bool add_if_missing) {
#ifdef BUILTIN_SDCARD
  if (csPin_ == BUILTIN_SDCARD) {
    if (!sdfs.begin(SdioConfig(FIFO_SDIO))) {
      if (!add_if_missing) return false; // not here and don't add...
#ifdef _SD_DAT3
      cdPin_ = _SD_DAT3;
      pinMode(_SD_DAT3, INPUT_PULLDOWN);
      check_disk_insertion_ = true;
      return true;
#else
      return false;
#endif
    }
  }
  else
#endif
  {
    Serial.printf("Trying to open SPI config: %u %u %u %u\n", csPin_, cdPin_, opt_, maxSpeed_);
    if (!sdfs.begin(SdSpiConfig(csPin_, opt_, maxSpeed_, port_))) {
      Serial.println("    Failed to open");
      if (!add_if_missing || (cdPin_ == 0xff)) return false; // Not found and no add or not detect
      pinMode(cdPin_, INPUT_PULLDOWN);
      check_disk_insertion_ = true;
      return true;
    }
  }
  addFSToStorage(false); // do the work to add us to the storage list.
  return true;
}


void SDMTPClass::loop() {
  if (check_disk_insertion_)
  {
    // delayMicroseconds(5);
    if (digitalRead(cdPin_))
    {
      // looks like SD Inserted. so disable the pin for now...
      // BUGBUG for SPI ones with extra IO pins can do more...
      pinMode(cdPin_, INPUT_DISABLE);
      check_disk_insertion_ = false; // only try this once

      delay(1);
      Serial.printf("\n*** SD Card Inserted ***");
      #ifdef BUILTIN_SDCARD
      if (csPin_ == BUILTIN_SDCARD) {
        if (!sdfs.begin(SdioConfig(FIFO_SDIO))) {
          #ifdef _SD_DAT3
          //pinMode(_SD_DAT3, INPUT_PULLDOWN);  // it failed try to reinit again
          #endif
          return; // bail
        }
      }
      else
      #endif
      {
        Serial.printf("Trying to open SPI config: %u %u %u %u\n", csPin_, cdPin_, opt_, maxSpeed_);
        if (sdfs.begin(SdSpiConfig(csPin_, opt_, maxSpeed_, port_))) {
          Serial.println("    Failed to open");
          // pinMode(cdPin_, INPUT_PULLDOWN);  // BUGBUG retury?  But first probably only when other removed?
          return;
        }
      }
      addFSToStorage(true); // do the work to add us to the storage list.
    }
  }
}

void SDMTPClass::addFSToStorage(bool send_events)
{
  // The SD is valid now...
  uint32_t store = storage_.getStoreID(sdc_name_);
  if (store != 0xFFFFFFFFUL)
  {
    if(send_events) mtpd_.send_StoreRemovedEvent(store);
    delay(50);
    //mtpd_.send_StorageInfoChangedEvent(store);
    if(send_events) mtpd_.send_StoreAddedEvent(store);

  } else {
    // not in our list, try adding it
    Serial.println("addFSToStorage: Added FS"); 
    store = storage_.addFilesystem(*this, sdc_name_, this, (uint32_t)(void*)this);
    if(send_events) mtpd_.send_StoreAddedEvent(store);
  }


  uint64_t ts = totalSize();
  uint64_t us  = usedSize();
  Serial.print("Total Size: "); Serial.print(ts);
  Serial.print(" Used Size: "); Serial.println(us);

}
