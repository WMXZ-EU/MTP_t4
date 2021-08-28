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
