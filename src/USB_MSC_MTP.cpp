#include <USB_MSC_MTP.h>
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);



// start off with one of these...
#include <MSC_MTP_Callback.h>
MSC_MTP_CB mscmtpcb;

// start off with one controller. 
msController msDrive[USE_MSC_FAT](myusb);
bool msDrive_previous[USE_MSC_FAT]; // Was this drive there the previous time through?
MSCClass msc[USE_MSC_FAT_VOL];
char  nmsc_str[USE_MSC_FAT_VOL][20];

uint16_t msc_storage_index[USE_MSC_FAT_VOL];
uint8_t msc_drive_index[USE_MSC_FAT_VOL]; // probably can find easy way not to need this.


void USB_MSC_MTP::begin() {
	myusb.begin();
}

//Checks if a Card is present:
//- only when it is not in use! - 
bool USB_MSC_MTP::mbrDmp(msController *pdrv) {
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

void USB_MSC_MTP::checkUSB(MTPStorage_SD *mtpstorage, bool fInit) {
  MTPD    mtpd(mtpstorage);
	
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
                msc_storage_index[index_msc] = mtpstorage->addFilesystem(msc[index_msc], nmsc_str[index_msc], &mscmtpcb, (uint32_t)(void*)&msc[index_msc]);
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
            mtpstorage->removeFilesystem(msc_storage_index[index_msc]);
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
}



void USB_MSC_MTP::dump_hexbytes(const void *ptr, int len)
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
