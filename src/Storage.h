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

#include "SD.h"
extern SDClass sdx[];

// This interface lets the MTP responder interface any storage.
// We'll need to give the MTP responder a pointer to one of these.
class MTPStorageInterface {
public:
  virtual void setStorageNumbers(const char **sd_str, int num) =0;

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
class MTPStorage_SD : public MTPStorageInterface 
{
public:
  void setStorageNumbers(const char **sd_str, int num) override;

private:
  File index_;
  File file_;
  File child_;

  int num_storage;
  const char **sd_str = 0;

  uint32_t mode_ = 0;
  uint32_t open_file_ = 0xFFFFFFFEUL;

  uint32_t index_entries_ = 0;
  bool index_generated = false;
  void printIndexList(void);

  bool readonly(uint32_t storage);
  bool has_directories(uint32_t storage) ;
  
  uint32_t clusterCount(uint32_t storage) ;
  uint32_t freeClusters(uint32_t storage) ;
  uint32_t clusterSize(uint32_t storage) ;

  void CloseIndex() ;
  void OpenIndex() ;
  void WriteIndexRecord(uint32_t i, const Record& r) ;
  uint32_t AppendIndexRecord(const Record& r) ;
  Record ReadIndexRecord(uint32_t i) ;
  uint16_t ConstructFilename(int i, char* out, int len) ;
  void OpenFileByIndex(uint32_t i, uint32_t mode = O_RDONLY) ;

  bool all_scanned_ = false;
  
  void GenerateIndex(uint32_t storage) ;
  void ScanDir(uint32_t storage, uint32_t i) ;
  
  void ScanAll(uint32_t storage) ;

  uint32_t next_;
  bool follow_sibling_;

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
