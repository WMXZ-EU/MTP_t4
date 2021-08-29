#include <MSC_MTP_Callback.h>
#include <mscFS.h>


PFsLib pfsLIB1;
uint8_t MSC_MTP_CB::formatStore(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token, uint32_t p2, bool post_process)
{
  // Lets map the user_token back to oint to our object...
  Serial.printf("Format Callback: user_token:%x store: %u p2:%u post:%u \n", user_token, store, p2, post_process);
  
  MSCClass *pmsc = (MSCClass*)user_token;

  if (pmsc->mscfs.fatType() == FAT_TYPE_FAT12) {
    Serial.printf("    Fat12 not supported\n");  
    return MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
  }

  // For all of these the fat ones will do on post_process
  if (!post_process) return MTPStorageInterfaceCB::FORMAT_NEEDS_CALLBACK;

  bool success = pfsLIB1.formatter(pmsc->mscfs); 

  return success ? MTPStorageInterfaceCB::FORMAT_SUCCESSFUL : MTPStorageInterfaceCB::FORMAT_NOT_SUPPORTED;
}

uint64_t MSC_MTP_CB::usedSizeCB(MTPStorage_SD *mtpstorage, uint32_t store, uint32_t user_token)
{

  Serial.printf("\n\n}}}}}}}}} MSCMTPCB::usedSizeCB called %x %u %u\n", (uint32_t)mtpstorage, store, user_token);
  
  MSCClass *pmsc = (MSCClass*)user_token;

  if (pmsc->mscfs.fatType() == FAT_TYPE_FAT32) {
	  Serial.printf("MSCMTPCB::usedSizeCB called for Fat32\n");  
  }
  
  return pmsc->usedSize();
}