// Storage.h - Teensy MTP Responder library
// Copyright (C) 2017 Fredrik Hubinette <hubbe@hubbe.net>
//
// With updates from MichaelMC and Yoong Hor Meng <yoonghm@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// modified for SDFS by WMXZ
// Nov 2020 adapted to SdFat-beta / SD combo

#ifndef Storage_H
#define Storage_H

#include "core_pins.h"

#define HAVE_LITTLEFS 1 // set to zero if no LtttleFS is existing or to be used

#include "SD.h"
#ifndef FILE_WRITE_BEGIN
  #define FILE_WRITE_BEGIN 2
#endif

// following is a device specific base class for storage classs
extern SDClass sdx[];


#if HAVE_LITTLEFS==1
#include "LittleFS.h"
extern LittleFS_RAM ramfs;
#endif

class mSD_Base
{ 
  public:
  File sd_open(uint32_t store, const char *filename, uint32_t mode) 
  { if(!cs || (cs[store]<256)) return sdx[store].open(filename,mode); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256) return ramfs.open(filename,mode);
    #endif
    else return 0;
  }
  bool sd_mkdir(uint32_t store, char *filename) 
  { if(!cs || (cs[store]<256)) return sdx[store].mkdir(filename); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return ramfs.mkdir(filename);
    #endif
    else return false;
  }

  bool sd_rename(uint32_t store, char *oldfilename, char *newfilename) 
  { if(!cs || (cs[store]<256)) return sdx[store].rename(oldfilename,newfilename); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return ramfs.rename(oldfilename,newfilename);
    #endif
    else return false;
  }

  bool sd_remove(uint32_t store, const char *filename) 
  { if(!cs || (cs[store]<256)) return sdx[store].remove(filename); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return ramfs.remove(filename);
    #endif
    else return false;
  }
  bool sd_rmdir(uint32_t store, char *filename) 
  { if(!cs || (cs[store]<256)) return sdx[store].rmdir(filename); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return ramfs.rmdir(filename);
    #endif
    else return false;
  }
    
  uint32_t sd_totalClusterCount(uint32_t store) 
  { if(!cs || (cs[store]<256)) return sdx[store].sdfs.clusterCount(); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return ramfs.totalSize()/512; 
    #endif
    else return 0;
 }
  uint32_t sd_freeClusterCount(uint32_t store)  
  { if(!cs || (cs[store]<256)) return sdx[store].sdfs.freeClusterCount(); 
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return (ramfs.totalSize()-ramfs.usedSize())/512; 
    #endif
    else return 0;
  }
  uint32_t sd_sectorsPerCluster(uint32_t store) 
  { if(!cs || (cs[store]<256)) return sdx[store].sdfs.sectorsPerCluster();
    #if HAVE_LITTLEFS==1
    else if(cs[store]==256)  return 1;
    #endif
    else return 0;
  }

  bool setCs(const int *csx) { cs = csx; return true; }

  private:
  const int * cs = 0;
};

// This interface lets the MTP responder interface any storage.
// We'll need to give the MTP responder a pointer to one of these.
class MTPStorageInterface {
public:
  virtual void setStorageNumbers(const char **sd_str, const int *cs, int num) =0;

  // Return true if this storage is read-only
  virtual bool readonly(uint32_t storage) = 0;

  // Does it have directories?
  virtual bool has_directories(uint32_t storage) = 0;

  virtual uint32_t clusterCount(uint32_t storage) = 0;
  virtual uint32_t freeClusters(uint32_t storage) = 0;
  virtual uint32_t clusterSize(uint32_t storage) = 0;

  virtual uint32_t getNumStorage() = 0;
  virtual const char * getStorageName(uint32_t storage) = 0;
  // parent = 0 means get all handles.
  // parent = 0xFFFFFFFF means get root folder.
  virtual void StartGetObjectHandles(uint32_t storage, uint32_t parent) = 0;
  virtual uint32_t GetNextObjectHandle(uint32_t  storage) = 0;

  // Size should be 0xFFFFFFFF if it's a directory.
  virtual void GetObjectInfo(uint32_t handle, char* name, uint32_t* size, uint32_t* parent, uint16_t *store) = 0;
  virtual uint32_t GetSize(uint32_t handle) = 0;

  virtual uint32_t Create(uint32_t storage, uint32_t parent, bool folder, const char* filename) = 0;
  virtual void read(uint32_t handle, uint32_t pos, char* buffer, uint32_t bytes) = 0;
  virtual void write(const char* data, uint32_t size);
  virtual void close() = 0;
  virtual bool DeleteObject(uint32_t object) = 0;
  virtual void CloseIndex() = 0;

  virtual void ResetIndex() = 0;
  virtual bool rename(uint32_t handle, const char* name) = 0 ;
  virtual bool move(uint32_t handle, uint32_t newStore, uint32_t newParent ) = 0 ;
};

  struct Record 
  { uint32_t parent;
    uint32_t child;  // size stored here for files
    uint32_t sibling;
    uint8_t isdir;
    uint8_t scanned;
    uint16_t store;  // index int physical storage (0 ... num_storages-1)
    char name[64];
  };

  void mtp_yield(void);


// Storage implementation for SD. SD needs to be already initialized.
class MTPStorage_SD : public MTPStorageInterface, mSD_Base
{
public:
  void setStorageNumbers(const char **sd_str, const int *cs, int num) override;

private:
  File index_;
  File file_;
  File child_;

  int num_storage = 0;
  const char **sd_str = 0;
  
  uint32_t mode_ = 0;
  uint32_t open_file_ = 0xFFFFFFFEUL;

  bool readonly(uint32_t storage);
  bool has_directories(uint32_t storage) ;
  
  uint32_t clusterCount(uint32_t storage) ;
  uint32_t freeClusters(uint32_t storage) ;
  uint32_t clusterSize(uint32_t storage) ;

  void CloseIndex() ;
  void OpenIndex() ;
  void GenerateIndex(uint32_t storage) ;
  void ScanDir(uint32_t storage, uint32_t i) ;
  void ScanAll(uint32_t storage) ;

  uint32_t index_entries_ = 0;
  bool index_generated = false;

  bool all_scanned_ = false;
  uint32_t next_;
  bool follow_sibling_;

  void WriteIndexRecord(uint32_t i, const Record& r) ;
  uint32_t AppendIndexRecord(const Record& r) ;
  Record ReadIndexRecord(uint32_t i) ;
  uint16_t ConstructFilename(int i, char* out, int len) ;
  void OpenFileByIndex(uint32_t i, uint32_t mode = FILE_READ) ;
  void dumpIndexList(void);
  void printRecord(int h, Record *p);

  uint32_t getNumStorage() override;
  const char * getStorageName(uint32_t storage) override;
  void StartGetObjectHandles(uint32_t storage, uint32_t parent) override ;
  uint32_t GetNextObjectHandle(uint32_t  storage) override ;
  void GetObjectInfo(uint32_t handle, char* name, uint32_t* size, uint32_t* parent, uint16_t *store) override ;
  uint32_t GetSize(uint32_t handle) override;
  void read(uint32_t handle, uint32_t pos, char* out, uint32_t bytes) override ;
  bool DeleteObject(uint32_t object) override ;
  uint32_t Create(uint32_t storage, uint32_t parent,  bool folder, const char* filename) override ;

  void write(const char* data, uint32_t bytes) override ;
  void close() override ;

  bool rename(uint32_t handle, const char* name) override ;
  bool move(uint32_t handle, uint32_t newStore, uint32_t newParent ) override ;
  
  void ResetIndex() override ;
};

#endif
