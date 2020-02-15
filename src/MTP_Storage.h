// MTP_Storage.h - Teensy MTP Responder Storage library
// Copyright (C) 2020 Walter Zimmer <walter@wmxz.eu>
// based on
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
// modified for T4 by WMXZ

#ifndef MTP_STORAGE_H
#define MTP_STORAGE_H

#include "core_pins.h"

#include "MTP_config.h"

extern SdFs SD;

// TODO:
//   support multiple storages
//   support serialflash
//   partial object fetch/receive
//   events (notify usb host when local storage changes)

// This interface lets the MTP responder interface any storage.
// We'll need to give the MTP responder a pointer to one of these.
class MTPStorageInterface {
public:
  // Return true if this storage is read-only
  virtual bool readonly() = 0;

  // Does it have directories?
  virtual bool has_directories() = 0;

  // Return size of storage in bytes.
  virtual uint64_t size() = 0;

  // Return free space in bytes.
  virtual uint64_t free() = 0;

  // parent = 0 means get all handles.
  // parent = 0xFFFFFFFF means get root folder.
  virtual void StartGetObjectHandles(uint32_t parent) = 0;
  virtual uint32_t GetNextObjectHandle() = 0;

  // Size should be 0xFFFFFFFF if it's a directory.
  virtual void GetObjectInfo(uint32_t handle,  char* name, uint32_t *dir, uint32_t* size, uint32_t* parent) = 0;
  virtual void SetObjectInfo(uint32_t handle, char* name, uint32_t dir, uint32_t size, uint32_t parent) = 0;

  virtual uint64_t GetSize(uint32_t handle) = 0;
  virtual void read(uint32_t handle, uint32_t pos, char* buffer, uint32_t bytes) = 0;
  virtual uint32_t Create(uint32_t parent, bool folder, const char* filename) = 0;
  virtual void write(const char* data, uint32_t size);
  virtual void close();
  virtual bool DeleteObject(uint32_t object) = 0;

  virtual void rename(uint32_t handle, const char* newName) = 0 ;
  virtual void move(uint32_t handle, uint32_t newParent ) = 0 ;

  virtual void ResetIndex(void ) = 0 ;

};

typedef struct {
    uint32_t parent;
    uint32_t child; 
    uint32_t sibling;
    uint8_t isdir;
    uint8_t scanned;
    uint64_t size;
    char name[80];
  }  Record;


// Storage implementation for SD. SD needs to be already initialized.
class MTPStorage_SD : public MTPStorageInterface {
public:
  void init(void);

private:
#if USE_SDFAT_BETA==1
  FsFile index_;
  FsFile f_;
#else
  File index_;
  File f_;
#endif

  uint16_t mode_ = 0;
  uint32_t open_file_ = 0xFFFFFFFEUL;
  uint32_t index_entries_ = 0;

  bool readonly() ;
  bool has_directories() ;
  uint64_t size() ;
  uint64_t free() ;

  void OpenIndex() ;
  void WriteIndexRecord(uint32_t i, const Record& r) ;
  uint32_t AppendIndexRecord(const Record& r) ;
  // TODO(hubbe): Cache a few records for speed.
  Record ReadIndexRecord(uint32_t i) ;
  void ConstructFilename(int i, char* out) ;
  void OpenFileByIndex(uint32_t i, oflag_t mode = O_RDONLY) ;

  // MTP object handles should not change or be re-used during a session.
  // This would be easy if we could just have a list of all files in memory.
  // Since our RAM is limited, we'll keep the index in a file instead.
  bool index_generated = false;
  void GenerateIndex() ;
  void ScanDir(uint32_t i) ;

  bool all_scanned_ = false;
  void ScanAll() ;

  uint32_t next_;
  bool follow_sibling_;
  void StartGetObjectHandles(uint32_t parent) override ;
  uint32_t GetNextObjectHandle() override ; 
  void GetObjectInfo(uint32_t handle, char* name, uint32_t *dir, uint32_t* size, uint32_t* parent) override ;
  void SetObjectInfo(uint32_t handle, char* name, uint32_t dir, uint32_t size, uint32_t parent) override ;
  uint64_t GetSize(uint32_t handle) ;
  void read(uint32_t handle, uint32_t pos, char* out, uint32_t bytes) override ;
  bool DeleteObject(uint32_t object) override ;
  uint32_t Create(uint32_t parent,  bool folder, const char* filename) override ;
  void write(const char* data, uint32_t bytes) override ;
  void close() override ;

  void rename(uint32_t handle, const char* newName) override ;
  void move(uint32_t handle, uint32_t newParent ) override ;

  void ResetIndex(void ) override ;

};

#endif
