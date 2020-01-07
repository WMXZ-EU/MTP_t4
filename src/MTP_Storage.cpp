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


#include "core_pins.h"
#include "usb_dev.h"
//#include "usb_serial.h"
#include "MTP_Storage.h"

#include "SdFat-beta.h"

SdFs SD;

// TODO:
//   support multiple storages
//   support serialflash
//   partial object fetch/receive
//   events (notify usb host when local storage changes)

// These should probably be weak.
void mtp_yield() {}
void mtp_lock_storage(bool lock) { }

#include "TimeLib.h"
 // Call back for file timestamps.  Only called for file create and sync().
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
{ 
  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(year(), month(), day());

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(hour(), minute(), second());
  
  // Return low time bits in units of 10 ms.
  *ms10 = second() & 1 ? 100 : 0;
}

      void MTPStorage_SD::init(void)
      { 
        #if DO_DEBUG>0
          Serial.println("Using SdFS");
        #endif
        #if USE_SDI0==0
          SPI.setMOSI(SD_MOSI);
          SPI.setMISO(SD_MISO);
          SPI.setSCK(SD_SCK);
        #endif
        if (!SD.begin(SD_CONFIG)) SD.errorHalt("SD.begin failed");
      
        // Set Time callback
        FsDateTime::callback = dateTime;
      }

  bool MTPStorage_SD::readonly() { return false; }
  bool MTPStorage_SD::has_directories() { return true; }
  uint64_t MTPStorage_SD::size(){
     return (uint64_t)512 * (uint64_t)SD.clusterCount() * (uint64_t)SD.sectorsPerCluster();
  }

  uint64_t MTPStorage_SD::free() {
    return (uint64_t)512 * (uint64_t)SD.freeClusterCount() * (uint64_t)SD.sectorsPerCluster(); 
  }

  void MTPStorage_SD::OpenIndex() {
    if (index_) return;
    mtp_lock_storage(true);
    index_ = SD.open("mtpindex.dat", FILE_WRITE);
    mtp_lock_storage(false);
  }

  void MTPStorage_SD::WriteIndexRecord(uint32_t i, const Record& r) {
    OpenIndex();
    mtp_lock_storage(true);
    index_.seek(sizeof(r) * i);
    index_.write((char*)&r, sizeof(r));
    mtp_lock_storage(false);
  }

  uint32_t MTPStorage_SD::AppendIndexRecord(const Record& r) {
    uint32_t new_record = index_entries_++;
    WriteIndexRecord(new_record, r);
    return new_record;
  }

  // TODO(hubbe): Cache a few records for speed.
  Record MTPStorage_SD::ReadIndexRecord(uint32_t i) {
    Record ret;
    if (i > index_entries_) {
      memset(&ret, 0, sizeof(ret));
      return ret;
    }
    OpenIndex();
    mtp_lock_storage(true);
    index_.seek(sizeof(ret) * i);
    index_.read(&ret, sizeof(ret));
    mtp_lock_storage(false);
    return ret;
  }

  void MTPStorage_SD::ConstructFilename(int i, char* out) {
    if (i == 0) {
      strcpy(out, "/");
    } else {
      Record tmp = ReadIndexRecord(i);
      ConstructFilename(tmp.parent, out);
      if (out[strlen(out)-1] != '/')
        strcat(out, "/");
      strcat(out, tmp.name);
    }
  }

  void MTPStorage_SD::OpenFileByIndex(uint32_t i, uint8_t mode) {
    if (open_file_ == i && mode_ == mode)
      return;
    char filename[256];
    ConstructFilename(i, filename);
    mtp_lock_storage(true);
    f_.close();
    f_ = SD.open(filename, mode);
    open_file_ = i;
    mode_ = mode;
    mtp_lock_storage(false);
  }

  // MTP object handles should not change or be re-used during a session.
  // This would be easy if we could just have a list of all files in memory.
  // Since our RAM is limited, we'll keep the index in a file instead.
  void MTPStorage_SD::GenerateIndex() {
    if (index_generated) return;
    index_generated = true;

    mtp_lock_storage(true);
    SD.remove("mtpindex.dat");
    mtp_lock_storage(false);
    index_entries_ = 0;

    Record r;
    r.parent = 0;
    r.sibling = 0;
    r.child = 0;
    r.isdir = true;
    r.scanned = false;
    strcpy(r.name, "/");
    AppendIndexRecord(r);
  }

  void MTPStorage_SD::ScanDir(uint32_t i) {
    Record record = ReadIndexRecord(i);
    if (record.isdir && !record.scanned) {
      OpenFileByIndex(i);
      if (!f_) return;
      int sibling = 0;
      while (true) {
        mtp_lock_storage(true);
        File child = f_.openNextFile();
        mtp_lock_storage(false);

        if (!child) break;

        Record r;
        r.parent = i;
        r.sibling = sibling;
        r.isdir = child.isDirectory();
        r.child = r.isdir ? 0 : child.size();
        r.scanned = false;
        child.getName(r.name, 64);
        sibling = AppendIndexRecord(r);
        child.close();
      }
      record.scanned = true;
      record.child = sibling;
      WriteIndexRecord(i, record);
    }
  }

  void MTPStorage_SD::ScanAll() {
    if (all_scanned_) return;
    all_scanned_ = true;

    GenerateIndex();
    for (uint32_t i = 0; i < index_entries_; i++) {
      ScanDir(i);
    }
  }

  void MTPStorage_SD::StartGetObjectHandles(uint32_t parent) {
    GenerateIndex();
    if (parent) {
      if (parent == 0xFFFFFFFF) parent = 0;

      ScanDir(parent);
      follow_sibling_ = true;
      // Root folder?
      next_ = ReadIndexRecord(parent).child;
    } else {
      ScanAll();
      follow_sibling_ = false;
      next_ = 1;
    }
  }

  uint32_t MTPStorage_SD::GetNextObjectHandle() {
    while (true) {
      if (next_ == 0) return 0;

      int ret = next_;
      Record r = ReadIndexRecord(ret);
      if (follow_sibling_) {
        next_ = r.sibling;
      } else {
        next_++;
        if (next_ >= index_entries_)
          next_ = 0;
      }
      if (r.name[0]) return ret;
    }
  }

  void MTPStorage_SD::GetObjectInfo(uint32_t handle, char* name, uint32_t* size, uint32_t* parent) {
    Record r = ReadIndexRecord(handle);
    strcpy(name, r.name);
    *parent = r.parent;
    *size = r.isdir ? 0xFFFFFFFFUL : r.child;
  }

  uint32_t MTPStorage_SD::GetSize(uint32_t handle) {
    return ReadIndexRecord(handle).child;
  }

  void MTPStorage_SD::read(uint32_t handle, uint32_t pos, char* out, uint32_t bytes) {
    OpenFileByIndex(handle);
    mtp_lock_storage(true);
    f_.seek(pos);
    f_.read(out, bytes);
    mtp_lock_storage(false);
  }

  bool MTPStorage_SD::DeleteObject(uint32_t object) {
    char filename[256];
    Record r;
    while (true) {
      r = ReadIndexRecord(object == 0xFFFFFFFFUL ? 0 : object);
      if (!r.isdir) break;
      if (!r.child) break;
      if (!DeleteObject(r.child))
        return false;
    }

    // We can't actually delete the root folder,
    // but if we deleted everything else, return true.
    if (object == 0xFFFFFFFFUL) return true;

    ConstructFilename(object, filename);
    bool success;
    mtp_lock_storage(true);
    if (r.isdir) {
      success = SD.rmdir(filename);
    } else {
      success = SD.remove(filename);
    }
    mtp_lock_storage(false);
    if (!success) return false;
    r.name[0] = 0;
    int p = r.parent;
    WriteIndexRecord(object, r);
    Record tmp = ReadIndexRecord(p);
    if (tmp.child == object) {
      tmp.child = r.sibling;
      WriteIndexRecord(p, tmp);
    } else {
      int c = tmp.child;
      while (c) {
  tmp = ReadIndexRecord(c);
  if (tmp.sibling == object) {
    tmp.sibling = r.sibling;
    WriteIndexRecord(c, tmp);
    break;
  } else {
    c = tmp.sibling;
  }
      }
    }
    return true;
  }

  uint32_t MTPStorage_SD::Create(uint32_t parent, bool folder, const char* filename) {
    uint32_t ret;
    if (parent == 0xFFFFFFFFUL) parent = 0;
    Record p = ReadIndexRecord(parent);
    Record r;
    if (strlen(filename) > 62) return 0;
    strcpy(r.name, filename);
    r.parent = parent;
    r.child = 0;
    r.sibling = p.child;
    r.isdir = folder;
    // New folder is empty, scanned = true.
    r.scanned = 1;
    ret = p.child = AppendIndexRecord(r);
    WriteIndexRecord(parent, p);
    if (folder) {
      char filename[256];
      ConstructFilename(ret, filename);
      mtp_lock_storage(true);
      SD.mkdir(filename);
      mtp_lock_storage(false);
    } else {
      OpenFileByIndex(ret, FILE_WRITE);
    }
    return ret;
  }

  void MTPStorage_SD::write(const char* data, uint32_t bytes) {
    mtp_lock_storage(true);
    f_.write(data, bytes);
    mtp_lock_storage(false);
  }

  void MTPStorage_SD::close() {
    mtp_lock_storage(true);
    uint64_t size = f_.size();
    f_.close();
    mtp_lock_storage(false);
    Record r = ReadIndexRecord(open_file_);
    r.child = size;
    WriteIndexRecord(open_file_, r);
    open_file_ = 0xFFFFFFFEUL;
  }


#if 0

#include "mfs.h"
extern mFS_class mFS; // should be declared in main

// TODO:
//   support multiple storages
//   support serialflash
//   partial object fetch/receive
//   events (notify usb host when local storage changes)

// These should probably be weak.
void mtp_yield() {}
void mtp_lock_storage(bool lock) { }

  bool MTPStorage_SD::readonly() { return false; }
  bool MTPStorage_SD::has_directories() { return true; }
  
  uint64_t MTPStorage_SD::size() {return mFS.SD_size();}
  uint64_t MTPStorage_SD::free() { return mFS.SD_free();}

  void MTPStorage_SD::CloseIndex()
  {
    mtp_lock_storage(true);
    mFS.indexClose();
    mtp_lock_storage(false);
    index_generated = false;
    index_entries_ = 0;
  }

  void MTPStorage_SD::OpenIndex() 
  { if(mFS.index()) return; // only once
    mtp_lock_storage(true);
    mFS.indexOpen((char*)"mtpindex.dat", FILE_WRITE);
    mtp_lock_storage(false);
  }

  void MTPStorage_SD::WriteIndexRecord(uint32_t i, const Record& r) 
  {
    OpenIndex();
    mtp_lock_storage(true);
    mFS.indexSeek(sizeof(r) * i);
    mFS.indexWrite((char*)&r, sizeof(r));
    mtp_lock_storage(false);
  }

  uint32_t MTPStorage_SD::AppendIndexRecord(const Record& r) 
  {
    uint32_t new_record = index_entries_++;
    WriteIndexRecord(new_record, r);
    return new_record;
  }

  // TODO(hubbe): Cache a few records for speed.
  Record MTPStorage_SD::ReadIndexRecord(uint32_t i) 
  {
    Record ret;
    if (i > index_entries_) 
    { memset(&ret, 0, sizeof(ret));
      return ret;
    }
    OpenIndex();
    mtp_lock_storage(true);
    mFS.indexSeek(sizeof(ret) * i);
    mFS.indexRead((char *)&ret, sizeof(ret));
    mtp_lock_storage(false);
    return ret;
  }

  void MTPStorage_SD::ConstructFilename(int i, char* out, int len) // construct filename rexursively
  {
    if (i == 0) 
    { strcpy(out, "/");
    }
    else 
    { Record tmp = ReadIndexRecord(i);
      ConstructFilename(tmp.parent, out, len);
      if (out[strlen(out)-1] != '/') strcat(out, "/");
      if(((strlen(out)+strlen(tmp.name)+1) < (unsigned) len)) strcat(out, tmp.name);
    }
  }

  void MTPStorage_SD::OpenFileByIndex(uint32_t i, uint32_t mode ) 
  {
    if (open_file_ == i && mode_ == mode) return;
    char filename[256];
    ConstructFilename(i, filename, 256);
    mtp_lock_storage(true);
    if(mFS.file()) mFS.fileClose();
    mFS.fileOpen(filename,mode);
    open_file_ = i;
    mode_ = mode;
    mtp_lock_storage(false);
  }

  // MTP object handles should not change or be re-used during a session.
  // This would be easy if we could just have a list of all files in memory.
  // Since our RAM is limited, we'll keep the index in a file instead.
  void MTPStorage_SD::GenerateIndex()
  {
    if (index_generated) return;
    index_generated = true;

    // first remove old index file
    mtp_lock_storage(true);
    mFS.remove((char*)"mtpindex.dat");
    mtp_lock_storage(false);
    index_entries_ = 0;

    Record r;
    r.parent = 0;
    r.sibling = 0;
    r.child = 0;
    r.isdir = true;
    r.scanned = false;
    strcpy(r.name, "/");
    AppendIndexRecord(r);
  }

  void MTPStorage_SD::ScanDir(uint32_t i) 
  {
    Record record = ReadIndexRecord(i);
    if (record.isdir && !record.scanned) {
      OpenFileByIndex(i);
      if (!mFS.file()) return;
      int sibling = 0;
      while (true) 
      {
        mtp_lock_storage(true);
        mFS.childOpenNextFile();
        mtp_lock_storage(false);
        
        if(!mFS.child()) break;

        Record r;
        r.parent = i;
        r.sibling = sibling;
        r.isdir = mFS.childIsDirectory();
        r.child = r.isdir ? 0 : mFS.childSize();
        r.scanned = false;
        mFS.childGetName(r.name, 64);
        sibling = AppendIndexRecord(r);
        mFS.childClose();
      }
      record.scanned = true;
      record.child = sibling;
      WriteIndexRecord(i, record);
    }
  }

  void MTPStorage_SD::ScanAll() 
  {
    if (all_scanned_) return;
    all_scanned_ = true;

    GenerateIndex();
    for (uint32_t i = 0; i < index_entries_; i++)  ScanDir(i);
  }

  void MTPStorage_SD::StartGetObjectHandles(uint32_t parent) 
  {
    GenerateIndex();
    if (parent) 
    { if (parent == 0xFFFFFFFF) parent = 0;

      ScanDir(parent);
      follow_sibling_ = true;
      // Root folder?
      next_ = ReadIndexRecord(parent).child;
    } 
    else 
    { ScanAll();
      follow_sibling_ = false;
      next_ = 1;
    }
  }

  uint32_t MTPStorage_SD::GetNextObjectHandle() 
  {
    while (true) {
      if (next_ == 0) return 0;

      int ret = next_;
      Record r = ReadIndexRecord(ret);
      if (follow_sibling_) 
      { next_ = r.sibling;
      } 
      else 
      {
        next_++;
        if (next_ >= index_entries_) next_ = 0;
      }
      if (r.name[0]) return ret;
    }
  }

  void MTPStorage_SD::GetObjectInfo(uint32_t handle, char* name, uint32_t* size, uint32_t* parent) 
  {
    Record r = ReadIndexRecord(handle);
    strcpy(name, r.name);
    *parent = r.parent;
    *size = r.isdir ? 0xFFFFFFFFUL : r.child;
  }

  uint32_t MTPStorage_SD::GetSize(uint32_t handle) 
  {
    return ReadIndexRecord(handle).child;
  }

  void MTPStorage_SD::read(uint32_t handle, uint32_t pos, char* out, uint32_t bytes) 
  {
    OpenFileByIndex(handle);
    mtp_lock_storage(true);
    mFS.fileSeek(pos);
    mFS.fileRead(out,bytes);
    mtp_lock_storage(false);
  }

  bool MTPStorage_SD::DeleteObject(uint32_t object) 
  {
    char filename[256];
    Record r;
    while (true) {
      r = ReadIndexRecord(object == 0xFFFFFFFFUL ? 0 : object);
      if (!r.isdir) break;
      if (!r.child) break;
      if (!DeleteObject(r.child))  return false;
    }

    // We can't actually delete the root folder,
    // but if we deleted everything else, return true.
    if (object == 0xFFFFFFFFUL) return true;

    ConstructFilename(object, filename, 256);
    bool success;
    mtp_lock_storage(true);
    if (r.isdir) success = mFS.rmdir(filename); else  success = mFS.remove(filename);
    mtp_lock_storage(false);
    if (!success) return false;
    
    r.name[0] = 0;
    int p = r.parent;
    WriteIndexRecord(object, r);
    Record tmp = ReadIndexRecord(p);
    if (tmp.child == object) 
    { tmp.child = r.sibling;
      WriteIndexRecord(p, tmp);
    } 
    else 
    { int c = tmp.child;
      while (c) 
      { tmp = ReadIndexRecord(c);
        if (tmp.sibling == object) 
        { tmp.sibling = r.sibling;
          WriteIndexRecord(c, tmp);
          break;
        } 
        else 
        { c = tmp.sibling;
        }
      }
    }
    return true;
  }

  uint32_t MTPStorage_SD::Create(uint32_t parent,  bool folder, const char* filename) 
  {
    uint32_t ret;
    if (parent == 0xFFFFFFFFUL) parent = 0;
    Record p = ReadIndexRecord(parent);
    Record r;
    if (strlen(filename) > 62) return 0;
    strcpy(r.name, filename);
    r.parent = parent;
    r.child = 0;
    r.sibling = p.child;
    r.isdir = folder;
    // New folder is empty, scanned = true.
    r.scanned = 1;
    ret = p.child = AppendIndexRecord(r);
    WriteIndexRecord(parent, p);
    if (folder) 
    {
      char filename[256];
      ConstructFilename(ret, filename, 256);
      mtp_lock_storage(true);
      mFS.mkdir(filename);
      mtp_lock_storage(false);
    } 
    else 
    {
      OpenFileByIndex(ret, FILE_WRITE);
    }
    return ret;
  }

  void MTPStorage_SD::write(const char* data, uint32_t bytes) 
  {
      mtp_lock_storage(true);
      mFS.fileWrite(data,bytes);
      mtp_lock_storage(false);
  }

  void MTPStorage_SD::close() 
  {
    mtp_lock_storage(true);
    uint64_t size = mFS.fileSize();
    mFS.fileClose();
    mtp_lock_storage(false);
    Record r = ReadIndexRecord(open_file_);
    r.child = size;
    WriteIndexRecord(open_file_, r);
    open_file_ = 0xFFFFFFFEUL;
  }
#endif