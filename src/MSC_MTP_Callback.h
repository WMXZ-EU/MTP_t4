#ifndef MSC_MTP_CALLBACK_H
#define MSC_MTP_CALLBACK_H

#include <mscFS.h>
#include <MTP.h>


class MSC_MTP_CB : public MTPStorageInterfaceCB 
{
  uint8_t formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process);
  uint64_t usedSizeCB(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token);
};

#endif