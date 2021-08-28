#ifndef SD_MTP_CALLBACK_H
#define SD_MTP_CALLBACK_H

#include <SD.h>
#include <mscFS.h>
#include <MTP.h>

// Setup a callback class for Littlefs storages..
class SD_MTP_CB : public MTPStorageInterfaceCB, public MTPStorage_SD
{
public:
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
};

#endif
