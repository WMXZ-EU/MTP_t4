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

#define DEBUG 0

  #define sd_isOpen(x)  (x)
  #define sd_getName(x,y,n) strcpy(y,x.name())

  #define indexFile "/mtpindex.dat"
/*
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
*/
// TODO:
//   support serialflash
//   partial object fetch/receive
//   events (notify usb host when local storage changes) (But, this seems too difficult)

// These should probably be weak.
void mtp_yield() {}
void mtp_lock_storage(bool lock) {}

  bool MTPStorage_SD::readonly(uint32_t storage) { return false; }
  bool MTPStorage_SD::has_directories(uint32_t storage) { return true; }

  uint64_t MTPStorage_SD::totalSize(uint32_t storage) { return sd_totalSize(storage-1); }
  uint64_t MTPStorage_SD::usedSize(uint32_t storage) { return sd_usedSize(storage-1); }

  void MTPStorage_SD::CloseIndex()
  {
    mtp_lock_storage(true);
    if(sd_isOpen(index_)) index_.close();
    mtp_lock_storage(false);
    index_generated = false;
    index_entries_ = 0;
  }

  void MTPStorage_SD::OpenIndex() 
  { if(sd_isOpen(index_)) return; // only once
    mtp_lock_storage(true);
    index_=sd_open(0,indexFile, FILE_WRITE_BEGIN);
    mtp_lock_storage(false);
  }

  void MTPStorage_SD::ResetIndex() {
    if(!sd_isOpen(index_)) return;
    
    CloseIndex();
    OpenIndex();

    all_scanned_ = false;
    open_file_ = 0xFFFFFFFEUL;
  }

  void MTPStorage_SD::WriteIndexRecord(uint32_t i, const Record& r) 
  { OpenIndex();
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

  uint16_t MTPStorage_SD::ConstructFilename(int i, char* out, int len) // construct filename rexursively
  {
    Record tmp = ReadIndexRecord(i);
      
    if (tmp.parent==(unsigned)i) 
    { strcpy(out, "/");
      return tmp.store;
    }
    else 
    { ConstructFilename(tmp.parent, out, len);
      if (out[strlen(out)-1] != '/') strcat(out, "/");
      if(((strlen(out)+strlen(tmp.name)+1) < (unsigned) len)) strcat(out, tmp.name);
      return tmp.store;
    }
  }

  void MTPStorage_SD::OpenFileByIndex(uint32_t i, uint32_t mode) 
  {
    if (open_file_ == i && mode_ == mode) return;
    char filename[256];
    uint16_t store = ConstructFilename(i, filename, 256);
    mtp_lock_storage(true);
    if(sd_isOpen(file_)) file_.close();
    file_=sd_open(store,filename,mode);
    open_file_ = i;
    mode_ = mode;
    mtp_lock_storage(false);
  }

  // MTP object handles should not change or be re-used during a session.
  // This would be easy if we could just have a list of all files in memory.
  // Since our RAM is limited, we'll keep the index in a file instead.
  void MTPStorage_SD::GenerateIndex(uint32_t storage)
  { if (index_generated) return; 
    index_generated = true;
    // first remove old index file
    mtp_lock_storage(true);
    sd_remove(0,indexFile);
    mtp_lock_storage(false);

    num_storage = sd_getFSCount();

    index_entries_ = 0;
    Record r;
    for(int ii=0; ii<num_storage; ii++)
    {
      r.store = ii; // store is typically (storage-1) //store 0...6; storage 1...7
      r.parent = ii;
      r.sibling = 0;
      r.child = 0;
      r.isdir = true;
      r.scanned = false;
      strcpy(r.name, "/");
      AppendIndexRecord(r);
    }
  }

  void MTPStorage_SD::ScanDir(uint32_t storage, uint32_t i) 
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
        r.store = record.store;
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

  void MTPStorage_SD::ScanAll(uint32_t storage) 
  { if (all_scanned_) return;
    all_scanned_ = true;

    GenerateIndex(storage);
    for (uint32_t i = 0; i < index_entries_; i++)  ScanDir(storage,i);
  }

//  void  MTPStorage_SD::setStorageNumbers(const char **str, const int *csx, int num) 
//  { sd_str = str; 
//    num_storage=num;
//    setCs(csx);
//  }
//  uint32_t MTPStorage_SD::getNumStorage() 
//  { if(num_storage) return num_storage; else return 1;
//  }
//  const char * MTPStorage_SD::getStorageName(uint32_t storage) 
//  { if(sd_str) return sd_str[storage-1]; else return "SD_DISK";
//  }

  void MTPStorage_SD::StartGetObjectHandles(uint32_t storage, uint32_t parent) 
  { 
    GenerateIndex(storage);
    if (parent) 
    { if (parent == 0xFFFFFFFF) parent = storage-1; // As per initizalization

      ScanDir(storage, parent);
      follow_sibling_ = true;
      // Root folder?
      next_ = ReadIndexRecord(parent).child;
    } 
    else 
    { 
      ScanAll(storage);
      follow_sibling_ = false;
      next_ = 1;
    }
  }

  uint32_t MTPStorage_SD::GetNextObjectHandle(uint32_t  storage)
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

  void MTPStorage_SD::GetObjectInfo(uint32_t handle, char* name, uint32_t* size, uint32_t* parent, uint16_t *store)
  {
    Record r = ReadIndexRecord(handle);
    strcpy(name, r.name);
    *parent = r.parent;
    *size = r.isdir ? 0xFFFFFFFFUL : r.child;
    *store = r.store;
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
    if(object==0xFFFFFFFFUL) return false; // don't do anything if trying to delete a root directory
    Record r;
    while (true) {
      r = ReadIndexRecord(object == 0xFFFFFFFFUL ? 0 : object); //
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
    if (r.isdir) success = sd_rmdir(r.store,filename); else  success = sd_remove(r.store,filename);
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

  uint32_t MTPStorage_SD::Create(uint32_t storage, uint32_t parent,  bool folder, const char* filename)
  {
    uint32_t ret;
    if (parent == 0xFFFFFFFFUL) parent = storage-1;
    Record p = ReadIndexRecord(parent);
    Record r;
    if (strlen(filename) > 62) return 0;
    strcpy(r.name, filename);
    r.store = p.store;
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
      uint16_t store =ConstructFilename(ret, filename, 256);
      mtp_lock_storage(true);
      sd_mkdir(store,filename);
      mtp_lock_storage(false);
    } 
    else 
    {
      OpenFileByIndex(ret, FILE_WRITE_BEGIN);
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

  bool MTPStorage_SD::rename(uint32_t handle, const char* name) 
  { char oldName[256];
    char newName[256];
    char temp[64];

    uint16_t store = ConstructFilename(handle, oldName, 256);
    Serial.println(oldName);

    Record p1 = ReadIndexRecord(handle);
    strcpy(temp,p1.name);
    strcpy(p1.name,name);

    WriteIndexRecord(handle, p1);
    ConstructFilename(handle, newName, 256);
    Serial.println(newName);

    if (sd_rename(store,oldName,newName)) return true;

    // rename failed; undo index update
    strcpy(p1.name,temp);
    WriteIndexRecord(handle, p1);
    return false;
  }

  void MTPStorage_SD::dumpIndexList(void)
  {
    for(uint32_t ii=0; ii<index_entries_; ii++)
    { Record p = ReadIndexRecord(ii);
      Serial.printf("%d: %d %d %d %d %d %s\n",ii, p.store, p.isdir,p.parent,p.sibling,p.child,p.name);
    }
  }

  void MTPStorage_SD::printRecord(int h, Record *p) 
  { Serial.printf("%d: %d %d %d %d %d\n",h, p->store,p->isdir,p->parent,p->sibling,p->child); }
  
/*
 * //index list management for moving object around
 * p1 is record of handle
 * p2 is record of new dir
 * p3 is record of old dir
 * 
 *  // remove from old direcory
 * if p3.child == handle  / handle is last in old dir
 *      p3.child = p1.sibling   / simply relink old dir
 *      save p3
 * else
 *      px record of p3.child
 *      while( px.sibling != handle ) update px = record of px.sibling
 *      px.sibling = p1.sibling
 *      save px
 * 
 *  // add to new directory
 * p1.parent = new
 * p1.sibling = p2.child
 * p2.child = handle
 * save p1
 * save p2
 * 
 */

  bool MTPStorage_SD::move(uint32_t handle,uint32_t storage, uint32_t newParent ) 
  { 
    #if DEBUG==1
      Serial.printf("%d -> %d %d\n",handle,storage,newParent);
    #endif
    Record p1 = ReadIndexRecord(handle); 

    uint32_t oldParent = p1.parent;
    if(newParent<=0) newParent=(storage-1); //storage runs from 1, while record.store runs from 0
    //Serial.printf("%d -> %d %d\n",handle,storage,newParent);

    Record p2 = ReadIndexRecord(newParent);
    Record p3 = ReadIndexRecord(oldParent); 
    // keep original storages
    Record p1o = p1;
    Record p2o = p2;
    Record p3o = p3;

    #define DISK2DISK_MOVE 0 //set to 1 after disk to disk move is proven to work
    #if DISK_2DISK_MOVE==0
      if(p1.store != p2.store) 
      { Serial.println(" Disk to Disk move is not supported"); return false; }
    #endif

    char oldName[256];
    uint16_t store0 = ConstructFilename(handle, oldName, 256);
    #if DEBUG==1
      Serial.print(store0); Serial.print(": ");Serial.println(oldName);
      printIndexList();
    #endif

    // remove from old direcory
    uint32_t jx=-1;
    Record px;
    Record pxo;
    if(p3.child==handle)
    {
      p3.child = p1.sibling;
      WriteIndexRecord(oldParent, p3);    
    }
    else
    { jx = p3.child;
      px = ReadIndexRecord(jx); 
      pxo = px;
      while(handle != px.sibling)
      {
        jx = px.sibling;
        px = ReadIndexRecord(jx); 
        pxo = px;
      }
      px.sibling = p1.sibling;
      WriteIndexRecord(jx, px);
    }
  
    // add to new directory
    p1.parent = newParent;
    p1.store = p2.store;
    p1.sibling = p2.child;
    p2.child = handle;
    WriteIndexRecord(handle, p1);
    WriteIndexRecord(newParent,p2);

    char newName[256];
    uint32_t store1 = ConstructFilename(handle, newName, 256);
    #if DEBUG==1
      Serial.print(store1); Serial.print(": ");Serial.println(newName);
      printIndexList();
    #endif

    if(p2.store == p3.store)
    {
      if(sd_rename(store0,oldName,newName)) 
        return true; 
      else 
      {
        // undo changes in index list
        if(jx<0) WriteIndexRecord(oldParent, p3o); else WriteIndexRecord(jx, pxo);
        WriteIndexRecord(handle, p1o);
        WriteIndexRecord(newParent,p2o);      
        return false;
      }
    }
    //
    // copy from one store to another (not completely tested yet)
    // store0:oldName -> store1:newName
    // do not move directories cross storages
    if(p1.isdir) 
    {
      // undo changes in index list
      if(jx<0) WriteIndexRecord(oldParent, p3o); else WriteIndexRecord(jx, pxo);
      WriteIndexRecord(handle, p1o);
      WriteIndexRecord(newParent,p2o);      
      return false;
    }
    // move needs to be done by physically copying file from one store to another one and deleting in old store
    #if DEBUG==1
      Serial.print(store0); Serial.print(": ");Serial.println(oldName);
      Serial.print(store1); Serial.print(": ");Serial.println(newName);
    #endif

    const int nbuf = 2048;
    char buffer[nbuf];
    File f2 = sd_open(store1,newName,FILE_WRITE_BEGIN);
    if(sd_isOpen(f2))
    { f2.seek(0); // position file to beginning (ARDUINO opens at end of file)
      File f1 = sd_open(store0,oldName,FILE_READ);
      int nd;
      while(1)
      { nd=f1.read(buffer,nbuf);
        if(nd<0) break;
        f2.write(buffer,nd);
        if(nd<nbuf) break;
      }
      // check error
      if(nd<0) { Serial.print("File Read Error :"); Serial.println(f1.getReadError());}

      // close all files
      f1.close();
      f2.close();
      if(nd<0) //  something went wrong
      { sd_remove(store1,newName); 
        // undo changes in index list
        if(jx<0) WriteIndexRecord(oldParent, p3o); else WriteIndexRecord(jx, pxo);
        WriteIndexRecord(handle, p1o);
        WriteIndexRecord(newParent,p2o);      
        return false;
      }

      // remove old files
      if(sd_remove(store0,oldName)) 
        return true; 
      else 
      { // undo changes in index list
        if(jx<0) WriteIndexRecord(oldParent, p3o); else WriteIndexRecord(jx, pxo);
        WriteIndexRecord(handle, p1o);
        WriteIndexRecord(newParent,p2o);      
        return false;
      }
    }
    return false;
  }
