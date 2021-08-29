#include <SD_MTP_Callback.h>
#include <mscFS.h>



//=============================================================================
// try to get the right FS for this store and then call it's format if we have one...
// Here is for LittleFS...
PFsLib pfsLIB;
uint8_t SD_MTP_CB::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
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
#elif defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD)
  #define _SD_DAT3 38
#elif defined(ARDUINO_TEENSY35) || defined(ARDUINO_TEENSY36)
  #define _SD_DAT3 62
#endif


bool  SD_MTP_CB::installSDIOInsertionDetection(SDClass *sdc, const char *sdc_name, int storage_index) {
#ifdef _SD_DAT3
  sdc_sdio_ = sdc;
  sdc_sdio_name_ = sdc_name;
  sdc_sdio_storage_index_ = storage_index;

  pinMode(_SD_DAT3, INPUT_PULLDOWN);
  check_sdio_ = true;
  return true;    
#else
  return false;    
#endif
}


void SD_MTP_CB::checkSDStatus() {
#ifdef _SD_DAT3
  if (check_sdio_)
  {
   // delayMicroseconds(5);
    bool r = digitalReadFast(_SD_DAT3);
    if (r)
    {
      // looks like SD Inserted. so disable the pin for now...
      pinMode(_SD_DAT3, INPUT_DISABLE);

      delay(1);
      Serial.printf("\n*** SDIO Card Inserted ***");
      if(!sdc_sdio_->sdfs.begin(SdioConfig(FIFO_SDIO)))
      { 
        Serial.println("SDIO Storage, inserted card failed or missing"); 
      }
      else
      {
        // The SD is valid now... 
        uint32_t store = storage_.getStoreID(sdc_sdio_name_);
        if (store != 0xFFFFFFFFUL) 
        {
          mtpd_.send_StoreRemovedEvent(store);
          delay(50);
          //mtpd_.send_StorageInfoChangedEvent(store);
          mtpd_.send_StoreAddedEvent(store);

        } else {
          // not in our list, try adding it
          store = storage_.addFilesystem(*sdc_sdio_, sdc_sdio_name_, this, (uint32_t)(void*)sdc_sdio_);
          mtpd_.send_StoreAddedEvent(store);
        }


        uint64_t totalSize = sdc_sdio_->totalSize();
        uint64_t usedSize  = sdc_sdio_->usedSize();
        Serial.print("Total Size: ");Serial.print(totalSize); 
        Serial.print(" Used Size: "); Serial.println(usedSize);

      }
      check_sdio_ = false; // only try this once
    }
  }
  #endif // SD_DAT3
}
