#ifndef LFS_MTP_CALLBACK_H
#define LFS_MTP_CALLBACK_H

#include <LittleFS.h>
#include <MTP.h>

// Setup a callback class for Littlefs storages..
class LittleFSMTPCB : public MTPStorageInterfaceCB, public MTPStorage_SD
{
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
};

#endif
