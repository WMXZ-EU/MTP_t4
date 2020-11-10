// Storage.cpp - Teensy MTP Responder library
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

#include "core_pins.h"
#include "usb_dev.h"
#include "usb_serial.h"

#include "Storage.h"

#if USE_SDFS==1
 SdFs sd;

  #define sd_begin sd.begin
  #define sd_open sd.open
  #define sd_mkdir sd.mkdir
  #define sd_rename sd.rename
  #define sd_remove sd.remove
  #define sd_rmdir sd.rmdir
  #define sd_isOpen(x)  x.isOpen()
  #define sd_getName(x,y,n) (x.getName(y,n))

#else
  #define sd_begin SD.sdfs.begin
  #define sd_open SD.open
  #define sd_mkdir SD.mkdir
  #define sd_rename SD.sdfs.rename
  #define sd_remove SD.remove
  #define sd_rmdir SD.rmdir
  #define sd_isOpen(x)  (x)
  #define sd_getName(x,y,n) strcpy(y,x.name())
#endif

  #define indexFile "/mtpindex.dat"

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


 bool Storage_init()
  { 
    #if DO_DEBUG>0
      Serial.println("Using SdFs");
    #endif
    #if USE_SDIO==0
      Serial.println("SPI");
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    if (!sd_begin(SD_CONFIG)) return false;

    // Set Time callback
    FsDateTime::callback = dateTime;

    return true;
	}


// TODO:
//   support multiple storages
//   support serialflash
//   partial object fetch/receive
//   events (notify usb host when local storage changes) (But, this seems too difficult)

// These should probably be weak.
void mtp_yield() {}
void mtp_lock_storage(bool lock) {}

  bool MTPStorage_SD::readonly() { return false; }
  bool MTPStorage_SD::has_directories() { return true; }
  
#if USE_SDFS==1
  uint32_t MTPStorage_SD::clusterCount() { return sd.clusterCount(); }
  uint32_t MTPStorage_SD::freeClusters() { return sd.freeClusterCount(); }
  uint32_t MTPStorage_SD::clusterSize() { return sd.sectorsPerCluster(); }
#else
  uint32_t MTPStorage_SD::clusterCount() { return SD.sdfs.clusterCount(); }
  uint32_t MTPStorage_SD::freeClusters() { return SD.sdfs.freeClusterCount(); }
  uint32_t MTPStorage_SD::clusterSize() { return SD.sdfs.sectorsPerCluster(); }
#endif

  void MTPStorage_SD::ResetIndex() {
    if(!sd_isOpen(index_)) return;
    
    mtp_lock_storage(true);
    if(sd_isOpen(index_)) index_.close();
    sd_remove(indexFile);
    index_ = sd_open(indexFile, FILE_WRITE);
    mtp_lock_storage(false);

    all_scanned_ = false;
    index_generated=false;
    open_file_ = 0xFFFFFFFEUL;
  }

  void MTPStorage_SD::CloseIndex()
  {
    mtp_lock_storage(true);
    index_.close();
    mtp_lock_storage(false);
    index_generated = false;
    index_entries_ = 0;
  }

  void MTPStorage_SD::OpenIndex() 
  { if(sd_isOpen(index_)) return; // only once
    mtp_lock_storage(true);
    index_=sd_open(indexFile, FILE_WRITE);
    mtp_lock_storage(false);
  }

  void MTPStorage_SD::WriteIndexRecord(uint32_t i, const Record& r) 
  {
    OpenIndex();
    mtp_lock_storage(true);
    index_.seek(sizeof(r) * i);
    index_.write((char*)&r, sizeof(r));
    mtp_lock_storage(false);
  }

  uint32_t MTPStorage_SD::AppendIndexRecord(const Record& r) 
  { uint32_t new_record = index_entries_++;
    WriteIndexRecord(new_record, r);
    return new_record;
  }

  // TODO(hubbe): Cache a few records for speed.
  Record MTPStorage_SD::ReadIndexRecord(uint32_t i) 
  {
    Record ret;
    memset(&ret, 0, sizeof(ret));
    if (i > index_entries_) 
    { memset(&ret, 0, sizeof(ret));
      return ret;
    }
    OpenIndex();
    mtp_lock_storage(true);
    index_.seek(sizeof(ret) * i);
    index_.read((char *)&ret, sizeof(ret));
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

  void MTPStorage_SD::OpenFileByIndex(uint32_t i, uint32_t mode) 
  {
    if (open_file_ == i && mode_ == mode) return;
    char filename[256];
    ConstructFilename(i, filename, 256);
    mtp_lock_storage(true);
    if(sd_isOpen(file_)) file_.close();
    file_=sd_open(filename,mode);
    open_file_ = i;
    mode_ = mode;
    mtp_lock_storage(false);
  }

  // MTP object handles should not change or be re-used during a session.
  // This would be easy if we could just have a list of all files in memory.
  // Since our RAM is limited, we'll keep the index in a file instead.
  void MTPStorage_SD::GenerateIndex()
  { if (index_generated) return; 
    index_generated = true;
    // first remove old index file
    mtp_lock_storage(true);
    sd_remove(indexFile);
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
  { Record record = ReadIndexRecord(i);
    if (record.isdir && !record.scanned) {
      OpenFileByIndex(i);
      if (!sd_isOpen(file_)) return;
    
      int sibling = 0;
      while (true) 
      { mtp_lock_storage(true);
        child_=file_.openNextFile();
        mtp_lock_storage(false);
        if(!sd_isOpen(child_)) break;

        Record r;
        r.parent = i;
        r.sibling = sibling;
        r.isdir = child_.isDirectory();
        r.child = r.isdir ? 0 : child_.size();
        r.scanned = false;
        sd_getName(child_,r.name,64);
        sibling = AppendIndexRecord(r);
        child_.close();
      }
      record.scanned = true;
      record.child = sibling;
      WriteIndexRecord(i, record);
    }
  }

  void MTPStorage_SD::ScanAll() 
  { if (all_scanned_) return;
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
    while (true) 
    { if (next_ == 0) return 0;

      int ret = next_;
      Record r = ReadIndexRecord(ret);
      if (follow_sibling_) 
      { next_ = r.sibling;
      } 
      else 
      { next_++;
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
    file_.seek(pos);
    file_.read(out,bytes);
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
    if (r.isdir) success = sd_rmdir(filename); else  success = sd_remove(filename);
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
      sd_mkdir(filename);
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
      file_.write(data,bytes);
      mtp_lock_storage(false);
  }

  void MTPStorage_SD::close() 
  {
    mtp_lock_storage(true);
    uint64_t size = file_.size();
    file_.close();
    mtp_lock_storage(false);
    Record r = ReadIndexRecord(open_file_);
    r.child = size;
    WriteIndexRecord(open_file_, r);
    open_file_ = 0xFFFFFFFEUL;
  }

  void MTPStorage_SD::rename(uint32_t handle, const char* name) 
  { char oldName[256];
    char newName[256];

    ConstructFilename(handle, oldName, 256);
    Record p1 = ReadIndexRecord(handle);
    strcpy(p1.name,name);
    WriteIndexRecord(handle, p1);
    ConstructFilename(handle, newName, 256);

    sd_rename(oldName,newName);
  }

  void MTPStorage_SD::move(uint32_t handle, uint32_t newParent ) 
  { char oldName[256];
    char newName[256];

    ConstructFilename(handle, oldName, 256);
    Record p1 = ReadIndexRecord(handle);

    if (newParent == 0xFFFFFFFFUL) newParent = 0;
    Record p2 = ReadIndexRecord(newParent); // is pointing to last object in directory

    p1.sibling = p2.child;
    p1.parent = newParent;

    p2.child = handle; 
    WriteIndexRecord(handle, p1);
    WriteIndexRecord(newParent, p2);

    ConstructFilename(handle, newName, 256);
    sd_rename(oldName,newName);
  }
