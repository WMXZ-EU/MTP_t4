#ifndef SD_MTP_CALLBACK_H
#define SD_MTP_CALLBACK_H

#include <SD.h>
#include <mscFS.h>
#include <MTP.h>

// Setup a callback class for Littlefs storages..
// Plus helper functions for handling SD disk insertions. 
class SD_MTP_CB : public MTPStorageInterfaceCB
{
public:
  // constructor
  SD_MTP_CB(MTPD &mtpd, MTPStorage_SD &storage) : mtpd_(mtpd), storage_(storage) {};

  // Callback function overrides.
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);

  // Support functions for SD Insertions.
  bool installSDIOInsertionDetection(SDClass *sdc, const char *sdc_name, int storage_index);
  void checkSDStatus();

private:
  MTPD &mtpd_;
  MTPStorage_SD &storage_;

  // Currently SDIO only, may add. SPI ones later
  bool check_sdio_ = false;
  SDClass *sdc_sdio_ = nullptr;
  const char *sdc_sdio_name_ = nullptr;
  int sdc_sdio_storage_index_ = -1;
};

#endif
