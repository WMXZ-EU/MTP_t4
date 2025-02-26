// MTP.cpp - Teensy MTP Responder library
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

// modified for SD by WMXZ

#if /*1 ||*/ defined(USB_MTPDISK) || defined(USB_MTPDISK_SERIAL)

#include "MTP.h"

#undef USB_DESC_LIST_DEFINE
#include "usb_desc.h"

#include "usb_mtp.h"
/*
#if defined(__IMXRT1062__)
  // following only while usb_mtp is not included in cores
  #if __has_include("usb_mtp.h")
    #include "usb_mtp.h"
  #else
    #include "usb1_mtp.h"
  #endif
#endif
*/
#include "usb_names.h"
extern struct usb_string_descriptor_struct usb_string_serial_number; 

#define DEBUG 1
#if DEBUG>0
  #define printf(...) Serial.printf(__VA_ARGS__)
#else
  #define printf(...) 
#endif

/***************************************************************************************************/
  // Container Types
  #define MTP_CONTAINER_TYPE_UNDEFINED    0
  #define MTP_CONTAINER_TYPE_COMMAND      1
  #define MTP_CONTAINER_TYPE_DATA         2
  #define MTP_CONTAINER_TYPE_RESPONSE     3
  #define MTP_CONTAINER_TYPE_EVENT        4

  // Container Offsets
  #define MTP_CONTAINER_LENGTH_OFFSET             0
  #define MTP_CONTAINER_TYPE_OFFSET               4
  #define MTP_CONTAINER_CODE_OFFSET               6
  #define MTP_CONTAINER_TRANSACTION_ID_OFFSET     8
  #define MTP_CONTAINER_PARAMETER_OFFSET          12
  #define MTP_CONTAINER_HEADER_SIZE               12

  // MTP Operation Codes
  #define MTP_OPERATION_GET_DEVICE_INFO                       0x1001
  #define MTP_OPERATION_OPEN_SESSION                          0x1002
  #define MTP_OPERATION_CLOSE_SESSION                         0x1003
  #define MTP_OPERATION_GET_STORAGE_IDS                       0x1004
  #define MTP_OPERATION_GET_STORAGE_INFO                      0x1005
  #define MTP_OPERATION_GET_NUM_OBJECTS                       0x1006
  #define MTP_OPERATION_GET_OBJECT_HANDLES                    0x1007
  #define MTP_OPERATION_GET_OBJECT_INFO                       0x1008
  #define MTP_OPERATION_GET_OBJECT                            0x1009
  #define MTP_OPERATION_GET_THUMB                             0x100A
  #define MTP_OPERATION_DELETE_OBJECT                         0x100B
  #define MTP_OPERATION_SEND_OBJECT_INFO                      0x100C
  #define MTP_OPERATION_SEND_OBJECT                           0x100D
  #define MTP_OPERATION_INITIATE_CAPTURE                      0x100E
  #define MTP_OPERATION_FORMAT_STORE                          0x100F
  #define MTP_OPERATION_RESET_DEVICE                          0x1010
  #define MTP_OPERATION_SELF_TEST                             0x1011
  #define MTP_OPERATION_SET_OBJECT_PROTECTION                 0x1012
  #define MTP_OPERATION_POWER_DOWN                            0x1013
  #define MTP_OPERATION_GET_DEVICE_PROP_DESC                  0x1014
  #define MTP_OPERATION_GET_DEVICE_PROP_VALUE                 0x1015
  #define MTP_OPERATION_SET_DEVICE_PROP_VALUE                 0x1016
  #define MTP_OPERATION_RESET_DEVICE_PROP_VALUE               0x1017
  #define MTP_OPERATION_TERMINATE_OPEN_CAPTURE                0x1018
  #define MTP_OPERATION_MOVE_OBJECT                           0x1019
  #define MTP_OPERATION_COPY_OBJECT                           0x101A
  #define MTP_OPERATION_GET_PARTIAL_OBJECT                    0x101B
  #define MTP_OPERATION_INITIATE_OPEN_CAPTURE                 0x101C
  #define MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED            0x9801
  #define MTP_OPERATION_GET_OBJECT_PROP_DESC                  0x9802
  #define MTP_OPERATION_GET_OBJECT_PROP_VALUE                 0x9803
  #define MTP_OPERATION_SET_OBJECT_PROP_VALUE                 0x9804
  #define MTP_OPERATION_GET_OBJECT_PROP_LIST                  0x9805
  #define MTP_OPERATION_SET_OBJECT_PROP_LIST                  0x9806
  #define MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC          0x9807
  #define MTP_OPERATION_SEND_OBJECT_PROP_LIST                 0x9808
  #define MTP_OPERATION_GET_OBJECT_REFERENCES                 0x9810
  #define MTP_OPERATION_SET_OBJECT_REFERENCES                 0x9811
  #define MTP_OPERATION_SKIP                                  0x9820

  // Responses
  #define MTP_RESPONSE_UNDEFINED                              0x2000
  #define MTP_RESPONSE_OK                                     0x2001
  #define MTP_RESPONSE_GENERAL_ERROR                          0x2002
  #define MTP_RESPONSE_SESSION_NOT_OPEN                       0x2003
  #define MTP_RESPONSE_INVALID_TRANSACTION_ID                 0x2004
  #define MTP_RESPONSE_OPERATION_NOT_SUPPORTED                0x2005
  #define MTP_RESPONSE_PARAMETER_NOT_SUPPORTED                0x2006
  #define MTP_RESPONSE_INCOMPLETE_TRANSFER                    0x2007
  #define MTP_RESPONSE_INVALID_STORAGE_ID                     0x2008
  #define MTP_RESPONSE_INVALID_OBJECT_HANDLE                  0x2009
  #define MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED              0x200A
  #define MTP_RESPONSE_INVALID_OBJECT_FORMAT_CODE             0x200B
  #define MTP_RESPONSE_STORAGE_FULL                           0x200C
  #define MTP_RESPONSE_OBJECT_WRITE_PROTECTED                 0x200D
  #define MTP_RESPONSE_STORE_READ_ONLY                        0x200E
  #define MTP_RESPONSE_ACCESS_DENIED                          0x200F
  #define MTP_RESPONSE_NO_THUMBNAIL_PRESENT                   0x2010
  #define MTP_RESPONSE_SELF_TEST_FAILED                       0x2011
  #define MTP_RESPONSE_PARTIAL_DELETION                       0x2012
  #define MTP_RESPONSE_STORE_NOT_AVAILABLE                    0x2013
  #define MTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED    0x2014
  #define MTP_RESPONSE_NO_VALID_OBJECT_INFO                   0x2015
  #define MTP_RESPONSE_INVALID_CODE_FORMAT                    0x2016
  #define MTP_RESPONSE_UNKNOWN_VENDOR_CODE                    0x2017
  #define MTP_RESPONSE_CAPTURE_ALREADY_TERMINATED             0x2018
  #define MTP_RESPONSE_DEVICE_BUSY                            0x2019
  #define MTP_RESPONSE_INVALID_PARENT_OBJECT                  0x201A
  #define MTP_RESPONSE_INVALID_DEVICE_PROP_FORMAT             0x201B
  #define MTP_RESPONSE_INVALID_DEVICE_PROP_VALUE              0x201C
  #define MTP_RESPONSE_INVALID_PARAMETER                      0x201D
  #define MTP_RESPONSE_SESSION_ALREADY_OPEN                   0x201E
  #define MTP_RESPONSE_TRANSACTION_CANCELLED                  0x201F
  #define MTP_RESPONSE_SPECIFICATION_OF_DESTINATION_UNSUPPORTED 0x2020
  #define MTP_RESPONSE_INVALID_OBJECT_PROP_CODE               0xA801
  #define MTP_RESPONSE_INVALID_OBJECT_PROP_FORMAT             0xA802
  #define MTP_RESPONSE_INVALID_OBJECT_PROP_VALUE              0xA803
  #define MTP_RESPONSE_INVALID_OBJECT_REFERENCE               0xA804
  #define MTP_RESPONSE_GROUP_NOT_SUPPORTED                    0xA805
  #define MTP_RESPONSE_INVALID_DATASET                        0xA806
  #define MTP_RESPONSE_SPECIFICATION_BY_GROUP_UNSUPPORTED     0xA807
  #define MTP_RESPONSE_SPECIFICATION_BY_DEPTH_UNSUPPORTED     0xA808
  #define MTP_RESPONSE_OBJECT_TOO_LARGE                       0xA809
  #define MTP_RESPONSE_OBJECT_PROP_NOT_SUPPORTED              0xA80A


  const unsigned short supported_op[]=
  {
    MTP_OPERATION_GET_DEVICE_INFO                        ,//0x1001
    MTP_OPERATION_OPEN_SESSION                           ,//0x1002
    MTP_OPERATION_CLOSE_SESSION                          ,//0x1003
    MTP_OPERATION_GET_STORAGE_IDS                        ,//0x1004
    MTP_OPERATION_GET_STORAGE_INFO                       ,//0x1005
    //MTP_OPERATION_GET_NUM_OBJECTS                        ,//0x1006
    MTP_OPERATION_GET_OBJECT_HANDLES                     ,//0x1007
    MTP_OPERATION_GET_OBJECT_INFO                        ,//0x1008
    MTP_OPERATION_GET_OBJECT                             ,//0x1009
    //MTP_OPERATION_GET_THUMB                              ,//0x100A
    MTP_OPERATION_DELETE_OBJECT                          ,//0x100B
    MTP_OPERATION_SEND_OBJECT_INFO                       ,//0x100C
    MTP_OPERATION_SEND_OBJECT                            ,//0x100D
    MTP_OPERATION_GET_DEVICE_PROP_DESC                   ,//0x1014
    MTP_OPERATION_GET_DEVICE_PROP_VALUE                  ,//0x1015
    //MTP_OPERATION_SET_DEVICE_PROP_VALUE                  ,//0x1016
    //MTP_OPERATION_RESET_DEVICE_PROP_VALUE                ,//0x1017
    MTP_OPERATION_MOVE_OBJECT                           ,//0x1019
    MTP_OPERATION_COPY_OBJECT                           ,//0x101A
    MTP_OPERATION_GET_PARTIAL_OBJECT                     ,//0x101B

    //MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED             ,//0x9801
    //MTP_OPERATION_GET_OBJECT_PROP_DESC                   ,//0x9802
    //MTP_OPERATION_GET_OBJECT_PROP_VALUE                  ,//0x9803
    //MTP_OPERATION_SET_OBJECT_PROP_VALUE                  //0x9804
    
    //MTP_OPERATION_GET_OBJECT_PROP_LIST                   ,//0x9805
    //MTP_OPERATION_GET_OBJECT_REFERENCES                  ,//0x9810
    //MTP_OPERATION_SET_OBJECT_REFERENCES                  ,//0x9811

    //MTP_OPERATION_GET_PARTIAL_OBJECT_64                  ,//0x95C1
    //MTP_OPERATION_SEND_PARTIAL_OBJECT                    ,//0x95C2
    //MTP_OPERATION_TRUNCATE_OBJECT                        ,//0x95C3
    //MTP_OPERATION_BEGIN_EDIT_OBJECT                      ,//0x95C4
    //MTP_OPERATION_END_EDIT_OBJECT                         //0x95C5
  };
  
  const int supported_op_size=sizeof(supported_op);
  const int supported_op_num = supported_op_size/sizeof(supported_op[0]);


  #define MTP_PROPERTY_STORAGE_ID                             0xDC01
  #define MTP_PROPERTY_OBJECT_FORMAT                          0xDC02
  #define MTP_PROPERTY_PROTECTION_STATUS                      0xDC03
  #define MTP_PROPERTY_OBJECT_SIZE                            0xDC04
  #define MTP_PROPERTY_OBJECT_FILE_NAME                       0xDC07
  #define MTP_PROPERTY_DATE_CREATED                           0xDC08
  #define MTP_PROPERTY_DATE_MODIFIED                          0xDC09
  #define MTP_PROPERTY_PARENT_OBJECT                          0xDC0B
  #define MTP_PROPERTY_PERSISTENT_UID                         0xDC41
  #define MTP_PROPERTY_NAME                                   0xDC44

  const uint16_t propertyList[] =
  {
    MTP_PROPERTY_STORAGE_ID                             ,//0xDC01
    MTP_PROPERTY_OBJECT_FORMAT                          ,//0xDC02
    MTP_PROPERTY_PROTECTION_STATUS                      ,//0xDC03
    MTP_PROPERTY_OBJECT_SIZE                            ,//0xDC04
    MTP_PROPERTY_OBJECT_FILE_NAME                       ,//0xDC07
    MTP_PROPERTY_DATE_CREATED                           ,//0xDC08
    MTP_PROPERTY_DATE_MODIFIED                          ,//0xDC09
    MTP_PROPERTY_PARENT_OBJECT                          ,//0xDC0B
    MTP_PROPERTY_PERSISTENT_UID                         ,//0xDC41
    MTP_PROPERTY_NAME                                    //0xDC44
  };
  
  uint32_t propertyListNum = sizeof(propertyList)/sizeof(propertyList[0]);
  

    #define MTP_EVENT_UNDEFINED                         0x4000
    #define MTP_EVENT_CANCEL_TRANSACTION                0x4001
    #define MTP_EVENT_OBJECT_ADDED                      0x4002
    #define MTP_EVENT_OBJECT_REMOVED                    0x4003
    #define MTP_EVENT_STORE_ADDED                       0x4004
    #define MTP_EVENT_STORE_REMOVED                     0x4005
    #define MTP_EVENT_DEVICE_PROP_CHANGED               0x4006
    #define MTP_EVENT_OBJECT_INFO_CHANGED               0x4007
    #define MTP_EVENT_DEVICE_INFO_CHANGED               0x4008
    #define MTP_EVENT_REQUEST_OBJECT_TRANSFER           0x4009
    #define MTP_EVENT_STORE_FULL                        0x400A
    #define MTP_EVENT_DEVICE_RESET                      0x400B
    #define MTP_EVENT_STORAGE_INFO_CHANGED              0x400C
    #define MTP_EVENT_CAPTURE_COMPLETE                  0x400D
    #define MTP_EVENT_UNREPORTED_STATUS                 0x400E
    #define MTP_EVENT_OBJECT_PROP_CHANGED               0xC801
    #define MTP_EVENT_OBJECT_PROP_DESC_CHANGED          0xC802  
    #define MTP_EVENT_OBJECT_REFERENCES_CHANGED         0xC803

const uint16_t supported_events[] =
  {
//    MTP_EVENT_UNDEFINED                         ,//0x4000
//    MTP_EVENT_CANCEL_TRANSACTION                ,//0x4001
//    MTP_EVENT_OBJECT_ADDED                      ,//0x4002
//    MTP_EVENT_OBJECT_REMOVED                    ,//0x4003
    MTP_EVENT_STORE_ADDED                       ,//0x4004
    MTP_EVENT_STORE_REMOVED                     ,//0x4005
//    MTP_EVENT_DEVICE_PROP_CHANGED               ,//0x4006
//    MTP_EVENT_OBJECT_INFO_CHANGED               ,//0x4007
//    MTP_EVENT_DEVICE_INFO_CHANGED               ,//0x4008
//    MTP_EVENT_REQUEST_OBJECT_TRANSFER           ,//0x4009
//    MTP_EVENT_STORE_FULL                        ,//0x400A
    MTP_EVENT_DEVICE_RESET                      ,//0x400B
    MTP_EVENT_STORAGE_INFO_CHANGED              ,//0x400C
//    MTP_EVENT_CAPTURE_COMPLETE                  ,//0x400D
//    MTP_EVENT_UNREPORTED_STATUS                 ,//0x400E
//    MTP_EVENT_OBJECT_PROP_CHANGED               ,//0xC801
//    MTP_EVENT_OBJECT_PROP_DESC_CHANGED          ,//0xC802  
//    MTP_EVENT_OBJECT_REFERENCES_CHANGED          //0xC803
  };
  
  const int supported_event_num = sizeof(supported_events)/sizeof(supported_events[0]);

 uint32_t sessionID_;

// MTP Responder.
/*
  struct MTPHeader {
    uint32_t len;  // 0
    uint16_t type; // 4
    uint16_t op;   // 6
    uint32_t transaction_id; // 8
  };

  struct MTPContainer {
    uint32_t len;  // 0
    uint16_t type; // 4
    uint16_t op;   // 6
    uint32_t transaction_id; // 8
    uint32_t params[5];    // 12
  };
*/

  void MTPD::write8 (uint8_t  x) { write((char*)&x, sizeof(x)); }
  void MTPD::write16(uint16_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write32(uint32_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write64(uint64_t x) { write((char*)&x, sizeof(x)); }

#define Store2Storage(x) (x+1)
#define Storage2Store(x) (x-1)

  void MTPD::writestring(const char* str) {
    if (*str) 
    { write8(strlen(str) + 1);
      while (*str) {  write16(*str);  ++str;  } write16(0);
    } else 
    { write8(0);
    }
  }

  void MTPD::WriteDescriptor() {
    uint32_t num_bytes=0;
    const char *msoft="microsoft.com: 1.0;";

    char buf1[20];    
    dtostrf( (float)(TEENSYDUINO / 100.0f), 3, 2, buf1);
    strlcat(buf1, " / MTP " MTP_VERS, sizeof(buf1) );
    
    char buf2[20];    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
    for (size_t i=0; i<10; i++) buf2[i] = usb_string_serial_number.wString[i];
    #pragma GCC diagnostic pop

    num_bytes=2+4+2+strlen(msoft)+2+ \
        4+supported_op_num*2+4+supported_event_num*2+ \
        4+2+4+4+2+2+strlen(MTP_MANUF)+strlen(MTP_MODEL)+ \
        strlen(buf1)+strlen(buf2);

    write16(100);  // MTP version
    write32(6);    // MTP extension
    write16(100);  // MTP version
    writestring(msoft);
    write16(0);    // functional mode

    // Supported operations (array of uint16)
    write32(supported_op_num);
    for(int ii=0; ii<supported_op_num;ii++) write16(supported_op[ii]);
    
    // Events (array of uint16)
    write32(supported_event_num);      
    for(int ii=0; ii<supported_event_num;ii++) write16(supported_events[ii]);

    write32(1);       // Device properties (array of uint16)
    write16(0xd402);  // Device friendly name

    write32(0);       // Capture formats (array of uint16)

    write32(2);       // Playback formats (array of uint16)
    write16(0x3000);  // Undefined format
    write16(0x3001);  // Folders (associations)

    writestring(MTP_MANUF);     // Manufacturer
    writestring(MTP_MODEL);     // Model
    
    writestring(buf1);    
    writestring(buf2);    
    (void) num_bytes;
  }

  void MTPD::WriteStorageIDs() {
    uint32_t num=storage_->get_FSCount();
    uint32_t num_bytes = 0;
    num_bytes= 4+num*4;

    write32(num); // number of storages (disks)
    for(uint32_t ii=0;ii<num;ii++)  write32(Store2Storage(ii)); // storage id
    (void) num_bytes;
  }

  void MTPD::GetStorageInfo(uint32_t storage) {
    uint32_t store = Storage2Store(storage);
    uint32_t num_bytes=0;
    const char *name = storage_->get_FSName(store);
    num_bytes=2+2+2+8+8+4+strlen(name)+strlen("");

    write16(storage_->readonly(store) ? 0x0001 : 0x0004);   // storage type (removable RAM)
    write16(storage_->has_directories(store) ? 0x0002: 0x0001);   // filesystem type (generic hierarchical)
    write16(0x0000);   // access capability (read-write)

    uint64_t ntotal = storage_->totalSize(store) ; 
    uint64_t nused = storage_->usedSize(store) ; 
    Serial.print("size ");Serial.println(ntotal);
    Serial.print("used ");Serial.println(nused);

    write64(ntotal);  // max capacity
    write64((ntotal-nused));  // free space (100M)
    //
    write32(0xFFFFFFFFUL);  // free space (objects)
    writestring(name);  // storage descriptor
    writestring("");  // volume identifier
    (void) num_bytes;

    //printf("%d %d ",storage,store); Serial.println(name); Serial.flush();
  }

  uint32_t MTPD::GetNumObjects(uint32_t storage, uint32_t parent) 
  { uint32_t store = Storage2Store(storage);
    storage_->StartGetObjectHandles(store, parent);
    int num = 0;
    while (storage_->GetNextObjectHandle(store)) num++;
    return num;
  }

  void MTPD::GetObjectHandles(uint32_t storage, uint32_t parent) 
  { uint32_t store = Storage2Store(storage);
    if (write_get_length_) {
      write_length_ = GetNumObjects(storage, parent);
      write_length_++;
      write_length_ *= 4;
    }
    else{
      uint32_t numObjects=GetNumObjects(storage, parent);
      //
      uint32_t num_bytes =0;
      num_bytes=4+numObjects*4;
      write32(numObjects);
      int handle;
      storage_->StartGetObjectHandles(store, parent);
      while ((handle = storage_->GetNextObjectHandle(store))) write32(handle);
      (void) num_bytes;
    }
  }
  
  void MTPD::GetObjectInfo(uint32_t handle) 
  {
    char filename[MAX_FILENAME_LEN];
    uint32_t filesize, parent;
    uint16_t store;
    char create[64],modify[64];
    storage_->GetObjectInfo(handle, filename, &filesize, &parent, &store, create, modify);

    uint32_t storage = Store2Storage(store);
    uint32_t num_bytes=0;
    printf("%d %s %d\n",handle,filename,filesize);

    num_bytes=4+2+2+4+2+4+4+4+4+4+4+4+2+4+4+ \
        strlen(filename)+strlen(create)+strlen(modify)+strlen("");
    write32(storage); // storage
    write16(filesize == 0xFFFFFFFFUL ? 0x3001 : 0x0000); // format
    write16(0);  // protection
    write32(filesize); // size
    write16(0); // thumb format
    write32(0); // thumb size
    write32(0); // thumb width
    write32(0); // thumb height
    write32(0); // pix width
    write32(0); // pix height
    write32(0); // bit depth
    write32(parent); // parent
    write16(filesize == 0xFFFFFFFFUL ? 1 : 0); // association type
    write32(0); // association description
    write32(0);  // sequence number
    writestring(filename);
    writestring(create);  // date created
    writestring(modify);  // date modified
    writestring("");  // keywords
    (void) num_bytes;
  }

  uint32_t MTPD::ReadMTPHeader() 
  {
    MTPHeader header;
    read((char *)&header, sizeof(MTPHeader));
    // check that the type is data
    if(header.type==2)
      return header.len - 12;
    else
      return 0;
  }

  uint8_t MTPD::read8() { uint8_t ret; read((char*)&ret, sizeof(ret));  return ret;  }
  uint16_t MTPD::read16() { uint16_t ret; read((char*)&ret, sizeof(ret)); return ret; }
  uint32_t MTPD::read32() { uint32_t ret; read((char*)&ret, sizeof(ret)); return ret; }

  void MTPD::readstring(char* buffer) {
    int len = read8();
    if (!buffer) {
      read(NULL, len * 2);
    } else {
      for (int i = 0; i < len; i++) {
        int16_t c2;
        *(buffer++) = c2 = read16();
      }
    }
  }

  void MTPD::GetDevicePropValue(uint32_t prop) {
    uint32_t num_bytes=0;
    switch (prop) {
      case 0xd402: // friendly name
        // This is the name we'll actually see in the windows explorer.
        // Should probably be configurable.
        num_bytes=strlen(MTP_NAME);
        writestring(MTP_NAME);
        break;
    }
    (void) num_bytes;
  }

  void MTPD::GetDevicePropDesc(uint32_t prop) {
    uint32_t num_bytes=0;
    switch (prop) {
      case 0xd402: // friendly name
        num_bytes=2+2+1+2*strlen(MTP_NAME)+1;
        write16(prop);
        write16(0xFFFF); // string type
        write8(0);       // read-only
        writestring(MTP_NAME);
        writestring(MTP_NAME);
        write8(0);       // no form
    }
    (void) num_bytes;
  }

    void MTPD::getObjectPropsSupported(uint32_t p1)
    {
      uint32_t num_bytes=0;
      num_bytes=4+propertyListNum*2;
      write32(propertyListNum);
      for(uint32_t ii=0; ii<propertyListNum;ii++) write16(propertyList[ii]);
      (void) num_bytes;
    }

    void MTPD::getObjectPropDesc(uint32_t p1, uint32_t p2)
    {
      uint32_t num_bytes=0;
      switch(p1)
      {
        case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
          num_bytes=2+2+1+4+4+1;
          write16(0xDC01);
          write16(0x006);
          write8(0); //get
          write32(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_OBJECT_FORMAT:        //0xDC02:
          num_bytes=2+2+1+2+4+1;
          write16(0xDC02);
          write16(0x004);
          write8(0); //get
          write16(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_PROTECTION_STATUS:    //0xDC03:
          num_bytes=2+2+1+2+4+1;
          write16(0xDC03);
          write16(0x004);
          write8(0); //get
          write16(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
          num_bytes=2+2+1+8+4+1;
          write16(0xDC04);
          write16(0x008);
          write8(0); //get
          write64(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
          num_bytes=2+2+1+1+4+1;
          write16(0xDC07);
          write16(0xFFFF);
          write8(1); //get/set
          write8(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
          num_bytes=2+2+1+1+4+1;
          write16(0xDC08);
          write16(0xFFFF);
          write8(0); //get
          write8(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
          num_bytes=2+2+1+1+4+1;
          write16(0xDC09);
          write16(0xFFFF);
          write8(0); //get
          write8(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_PARENT_OBJECT:    //0xDC0B:
          num_bytes=2+2+1+4+4+1;
          write16(0xDC0B);
          write16(6);
          write8(0); //get
          write32(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_PERSISTENT_UID:   //0xDC41:
          num_bytes=2+2+1+8+8+4+1;
          write16(0xDC41);
          write16(0x0A);
          write8(0); //get
          write64(0);
          write64(0);
          write32(0);
          write8(0);
          break;
        case MTP_PROPERTY_NAME:             //0xDC44:
          num_bytes=2+2+1+1+4+1;
          write16(0xDC44);
          write16(0xFFFF);
          write8(0); //get
          write8(0);
          write32(0);
          write8(0);
          break;
        default:
          num_bytes=0;
          break;
      }
      (void) num_bytes;
    }

    void MTPD::getObjectPropValue(uint32_t p1, uint32_t p2)
    { char name[MAX_FILENAME_LEN];
      uint32_t dir;
      uint32_t filesize;
      uint32_t parent;
      uint16_t store;
      char create[64], modify[64];

      storage_->GetObjectInfo(p1,name,&filesize,&parent, &store, create, modify);
      dir = (filesize == 0xFFFFFFFFUL);
      uint32_t storage = Store2Storage(store);
      //
      uint32_t num_bytes=0;
      switch(p2)
      { 
        case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
          num_bytes=4;
          write32(storage);
          break;
        case MTP_PROPERTY_OBJECT_FORMAT:      //0xDC02:
          num_bytes=2;
          write16(dir?0x3001:0x3000);
          break;
        case MTP_PROPERTY_PROTECTION_STATUS:  //0xDC03:
          num_bytes=2;
          write16(0);
          break;
        case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
          num_bytes=4+4;
          write32(filesize);
          write32(0);
          break;
        case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
          num_bytes=strlen(name);
          writestring(name);
          break;
        case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
          num_bytes=strlen(create);
          writestring(create);
          break;
        case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
          num_bytes=strlen(modify);
          writestring(modify);
          break;
        case MTP_PROPERTY_PARENT_OBJECT:      //0xDC0B:
          num_bytes=4;
          write32((store==parent)? 0: parent);
          break;
        case MTP_PROPERTY_PERSISTENT_UID:     //0xDC41:
          num_bytes=4+4+4+4;
          write32(p1);
          write32(parent);
          write32(storage);
          write32(0);
          break;
        case MTP_PROPERTY_NAME:               //0xDC44:
          num_bytes=strlen(name);
          writestring(name);
          break;
        default:
          num_bytes=0;
          break;
      }
      (void) num_bytes;
    }
    
    uint32_t MTPD::deleteObject(uint32_t handle)
    {
        if (!storage_->DeleteObject(handle))
        {
          return 0x2012; // partial deletion
        }
        return 0x2001;
    }

    uint32_t MTPD::moveObject(uint32_t handle, uint32_t newStorage, uint32_t newHandle)
    { uint32_t store1=Storage2Store(newStorage);
      if(storage_->move(handle,store1,newHandle)) return 0x2001; else return  0x2005;
    }
    
    uint32_t MTPD::copyObject(uint32_t handle, uint32_t newStorage, uint32_t newHandle)
    { uint32_t store1=Storage2Store(newStorage);
      return storage_->copy(handle,store1,newHandle);
    }
    
    void MTPD::openSession(uint32_t id)
    {
      sessionID_ = id;
      storage_->ResetIndex();
    }


#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

  void MTPD::write(const char *data, int len) {
    if (write_get_length_) {
      write_length_ += len;
    } else {
      int pos = 0;
      while (pos < len) {
        get_buffer();
        int avail = sizeof(data_buffer_->buf) - data_buffer_->len;
        int to_copy = min(len - pos, avail);
        memcpy(data_buffer_->buf + data_buffer_->len,
               data + pos,
               to_copy);
        data_buffer_->len += to_copy;
        pos += to_copy;
        if (data_buffer_->len == sizeof(data_buffer_->buf)) {
          usb_tx(MTP_TX_ENDPOINT, data_buffer_);
          data_buffer_ = NULL;
        }
      }
    }
  }
  void MTPD::GetObject(uint32_t object_id) 
  {
    uint32_t size = storage_->GetSize(object_id);
    if (write_get_length_) {
      write_length_ += size;
    } else {
      uint32_t pos = 0;
      while (pos < size) {
        get_buffer();
        uint32_t avail = sizeof(data_buffer_->buf) - data_buffer_->len;
        uint32_t to_copy = min(size - pos, avail);
        // Read directly from storage into usb buffer.
        storage_->read(object_id, pos,
                    (char*)(data_buffer_->buf + data_buffer_->len), to_copy);
        pos += to_copy;
        data_buffer_->len += to_copy;
        if (data_buffer_->len == sizeof(data_buffer_->buf)) {
          usb_tx(MTP_TX_ENDPOINT, data_buffer_);
          data_buffer_ = NULL;
        }
      }
    }
  }

  #define CONTAINER ((struct MTPContainer*)(receive_buffer->buf))

  #define TRANSMIT(FUN) do {                              \
      write_length_ = 0;                                  \
      write_get_length_ = true;                           \
      FUN;                                                \
      write_get_length_ = false;                          \
      MTPHeader header;                                   \
      header.len = write_length_ + 12;                    \
      header.type = 2;                                    \
      header.op = CONTAINER->op;                          \
      header.transaction_id = CONTAINER->transaction_id;  \
      write((char *)&header, sizeof(header));             \
      FUN;                                                \
      get_buffer();                                       \
      usb_tx(MTP_TX_ENDPOINT, data_buffer_);              \
      data_buffer_ = NULL;                                \
    } while(0)

    #define printContainer() \
    { printf("%x %d %d %d: ", CONTAINER->op, CONTAINER->len, CONTAINER->type, CONTAINER->transaction_id); \
      if(CONTAINER->len>12) printf(" %x", CONTAINER->params[0]); \
      if(CONTAINER->len>16) printf(" %x", CONTAINER->params[1]); \
      if(CONTAINER->len>20) printf(" %x", CONTAINER->params[2]); \
      printf("\n"); \
    }

  void MTPD::read(char* data, uint32_t size) 
  {
    while (size) {
      receive_buffer();
      uint32_t to_copy = data_buffer_->len - data_buffer_->index;
      to_copy = min(to_copy, size);
      if (data) {
        memcpy(data, data_buffer_->buf + data_buffer_->index, to_copy);
        data += to_copy;
      }
      size -= to_copy;
      data_buffer_->index += to_copy;
      if (data_buffer_->index == data_buffer_->len) {
        usb_free(data_buffer_);
        data_buffer_ = NULL;
      }
    }
  }

  uint32_t MTPD::SendObjectInfo(uint32_t storage, uint32_t parent) {
    uint32_t len = ReadMTPHeader();
    char filename[MAX_FILENAME_LEN];

    uint32_t store = Storage2Store(storage);

    read32(); len-=4; // storage
    bool dir = read16() == 0x3001; len-=2; // format
    read16();  len-=2; // protection
    read32(); len-=4; // size
    read16(); len-=2; // thumb format
    read32(); len-=4; // thumb size
    read32(); len-=4; // thumb width
    read32(); len-=4; // thumb height
    read32(); len-=4; // pix width
    read32(); len-=4; // pix height
    read32(); len-=4; // bit depth
    read32(); len-=4; // parent
    read16(); len-=2; // association type
    read32(); len-=4; // association description
    read32(); len-=4; // sequence number

    readstring(filename); len -= (2*(strlen(filename)+1)+1); 
    // ignore rest of ObjectInfo
    while(len>=4) { read32(); len-=4;}
    while(len) {read8(); len--;}
    
    return storage_->Create(store, parent, dir, filename);
  }

  bool MTPD::SendObject() {
    uint32_t len = ReadMTPHeader();
    while (len) 
    { 
      receive_buffer();
      uint32_t to_copy = data_buffer_->len - data_buffer_->index;
      to_copy = min(to_copy, len);
      if(!storage_->write((char*)(data_buffer_->buf + data_buffer_->index), to_copy)) return false;
      data_buffer_->index += to_copy;
      len -= to_copy;
      if (data_buffer_->index == data_buffer_->len) 
      {
        usb_free(data_buffer_);
        data_buffer_ = NULL;
      }
    }
    storage_->close();
    return true;
  }
  
    uint32_t MTPD::setObjectPropValue(uint32_t p1, uint32_t p2)
    {
      receive_buffer();
      if(p2==0xDC07)
      {
        char filename[MAX_FILENAME_LEN];
        ReadMTPHeader();
        readstring(filename);

        storage_->rename(p1,filename);

        return 0x2001;
      }
      else
        return 0x2005;
    }

  void MTPD::loop(void) 
  {
    usb_packet_t *receive_buffer;
    if ((receive_buffer = usb_rx(MTP_RX_ENDPOINT))) {
      printContainer();
      
        int op = CONTAINER->op;
        int p1 = CONTAINER->params[0];
        int p2 = CONTAINER->params[1];
        int p3 = CONTAINER->params[2];
        int id = CONTAINER->transaction_id;
        int len= CONTAINER->len;
        int typ= CONTAINER->type;
        TID=id;

      uint32_t return_code = 0;
      if (receive_buffer->len >= 12) {
        return_code = 0x2001;  // Ok
        receive_buffer->len = 12;
        
        if (typ == 1) { // command
          switch (op) {
            case 0x1001: // GetDescription
              TRANSMIT(WriteDescriptor());
              break;
            case 0x1002:  // OpenSession
              openSession(p1);
              break;
            case 0x1003:  // CloseSession
              break;
            case 0x1004:  // GetStorageIDs
              TRANSMIT(WriteStorageIDs());
              break;
            case 0x1005:  // GetStorageInfo
              TRANSMIT(GetStorageInfo(p1));
              break;
            case 0x1006:  // GetNumObjects
              if (p2) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                p1 = GetNumObjects(p1, p3);
              }
              break;
            case 0x1007:  // GetObjectHandles
              if (p2) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                TRANSMIT(GetObjectHandles(p1, p3));
              }
              break;
            case 0x1008:  // GetObjectInfo
              TRANSMIT(GetObjectInfo(p1));
              break;
            case 0x1009:  // GetObject
              TRANSMIT(GetObject(p1));
              break;
            case 0x100B:  // DeleteObject
              if (p2) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                if (!storage_->DeleteObject(p1)) {
                  return_code = 0x2012; // partial deletion
                }
              }
              break;
            case 0x100C:  // SendObjectInfo
              p3 =  SendObjectInfo(p1, // storage
                                   p2); // parent
              CONTAINER->params[1]=p2;
              CONTAINER->params[2]=p3;
              len = receive_buffer->len = 12 + 3 * 4;
              break;
            case 0x100D:  // SendObject
              SendObject();
              break;
            case 0x1014:  // GetDevicePropDesc
              TRANSMIT(GetDevicePropDesc(p1));
              break;
            case 0x1015:  // GetDevicePropvalue
              TRANSMIT(GetDevicePropValue(p1));
              break;
              
          case 0x1010:  // Reset
              return_code = 0x2005;
              break;

          case 0x1019:  // MoveObject
              return_code = moveObject(p1,p2,p3);
              len  = receive_buffer->len = 12;
              break;

          case 0x101A:  // CopyObject
              return_code = copyObject(p1,p2,p3);
              if(! return_code) { len  = receive_buffer->len = 12; return_code = 0x2005; }
              else {p1 = return_code; return_code=0x2001;}
              break;

          case 0x9801:  // getObjectPropsSupported
              TRANSMIT(getObjectPropsSupported(p1));
              break;

          case 0x9802:  // getObjectPropDesc
            TRANSMIT(getObjectPropDesc(p1,p2));
              break;

          case 0x9803:  // getObjectPropertyValue
            TRANSMIT(getObjectPropValue(p1,p2));
              break;

          case 0x9804:  // setObjectPropertyValue
              return_code = setObjectPropValue(p1,p2);
              break;
              
            default:
              return_code = 0x2005;  // operation not supported
              break;
          }
        } else {
          return_code = 0x2000;  // undefined
        }
      }
      if (return_code) {
        CONTAINER->type=3;
        CONTAINER->len=len;
        CONTAINER->op=return_code;
        CONTAINER->transaction_id=id;
        CONTAINER->params[0]=p1;
        #if DEBUG>1
          printContainer();
        #endif

        usb_tx(MTP_TX_ENDPOINT, receive_buffer);
        receive_buffer = 0;
      } else {
          usb_free(receive_buffer);
      }
    }
    // Maybe put event handling inside mtp_yield()?
    if ((receive_buffer = usb_rx(MTP_EVENT_ENDPOINT))) {
      printf("Event: "); printContainer();
      usb_free(receive_buffer);
    }
  }

#elif defined(__IMXRT1062__)  

    void MTPD::write_init(uint16_t op, uint32_t transaction_id, uint32_t length)
    {
      MTPHeader header;                                   
      header.len = length_ + sizeof(header);        
      header.type = 2;                                    
      header.op = op;                          
      header.transaction_id = transaction_id; 
      write_length_ = 0;
      write((char *)&header, sizeof(header));      
    }

    void MTPD::write_finish(void)
    {
      if(write_length_ > 0)
      {
        push_packet(tx_data_buffer,rest);
      }
    }

    void MTPD::writeo(const char *data, int len) 
    { 
        static uint8_t *dst=0;
        if(write_length_==0) dst=tx_data_buffer;  // start of buffer 
        write_length_ += len;
        
        const char * src=data;
        //
        int pos = 0; // into data
        while(pos<len)
        { int avail = tx_data_buffer+MTP_TX_SIZE - dst; 
          int to_copy = min(len - pos, avail);
          memcpy(dst,src,to_copy);
          pos += to_copy;
          src += to_copy;
          dst += to_copy;
          if(dst == tx_data_buffer+MTP_TX_SIZE) //end of buffer
          { push_packet(tx_data_buffer,MTP_TX_SIZE);
            dst=tx_data_buffer;
          }
        }
    }

    void MTPD::getObjecto(uint16_t op, uint32_t transaction_id, int32_t object_id)
    {
      uint32_t size = storage_->GetSize(object_id);
      uint32_t offset = 0;
      uint32_t nread = 0;
      write_init(op, transacion_id, size);
      while(offset<size)
      {
            nread=min(size-offset,(uint32_t)DISK_BUFFER_SIZE);
            storage_->read(object_id,offset,(char *)disk_buffer, nread);
            offset += nread;
            if(nread==DISK_BUFFER_SIZE)
            {
              writeo(disk_buffer,DISK_BUFFER_SIZE);
            }
      }
      if(nread>0)
        writeo(disk_buffer,nread);
      write_finish();
    }


    void MTPD::write(const char *data, int len) 
    { if (write_get_length_) 
      {
        write_length_ += len;
      } 
      else 
      { 
        static uint8_t *dst=0;
        if(!write_length_) dst=tx_data_buffer;   
        write_length_ += len;
        
        const char * src=data;
        //
        int pos = 0; // into data
        while(pos<len)
        { int avail = tx_data_buffer+MTP_TX_SIZE - dst; 
          int to_copy = min(len - pos, avail);
          memcpy(dst,src,to_copy);
          pos += to_copy;
          src += to_copy;
          dst += to_copy;
          if(dst == tx_data_buffer+MTP_TX_SIZE) //end of buffer
          { push_packet(tx_data_buffer,MTP_TX_SIZE);
            dst=tx_data_buffer;
          }
        }
      }
    }

    void MTPD::GetObject(uint32_t object_id) 
    {
      uint32_t size = storage_->GetSize(object_id);
      if (write_get_length_) {
        write_length_ += size;
      } else 
      { 
        //printf("%x %d\n",object_id, size);
        uint32_t pos = 0; // into data
        uint32_t len = sizeof(MTPHeader); // after header

        disk_pos=DISK_BUFFER_SIZE;
        while(pos<size)
        {
          if(disk_pos==DISK_BUFFER_SIZE)
          {
            uint32_t nread=min(size-pos,(uint32_t)DISK_BUFFER_SIZE);
            storage_->read(object_id,pos,(char *)disk_buffer,nread);
            disk_pos=0; // flag that we read buffer
          }

          uint32_t to_copy = min(size-pos, MTP_TX_SIZE-len);  // fill up mtx_tx_buffer
          to_copy = min (to_copy, DISK_BUFFER_SIZE-disk_pos); // but not more than we have remaining in disk buffer

          memcpy(tx_data_buffer+len, disk_buffer+disk_pos, to_copy);
          disk_pos += to_copy;
          pos += to_copy;
          len += to_copy;

          if (len==MTP_TX_SIZE)
          {
            push_packet(tx_data_buffer,MTP_TX_SIZE);
            len=0;
          }
        }
      }
    }

    uint32_t MTPD::GetPartialObject(uint32_t object_id, uint32_t offset, uint32_t NumBytes) 
    {
      uint32_t size = storage_->GetSize(object_id);

      size -= offset;
      if(NumBytes == 0xffffffff) NumBytes=size;
      if (NumBytes<size) size=NumBytes;

      if (write_get_length_) {
        write_length_ += size;
      } else 
      { 
        uint32_t pos = offset; // into data
        uint32_t len = sizeof(MTPHeader);

        disk_pos=DISK_BUFFER_SIZE;
        while(pos<size)
        {
          if(disk_pos==DISK_BUFFER_SIZE)
          {
            uint32_t nread=min(size-pos,(uint32_t)DISK_BUFFER_SIZE);
            storage_->read(object_id,pos,(char *)disk_buffer,nread);
            disk_pos=0;
          }

          uint32_t to_copy = min(size-pos,MTP_TX_SIZE-len);
          to_copy = min (to_copy, DISK_BUFFER_SIZE-disk_pos);

          memcpy(tx_data_buffer+len,disk_buffer+disk_pos,to_copy);
          disk_pos += to_copy;
          pos += to_copy;
          len += to_copy;

          if(len==MTP_TX_SIZE)
          { push_packet(tx_data_buffer,MTP_TX_SIZE);
            len=0;
          }
        }
      }
      return size;
    }

    #define CONTAINER ((struct MTPContainer*)(rx_data_buffer))

    #define TRANSMIT(FUN) do {                            \
      write_length_ = 0;                                  \
      write_get_length_ = true;                           \
      FUN;                                                \
      \
      MTPHeader header;                                   \
      header.len = write_length_ + sizeof(header);        \
      header.type = 2;                                    \
      header.op = CONTAINER->op;                          \
      header.transaction_id = CONTAINER->transaction_id;  \
      write_length_ = 0;                                  \
      write_get_length_ = false;                          \
      write((char *)&header, sizeof(header));             \
      FUN;                                                \
      \
      uint32_t rest;                                      \
      rest = (header.len % MTP_TX_SIZE);                  \
      if(rest>0)                                          \
      {                                                   \
        push_packet(tx_data_buffer,rest);                 \
      }                                                   \
    } while(0)
  

    #define TRANSMIT1(FUN) do {                           \
      write_length_ = 0;                                  \
      write_get_length_ = true;                           \
      uint32_t dlen = FUN;                                \
      \
      MTPContainer header;                                   \
      header.len = write_length_ + sizeof(MTPHeader);      \
      header.type = 2;                                    \
      header.op = CONTAINER->op;                          \
      header.transaction_id = CONTAINER->transaction_id;  \
      header.params[0]=dlen;                              \
      write_length_ = 0;                                  \
      write_get_length_ = false;                          \
      write((char *)&header, sizeof(header));             \
      FUN;                                                \
      \
      uint32_t rest;                                      \
      rest = (header.len % MTP_TX_SIZE);                  \
      if(rest>0)                                          \
      {                                                   \
        push_packet(tx_data_buffer,rest);                        \
      }                                                   \
    } while(0)


    #define printContainer() \
    { printf("%x %d %d %d: ", CONTAINER->op, CONTAINER->len, CONTAINER->type, CONTAINER->transaction_id); \
      if(CONTAINER->len>12) printf(" %x", CONTAINER->params[0]); \
      if(CONTAINER->len>16) printf(" %x", CONTAINER->params[1]); \
      if(CONTAINER->len>20) printf(" %x", CONTAINER->params[2]); \
      printf("\r\n"); \
    }


    void MTPD::read(char* data, uint32_t size) 
    {
      static int index=0;
      if(!size) 
      {
        index=0;
        return;
      }

      while (size) {
        uint32_t to_copy = MTP_RX_SIZE - index;
        to_copy = min(to_copy, size);
        if (data) {
          memcpy(data, rx_data_buffer + index, to_copy);
          data += to_copy;
        }
        size -= to_copy;
        index += to_copy;
        if (index == MTP_RX_SIZE) {
          pull_packet(rx_data_buffer);
          index=0;
        }
      }
    }

    uint32_t MTPD::SendObjectInfo(uint32_t storage, uint32_t parent) {
      pull_packet(rx_data_buffer);
      read(0,0); // resync read
//      printContainer(); 
      uint32_t store = Storage2Store(storage);

      int len=ReadMTPHeader();
      char filename[MAX_FILENAME_LEN];

      read32(); len -=4; // storage
      bool dir = (read16() == 0x3001); len -=2; // format
      read16(); len -=2; // protection
      read32(); len -=4; // size
      read16(); len -=2; // thumb format
      read32(); len -=4; // thumb size
      read32(); len -=4; // thumb width
      read32(); len -=4; // thumb height
      read32(); len -=4; // pix width
      read32(); len -=4; // pix height
      read32(); len -=4; // bit depth
      read32(); len -=4; // parent
      read16(); len -=2; // association type
      read32(); len -=4; // association description
      read32(); len -=4; // sequence number
      readstring(filename); len -= (2*(strlen(filename)+1)+1); 
      // ignore rest of ObjectInfo
      while(len>=4) { read32(); len-=4;}
      while(len) {read8(); len--;}

      return storage_->Create(store, parent, dir, filename);
    }

    bool MTPD::SendObject() 
    { 
      pull_packet(rx_data_buffer);
      read(0,0);
//      printContainer(); 

      uint32_t len = ReadMTPHeader();
      uint32_t index = sizeof(MTPHeader);
      disk_pos=0;
      
      while((int)len>0)
      { uint32_t bytes = MTP_RX_SIZE - index;                     // how many data in usb-packet
        bytes = min(bytes,len);                                   // loimit at end
        uint32_t to_copy=min(bytes, DISK_BUFFER_SIZE-disk_pos);   // how many data to copy to disk buffer
        memcpy(disk_buffer+disk_pos, rx_data_buffer + index,to_copy);
        disk_pos += to_copy;
        bytes -= to_copy;
        len -= to_copy;
        //printf("a %d %d %d %d %d\n", len,disk_pos,bytes,index,to_copy);
        //
        if(disk_pos==DISK_BUFFER_SIZE)
        {
          if(storage_->write((const char *)disk_buffer, DISK_BUFFER_SIZE)<DISK_BUFFER_SIZE) return false;
          disk_pos =0;

          if(bytes) // we have still data in transfer buffer, copy to initial disk_buffer
          {
            memcpy(disk_buffer,rx_data_buffer+index+to_copy,bytes);
            disk_pos += bytes;
            len -= bytes;
          }
          //printf("b %d %d %d %d %d\n", len,disk_pos,bytes,index,to_copy);
        }
        if(len>0)  // we have still data to be transfered
        { pull_packet(rx_data_buffer);
          index=0;
        }
      }
      //printf("len %d\n",disk_pos);
      if(disk_pos)
      {
        if(storage_->write((const char *)disk_buffer, disk_pos)<disk_pos) return false;
      }
      storage_->close();
      return true;
    }

    uint32_t MTPD::setObjectPropValue(uint32_t handle, uint32_t p2)
    { pull_packet(rx_data_buffer);
      read(0,0);
      //printContainer(); 
         
      if(p2==0xDC07)
      { 
        char filename[MAX_FILENAME_LEN]; 
        ReadMTPHeader();
        readstring(filename);
        if(storage_->rename(handle,filename)) return 0x2001; else return 0x2005;
      }
      else
        return 0x2005;
    }

    void MTPD::loop(void)
    { if(!usb_mtp_available()) return;
      if(fetch_packet(rx_data_buffer))
      { printContainer(); // to switch on set debug to 1 at beginning of file

        int op = CONTAINER->op;
        int p1 = CONTAINER->params[0];
        int p2 = CONTAINER->params[1];
        int p3 = CONTAINER->params[2];
        int id = CONTAINER->transaction_id;
        int len= CONTAINER->len;
        int typ= CONTAINER->type;
        TID=id;

        int return_code =0x2001; //OK use as default value

        if(typ==2) return_code=0x2005; // we should only get cmds

        switch (op)
        {
          case 0x1001:
            TRANSMIT(WriteDescriptor());
            break;

          case 0x1002:  //open session
            openSession(p1);
            break;

          case 0x1003:  // CloseSession
            break;

          case 0x1004:  // GetStorageIDs
              TRANSMIT(WriteStorageIDs());
            break;

          case 0x1005:  // GetStorageInfo
            TRANSMIT(GetStorageInfo(p1));
            break;

          case 0x1006:  // GetNumObjects
            if (p2) 
            {
                return_code = 0x2014; // spec by format unsupported
            } else 
            {
                p1 = GetNumObjects(p1, p3);
            }
            break;

          case 0x1007:  // GetObjectHandles
            if (p2) 
            { return_code = 0x2014; // spec by format unsupported
            } else 
            { 
              TRANSMIT(GetObjectHandles(p1, p3));
            }
            break;

          case 0x1008:  // GetObjectInfo
            TRANSMIT(GetObjectInfo(p1));
            break;

          case 0x1009:  // GetObject
            TRANSMIT(GetObject(p1));
            //getObject(op,id,p1);
            break;

          case 0x100B:  // DeleteObject
              if (p2) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                if (!storage_->DeleteObject(p1)) {
                  return_code = 0x2012; // partial deletion
                }
              }
              break;

          case 0x100C:  // SendObjectInfo
              p3 = SendObjectInfo(p1, // storage
                                  p2); // parent

              CONTAINER->params[1]=p2;
              CONTAINER->params[2]=p3;
              len = 12 + 3 * 4;
              break;

          case 0x100D:  // SendObject
              if(!SendObject()) return_code = 0x2005;
              len = 12;
              break;

          case 0x1014:  // GetDevicePropDesc
              TRANSMIT(GetDevicePropDesc(p1));
              break;

          case 0x1015:  // GetDevicePropvalue
              TRANSMIT(GetDevicePropValue(p1));
              break;

          case 0x1010:  // Reset
              return_code = 0x2005;
              break;

          case 0x1019:  // MoveObject
              return_code = moveObject(p1,p2,p3);
              len = 12;
              break;

          case 0x101A:  // CopyObject
              return_code = copyObject(p1,p2,p3);
              if(!return_code) 
              { return_code=0x2005; len = 12; }
              else
              { p1 = return_code; return_code=0x2001; len = 16;  }
              break;

          case 0x101B:  // GetPartialObject
              TRANSMIT1(GetPartialObject(p1,p2,p3));
              break;

          case 0x9801:  // getObjectPropsSupported
              TRANSMIT(getObjectPropsSupported(p1));
              break;

          case 0x9802:  // getObjectPropDesc
              TRANSMIT(getObjectPropDesc(p1,p2));
              break;

          case 0x9803:  // getObjectPropertyValue
              TRANSMIT(getObjectPropValue(p1,p2));
              break;

          case 0x9804:  // setObjectPropertyValue
              return_code = setObjectPropValue(p1,p2);
              break;

          default:
              return_code = 0x2005;  // operation not supported
              break;
        }
        if(return_code)
        {
            CONTAINER->type=3;
            CONTAINER->len=len;
            CONTAINER->op=return_code;
            CONTAINER->transaction_id=id;
            CONTAINER->params[0]=p1;
            #if DEBUG >1
              printContainer(); // to switch on set debug to 2 at beginning of file
            #endif

            memcpy(tx_data_buffer,rx_data_buffer,len);
            push_packet(tx_data_buffer,len); // for acknowledge use rx_data_buffer
        }
      }
    }
#endif


//***************************************************************************
//  USB bulk endpoint low-level input & output
//***************************************************************************

#if defined(__MK20DX128__) || defined(__MK20DX256__) ||  defined(__MK64FX512__) || defined(__MK66FX1M0__)

//  usb_packet_t *data_buffer_ = NULL;
  void MTPD::get_buffer() {
    while (!data_buffer_) {
      data_buffer_ = usb_malloc();
      if (!data_buffer_) mtp_yield();
    }
  }

  void MTPD::receive_buffer() {
    while (!data_buffer_) {
      data_buffer_ = usb_rx(MTP_RX_ENDPOINT);
      if (!data_buffer_) mtp_yield();
    }
  }


bool MTPD_class::receive_bulk(uint32_t timeout) { // T3
  elapsedMillis msec = 0;
  while (msec <= timeout) {
    usb_packet_t *packet = usb_rx(MTP_RX_ENDPOINT);
    if (packet) {
      receive_buffer.len = packet->len;
      receive_buffer.index = 0;
      receive_buffer.size = sizeof(packet->buf);
      receive_buffer.data = packet->buf;
      receive_buffer.usb = packet;
      return true;
    }
  }
  return false;
}

void MTPD_class::free_received_bulk() { // T3
  if (receive_buffer.usb) {
    usb_free((usb_packet_t *)receive_buffer.usb);
  }
  receive_buffer.len = 0;
  receive_buffer.data = NULL;
  receive_buffer.usb = NULL;
}

void MTPD_class::allocate_transmit_bulk() { // T3
  while (1) {
    usb_packet_t *packet = usb_malloc();
    if (packet) {
      transmit_buffer.len = 0;
      transmit_buffer.index = 0;
      transmit_buffer.size = sizeof(packet->buf);
      transmit_buffer.data = packet->buf;
      transmit_buffer.usb = packet;
      return;
    }
    mtp_yield();
  }
}

int MTPD_class::transmit_bulk() { // T3
  usb_packet_t *packet = (usb_packet_t *)transmit_buffer.usb;
  int len = transmit_buffer.len;
  packet->len = len;
  write_transfer_open = (len == 64);
  usb_tx(MTP_TX_ENDPOINT, packet);
  transmit_buffer.len = 0;
  transmit_buffer.index = 0;
  transmit_buffer.data = NULL;
  transmit_buffer.usb = NULL;
  return len;
}

// TODO: core library not yet implementing cancel on Teensy 3.x
uint8_t MTP_class::usb_mtp_status = 0x01;


#elif defined(__IMXRT1062__)

    int MTPD::push_packet(uint8_t *data_buffer,uint32_t len)
    {
      while(usb_mtp_send(data_buffer,len,60)<=0) ;
      return 1;
    }

    int MTPD::pull_packet(uint8_t *data_buffer)
    {
      while(!usb_mtp_available());
      return usb_mtp_recv(data_buffer,60);
    }

    int MTPD::fetch_packet(uint8_t *data_buffer)
    {
      return usb_mtp_recv(data_buffer,60);
    }

#endif // __IMXRT1062__


#if USE_EVENTS==1

 #if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)

  #include "usb_mtp.h"
  extern "C"
  {
    usb_packet_t *tx_event_packet=NULL;

    int usb_init_events(void)
    {
      tx_event_packet = usb_malloc();
      if(tx_event_packet) return 1; else return 0; 
    }


    int usb_mtp_sendEvent(const void *buffer, uint32_t len, uint32_t timeout)
    {
      if (!usb_configuration) return -1;
      memcpy(tx_event_packet->buf, buffer, len);
      tx_event_packet->len = len;
      usb_tx(MTP_EVENT_ENDPOINT, tx_event_packet);
      return len;
    }
  }

  #elif defined(__IMXRT1062__)
  // keep this here until cores is upgraded 

  #include "usb_mtp.h"
  extern "C"
  {
    static transfer_t tx_event_transfer[1] __attribute__ ((used, aligned(32)));
    static uint8_t tx_event_buffer[MTP_EVENT_SIZE] __attribute__ ((used, aligned(32)));

    static transfer_t rx_event_transfer[1] __attribute__ ((used, aligned(32)));
    static uint8_t rx_event_buffer[MTP_EVENT_SIZE] __attribute__ ((used, aligned(32)));

    static uint32_t mtp_txEventcount=0;
    static uint32_t mtp_rxEventcount=0;

    uint32_t get_mtp_txEventcount() {return mtp_txEventcount; }
    uint32_t get_mtp_rxEventcount() {return mtp_rxEventcount; }
    
    static void txEvent_event(transfer_t *t) { mtp_txEventcount++;}
    static void rxEvent_event(transfer_t *t) { mtp_rxEventcount++;}

    int usb_init_events(void)
    {
        usb_config_tx(MTP_EVENT_ENDPOINT, MTP_EVENT_SIZE, 0, txEvent_event);
        //	
        usb_config_rx(MTP_EVENT_ENDPOINT, MTP_EVENT_SIZE, 0, rxEvent_event);
        usb_prepare_transfer(rx_event_transfer + 0, rx_event_buffer, MTP_EVENT_SIZE, 0);
        usb_receive(MTP_EVENT_ENDPOINT, rx_event_transfer + 0);
        return 1;
    }

    static int usb_mtp_wait(transfer_t *xfer, uint32_t timeout)
    {
      uint32_t wait_begin_at = systick_millis_count;
      while (1) {
        if (!usb_configuration) return -1; // usb not enumerated by host
        uint32_t status = usb_transfer_status(xfer);
        if (!(status & 0x80)) break; // transfer descriptor ready
        if (systick_millis_count - wait_begin_at > timeout) return 0;
        yield();
      }
      return 1;
    }

    int usb_mtp_recvEvent(void *buffer, uint32_t len, uint32_t timeout)
    {
      int ret= usb_mtp_wait(rx_event_transfer, timeout); if(ret<=0) return ret;

      memcpy(buffer, rx_event_buffer, len);
      memset(rx_event_transfer, 0, sizeof(rx_event_transfer));

      NVIC_DISABLE_IRQ(IRQ_USB1);
      usb_prepare_transfer(rx_event_transfer + 0, rx_event_buffer, MTP_EVENT_SIZE, 0);
      usb_receive(MTP_EVENT_ENDPOINT, rx_event_transfer + 0);
      NVIC_ENABLE_IRQ(IRQ_USB1);
      return MTP_EVENT_SIZE;
    }

    int usb_mtp_sendEvent(const void *buffer, uint32_t len, uint32_t timeout)
    {
      transfer_t *xfer = tx_event_transfer;
      int ret= usb_mtp_wait(xfer, timeout); if(ret<=0) return ret;

      uint8_t *eventdata = tx_event_buffer;
      memcpy(eventdata, buffer, len);
      usb_prepare_transfer(xfer, eventdata, len, 0);
      usb_transmit(MTP_EVENT_ENDPOINT, xfer);
      return len;
    }
  }
  #endif

  const uint32_t EVENT_TIMEOUT=60;

  int MTPD::send_Event(uint16_t eventCode)
  {
    MTPContainer event;
    event.len = 12;
    event.op =eventCode ;
    event.type = MTP_CONTAINER_TYPE_EVENT; 
    event.transaction_id=TID;
    event.params[0]=0;
    event.params[1]=0;
    event.params[2]=0;
    return usb_mtp_sendEvent((const void *) &event, event.len, EVENT_TIMEOUT);
  }
  int MTPD::send_Event(uint16_t eventCode, uint32_t p1)
  {
    MTPContainer event;
    event.len = 16;
    event.op =eventCode ;
    event.type = MTP_CONTAINER_TYPE_EVENT; 
    event.transaction_id=TID;
    event.params[0]=p1;
    event.params[1]=0;
    event.params[2]=0;
    return usb_mtp_sendEvent((const void *) &event, event.len, EVENT_TIMEOUT);
  }
  int MTPD::send_Event(uint16_t eventCode, uint32_t p1, uint32_t p2)
  {
    MTPContainer event;
    event.len = 20;
    event.op =eventCode ;
    event.type = MTP_CONTAINER_TYPE_EVENT; 
    event.transaction_id=TID;
    event.params[0]=p1;
    event.params[1]=p2;
    event.params[2]=0;
    return usb_mtp_sendEvent((const void *) &event, event.len, EVENT_TIMEOUT);
  }
  int MTPD::send_Event(uint16_t eventCode, uint32_t p1, uint32_t p2, uint32_t p3)
  {
    MTPContainer event;
    event.len = 24;
    event.op =eventCode ;
    event.type = MTP_CONTAINER_TYPE_EVENT; 
    event.transaction_id=TID;
    event.params[0]=p1;
    event.params[1]=p2;
    event.params[2]=p3;
    return usb_mtp_sendEvent((const void *) &event, event.len, EVENT_TIMEOUT);
  }

  int MTPD::send_DeviceResetEvent(void) 
  { return send_Event(MTP_EVENT_DEVICE_RESET); } 

  // following WIP
  int MTPD::send_StorageInfoChangedEvent(uint32_t p1) 
  { return send_Event(MTP_EVENT_STORAGE_INFO_CHANGED, Store2Storage(p1));}

  // following not tested
  int MTPD::send_addObjectEvent(uint32_t p1) 
  { return send_Event(MTP_EVENT_OBJECT_ADDED, p1); }
  int MTPD::send_removeObjectEvent(uint32_t p1) 
  { return send_Event(MTP_EVENT_OBJECT_REMOVED, p1); }

#endif


#if MPT_MODE==1
/************************************************************************************************ */
// Define global(static) members
uint32_t MTPD_class::sessionID_ = 0;

#if defined(__IMXRT1062__)
DMAMEM 
#endif
uint8_t MTPD_class::disk_buffer_[DISK_BUFFER_SIZE] __attribute__((aligned(32)));

static int utf8_strlen(const char *str) {
  if (!str) return 0;
  int len=0, count=0;
  while (1) {
    unsigned int c = *str++;
    if (c == 0) return len;
    if ((c & 0x80) == 0) {
      len++;
      count = 0;
    } else if ((c & 0xC0) == 0x80 && count > 0) {
      if (--count == 0) len++;
    } else if ((c & 0xE0) == 0xC0) {
      count = 1;
    } else if ((c & 0xF0) == 0xE0) {
      count = 2;
    } else {
      count = 0;
    }
  }
}

void MTPD_class::writestring(const char* str) 
  {
    if (*str) 
    {   write8(utf8_strlen(str) + 1); 
        while (*str) {  write16(*str);  ++str;  } 
        write16(0);
    } else 
    { write8(0);
    }
  }
uint32_t MTPD_class::writestringlen(const char *str) {
  int len = strlen(str);
  if (len == 0) return 1;
  return len*2 + 2 + 1;
}


void MTPD_class::loop(void) 
{
  if (receive_bulk(0)) 
  {
    if ((receive_buffer.len >= 12) && (receive_buffer.len <= 32)) 
    {
      // This container holds the operation code received from host
      // Commands which transmit a 12 byte header as the first part
      // of their data phase will reuse this container, overwriting
      // the len & type fields, but keeping op and transaction_id.
      // Then this container is again reused to transmit the final
      // response code, keeping the original transaction_id, but
      // the other 3 header fields are based on "return_code".  If
      // the response requires parameters, they are written into
      // this container's parameter list.
      struct MTPContainer container;
      memset(&container, 0, sizeof(container));
      memcpy(&container, receive_buffer.data, receive_buffer.len);
      free_received_bulk();
      //printContainer(&container, "loop:");

      //int p1 = container.params[0];
      //int p2 = container.params[1];
      //int p3 = container.params[2];
      TID = container.transaction_id;

      // The low 16 bits of return_code have the response code
      // operation field.  The top 4 bits indicate the number
      // of parameters to transmit with the response code.
      int return_code = MTP_RESPONSE_OK; // use as default value
      //bool send_reset_event = false;

      if (container.type == MTP_CONTAINER_TYPE_COMMAND) 
      {
        switch (container.op) 
        {
          case 0x1001: // GetDeviceInfo
            return_code = getDeviceInfo(container);
            break;
          case 0x1002: // OpenSession
            return_code = openSession(container);
            break;
          case 0x1003: // CloseSession
            printf("MTP_class::CloseSession\n");
            sessionID_ = 0; //
            break;
          case 0x1004: // GetStorageIDs
            return_code = getStorageIDs(container);
            break;
          case 0x1005: // GetStorageInfo
            return_code = getStorageInfo(container);
            break;
          case 0x1006: // GetNumObjects
            return_code = getNumObjects(container);
            break;
          case 0x1007: // GetObjectHandles
            return_code = getObjectHandles(container);
            break;
          case 0x1008: // GetObjectInfo
            return_code = getObjectInfo(container);
            break;
          case 0x1009: // GetObject
            return_code = getObject(container);
            break;
          case 0x100B: // DeleteObject
            return_code = deleteObject(container);
            break;
          case 0x100C: // SendObjectInfo
            //return_code = SendObjectInfo(container);
            break;
          case 0x100D: // SendObject
            //return_code = SendObject(container);
            break;
          case 0x100F: // FormatStore
            return_code = formatStore(container);
            //if (return_code == MTP_RESPONSE_OK) send_reset_event = true;
            break;
          case 0x1014: // GetDevicePropDesc
            return_code = getDevicePropDesc(container);
            break;
          case 0x1015: // GetDevicePropvalue
            return_code = getDevicePropValue(container);
            break;
          case 0x1010: // Reset
            return_code = MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
            break;
          case 0x1019: // MoveObject
            return_code = moveObject(container);
            break;
          case 0x101A: // CopyObject
            return_code = copyObject(container);
            break;
          case 0x101B: // GetPartialObject
            return_code = getPartialObject(container);
            break;
          case 0x9801: // GetObjectPropsSupported
            return_code = getObjectPropsSupported(container);
            break;
          case 0x9802: // GetObjectPropDesc
            return_code = getObjectPropDesc(container);
            break;
          case 0x9803: // GetObjectPropertyValue
            return_code = getObjectPropValue(container);
            break;
          case 0x9804: // setObjectPropertyValue
            return_code = setObjectPropValue(container);
            break;
          default:
            return_code = MTP_RESPONSE_OPERATION_NOT_SUPPORTED; 
            break;
        }
      } 
      else 
      {
        return_code = MTP_RESPONSE_OPERATION_NOT_SUPPORTED; // we should only get cmds
        //printContainer(&container, "!!! unexpected/unknown message:");
      }

      // send trailing response to terminate reponder action
      if (return_code && usb_mtp_status == 0x01) 
      {
        container.len = 12 + (return_code >> 28) * 4; // top 4 bits is number of parameters
        container.type = MTP_CONTAINER_TYPE_RESPONSE;
        container.op = (return_code & 0xFFFF);        // low 16 bits is op response code
        // container.transaction_id reused from original received command
        #if DEBUG > 1
          //printContainer(&container); // to switch on set debug to 2 at beginning of file
        #endif
        write(&container, container.len);
        write_finish();

        // Maybe some operations might need to tell host to do reset
        // right now try after a format store.
        //if (send_reset_event)  send_DeviceResetEvent();

      }
    } 
    else 
    {
      printf("ERROR: loop received command with %u bytes\n", receive_buffer.len);
      free_received_bulk();
      // TODO: what is the proper way to handle this error?
      // Still Image Class spec 1.0 says on page 20:
      //   "If the number of bytes transferred in the Command phase is less than
      //    that specified in the first four bytes of the Command Block then the
      //    device has received an invalid command and should STALL the Bulk-Pipe
      //    (refer to Clause 7.2)."
      // What are we supposed to do is too much data arrives?  Or other invalid cmds?
    }
  }

  // check here to mske sure the USB status is reset
  if (usb_mtp_status != 0x01) {
    printf("MTP_class::Loop usb_mtp_status %x != 0x1 reset\n", usb_mtp_status);
    usb_mtp_status = 0x01;
  }

  // Storage loop() handles removable media insert / remove
  //storage_->loop();
}


//***************************************************************************
//  MTP Commands - Metadata, Capability Detection, and Boring Stuff
//***************************************************************************

// GetDeviceInfo, MTP 1.1 spec, page 210
//   Command: no parameters
//   Data: Teensy->PC: DeviceInfo
//   Response: no parameters
uint32_t MTPD_class::getDeviceInfo(struct MTPContainer &cmd)  
{
    const char *msoft="microsoft.com: 1.0;";

    char buf1[20];    
    dtostrf( (float)(TEENSYDUINO / 100.0f), 3, 2, buf1);
    strlcat(buf1, " / MTP " MTP_VERS, sizeof(buf1) );
    
    char buf2[20];    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
    for (size_t i=0; i<10; i++) buf2[i] = usb_string_serial_number.wString[i];
    #pragma GCC diagnostic pop

    uint32_t num_bytes=0;
    num_bytes=2+4+2+strlen(msoft)+2+ \
        4+supported_op_num*2+4+supported_event_num*2+ \
        4+2+4+4+2+2+strlen(MTP_MANUF)+strlen(MTP_MODEL)+ \
        strlen(buf1)+strlen(buf2);

    write_init(cmd,num_bytes);
    write16(100);  // MTP version
    write32(6);    // MTP extension
    write16(100);  // MTP version
    writestring(msoft);
    write16(0);    // functional mode

    // Supported operations (array of uint16)
    write32(supported_op_num);
    write(supported_op,sizeof(supported_op));
    //for(int ii=0; ii<supported_op_num;ii++) write16(supported_op[ii]);
    
    // Events (array of uint16)
    write32(supported_event_num);      
    write(supported_events,sizeof(supported_events));
    //for(int ii=0; ii<supported_event_num;ii++) write16(supported_events[ii]);

    write32(1);       // Device properties (array of uint16)
    write16(0xd402);  // Device friendly name

    write32(0);       // Capture formats (array of uint16)

    write32(2);       // Playback formats (array of uint16)
    write16(0x3000);  // Undefined format
    write16(0x3001);  // Folders (associations)

    writestring(MTP_MANUF);     // Manufacturer
    writestring(MTP_MODEL);     // Model
    
    writestring(buf1);    
    writestring(buf2); 
    write_finish();
    return MTP_RESPONSE_OK;   
  }

  // GetDevicePropDesc, MTP 1.1 spec, page 233
  //   Command: 1 parameter: DevicePropCode
  //   Data: Teensy->PC: DevicePropDesc
  //   Response: no parameters
  uint32_t MTPD_class::getDevicePropDesc(struct MTPContainer &cmd)   {
    const uint32_t property = cmd.params[0];
    uint32_t num_bytes=0;
    switch (property) {
      case 0xd402: // friendly name
        num_bytes=2+2+1+2*writestringlen(MTP_NAME)+1;
        write_init(cmd,num_bytes);
        write16(property);
        write16(0xFFFF); // string type
        write8(0);       // read-only
        writestring(MTP_NAME);
        writestring(MTP_NAME);
        write8(0);       // no form
        write_finish();
        return MTP_RESPONSE_OK;
    }
    write_init(cmd,num_bytes);
    return MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
  }

  // OpenSession, MTP 1.1 spec, page 211
  //   Command: 1 parameter: SessionID
  //   Data: none
  //   Response: no parameters
  uint32_t MTPD_class::openSession(struct MTPContainer &cmd) 
  {
    sessionID_ = cmd.params[0];
    //storage_->ResetIndex();
    return MTP_RESPONSE_OK;
  }

  // GetStorageIDs, MTP 1.1 spec, page 213
  //   Command: no parameters
  //   Data: Teensy->PC: StorageID array (page 45)
  //   Response: no parameters
  uint32_t MTPD_class::getStorageIDs(struct MTPContainer &cmd)
  {
    uint32_t num=storage_->get_FSCount();
    uint32_t num_bytes = 0;
    num_bytes= 4+num*4;

    write_init(cmd,num_bytes);
    write32(num); // number of storages (disks)
    for(uint32_t ii=0;ii<num;ii++)  write32(Store2Storage(ii)); // storage id
    write_finish();
    return MTP_RESPONSE_OK;   
  }


  // GetStorageInfo, MTP 1.1 spec, page 214
  //   Command: 1 parameter: StorageID
  //   Data: Teensy->PC: StorageInfo (page 46)
  //   Response: no parameters
  uint32_t MTPD_class::getStorageInfo(struct MTPContainer &cmd) 
  {
    uint32_t storage = cmd.params[0];
    uint32_t store = Storage2Store(storage);
    const char *name = storage_->get_FSName(store);

    uint64_t ntotal = storage_->totalSize(store) ; 
    uint64_t nused = storage_->usedSize(store) ; 

    uint32_t num_bytes=0;
    num_bytes=2+2+2+8+8+4+strlen(name)+strlen("");

    write_init(cmd,num_bytes);
    write16(storage_->readonly(store) ? 0x0001 : 0x0004);   // storage type (removable RAM)
    write16(storage_->has_directories(store) ? 0x0002: 0x0001);   // filesystem type (generic hierarchical)
    write16(0x0000);   // access capability (read-write)

    write64(ntotal);  // max capacity
    write64((ntotal-nused));  // free space (100M)
    //
    write32(0xFFFFFFFFUL);  // free space (objects)
    writestring(name);  // storage descriptor
    writestring("");  // volume identifier
    write_finish();
    return MTP_RESPONSE_OK;   
  }

  // GetObjectInfo, MTP 1.1 spec, page 218
  //   Command: 1 parameter: ObjectHandle
  //   Data: Teensy->PC: ObjectInfo
  //   Response: no parameters
  uint32_t MTPD_class::getObjectInfo(struct MTPContainer &cmd) 
  {
    uint32_t handle = cmd.params[0];

    char filename[MAX_FILENAME_LEN];
    uint32_t size, parent;
    uint16_t store;
    char create[64],modify[64];
    storage_->GetObjectInfo(handle, filename, &size, &parent, &store, create, modify);

    uint32_t storage = Store2Storage(store);
    uint32_t num_bytes=0;

    num_bytes=4+2+2+4+2+4+4+4+4+4+4+4+2+4+4+ \
        strlen(filename)+strlen(create)+strlen(modify)+strlen("");
    write_init(cmd,num_bytes);
    write32(storage); // storage
    write16(size == 0xFFFFFFFFUL ? 0x3001 : 0x0000); // format
    write16(0);  // protection
    write32(size); // size
    write16(0); // thumb format
    write32(0); // thumb size
    write32(0); // thumb width
    write32(0); // thumb height
    write32(0); // pix width
    write32(0); // pix height
    write32(0); // bit depth
    write32(parent); // parent
    write16(size == 0xFFFFFFFFUL ? 1 : 0); // association type
    write32(0); // association description
    write32(0);  // sequence number
    writestring(filename);
    writestring(create);  // date created
    writestring(modify);  // date modified
    writestring("");  // keywords
    write_finish();
    return MTP_RESPONSE_OK;
  }

  // GetNumObjects, MTP 1.1 spec, page 215
  //   Command: 3 parameters: StorageID, [ObjectFormatCode], [Parent folder]
  //   Data: none
  //   Response: 1 parameter: NumObjects
  uint32_t MTPD_class::getNumObjects(struct MTPContainer &cmd)
  { uint32_t storage = cmd.params[0];
    uint32_t format = cmd.params[1];
    uint32_t parent = cmd.params[2];
    if (format) return MTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED;
    
    uint32_t store = Storage2Store(storage);
    storage_->StartGetObjectHandles(store, parent);
    int num = 0;
    while (storage_->GetNextObjectHandle(store)) num++;
    cmd.params[0] = num;
    return MTP_RESPONSE_OK | (1<<28);
  }

  // GetObjectHandles, MTP 1.1 spec, page 217
  //   Command: 3 parameters: StorageID, [ObjectFormatCode], [Parent folder]
  //   Data: Teensy->PC: ObjectHandle array
  //   Response: no parameters
  uint32_t MTPD_class::getObjectHandles(struct MTPContainer &cmd) 
  { uint32_t storage = cmd.params[0];
    uint32_t format = cmd.params[1];
    uint32_t parent = cmd.params[2];
    if (format) {
      write_init(cmd, 4);
      write32(0); // empty ObjectHandle array
      write_finish();
      return MTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED;
    }
    uint32_t store = Storage2Store(storage);
    
    storage_->StartGetObjectHandles(store, parent);
    int numObjects = 0;
    while (storage_->GetNextObjectHandle(store)) numObjects++;
    //
    uint32_t num_bytes =0;
    num_bytes=4+numObjects*4;
    write_init(cmd,num_bytes);
    write32(numObjects);
    int handle;
    storage_->StartGetObjectHandles(store, parent);
    while ((handle = storage_->GetNextObjectHandle(store))) write32(handle);
    write_finish();
    return MTP_RESPONSE_OK;
  }

  // GetDevicePropValue, MTP 1.1 spec, page 234
  //   Command: 1 parameter: DevicePropCode
  //   Data: Teensy->PC: DeviceProp Value
  //   Response: no parameters
  uint32_t MTPD_class::getDevicePropValue(struct MTPContainer &cmd) {
    const uint32_t property = cmd.params[0];
    switch (property) 
    {
      case 0xd402: // friendly name
        write_init(cmd,writestringlen(MTP_NAME));
        // This is the name we'll actually see in the windows explorer.
        // Should probably be configurable.
        writestring(MTP_NAME);
        write_finish();
        return MTP_RESPONSE_OK;
        break;
    }
    write_init(cmd, 0);
    return MTP_RESPONSE_DEVICE_PROP_NOT_SUPPORTED;
  }

  // GetObjectPropsSupported, MTP 1.1 spec, page 243
  //   Command: 1 parameter: ObjectFormatCode
  //   Data: Teensy->PC: ObjectPropCode Array
  //   Response: no parameters
  uint32_t MTPD_class::getObjectPropsSupported(struct MTPContainer &cmd) 
  {
    write_init(cmd, 4 + sizeof(propertyList));
    write32(propertyListNum);
    write(propertyList, sizeof(propertyList));
    write_finish();
    return MTP_RESPONSE_OK;
}

  // GetObjectPropDesc, MTP 1.1 spec, page 244
  //   Command: 2 parameter: ObjectPropCode, Object Format Code
  //   Data: Teensy->PC: ObjectPropDesc
  //   Response: no parameters
  uint32_t MTPD_class::getObjectPropDesc(struct MTPContainer &cmd) 
  {
    uint32_t property = cmd.params[0];

    uint32_t num_bytes=0;
    switch(property)
    {
      case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
        num_bytes=2+2+1+4+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC01);
        write16(0x006);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FORMAT:        //0xDC02:
        num_bytes=2+2+1+2+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC02);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PROTECTION_STATUS:    //0xDC03:
        num_bytes=2+2+1+2+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC03);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
        num_bytes=2+2+1+8+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC04);
        write16(0x008);
        write8(0); //get
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
        num_bytes=2+2+1+1+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC07);
        write16(0xFFFF);
        write8(1); //get/set
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
        num_bytes=2+2+1+1+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC08);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
        num_bytes=2+2+1+1+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC09);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PARENT_OBJECT:    //0xDC0B:
        num_bytes=2+2+1+4+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC0B);
        write16(6);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PERSISTENT_UID:   //0xDC41:
        num_bytes=2+2+1+8+8+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC41);
        write16(0x0A);
        write8(0); //get
        write64(0);
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_NAME:             //0xDC44:
        num_bytes=2+2+1+1+4+1;
        write_init(cmd,num_bytes);
        write16(0xDC44);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      default:
        num_bytes=0;
        write_init(cmd,num_bytes);
        break;
    }
    write_finish();
    return MTP_RESPONSE_OK;
  }

// GetObjectPropValue, MTP 1.1 spec, page 245
//   Command: 2 parameters: ObjectHandle, ObjectPropCode
//   Data: Teensy->PC: ObjectProp Value
//   Response: no parameters
uint32_t MTPD_class::getObjectPropValue(struct MTPContainer &cmd) {
  const uint32_t handle = cmd.params[0];
  const uint32_t property = cmd.params[1];
      
      char name[MAX_FILENAME_LEN];
      uint32_t dir;
      uint32_t filesize;
      uint32_t parent;
      uint16_t store;
      char create[64], modify[64];

      storage_->GetObjectInfo(handle,name,&filesize,&parent, &store, create, modify);
      dir = (filesize == 0xFFFFFFFFUL);
      uint32_t storage = Store2Storage(store);
      //
      switch(property)
      { 
        case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
          write_init(cmd, 4);
          write32(storage);
          break;
        case MTP_PROPERTY_OBJECT_FORMAT:      //0xDC02:
          write_init(cmd, 2);
          write16(dir?0x3001:0x3000);
          break;
        case MTP_PROPERTY_PROTECTION_STATUS:  //0xDC03:
          write_init(cmd, 2);
          write16(0);
          break;
        case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
          write_init(cmd, 4+4);
          write32(filesize);
          write32(0);
          break;
        case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
          write_init(cmd, writestringlen(name));
          writestring(name);
          break;
        case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
          write_init(cmd, writestringlen(create));
          writestring(create);
          break;
        case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
          write_init(cmd, writestringlen(modify));
          writestring(modify);
          break;
        case MTP_PROPERTY_PARENT_OBJECT:      //0xDC0B:
          write_init(cmd, 4);
          write32((store==parent)? 0: parent);
          break;
        case MTP_PROPERTY_PERSISTENT_UID:     //0xDC41:
          write_init(cmd, 4+4+4+4);
          write32(property);
          write32(parent);
          write32(storage);
          break;
        case MTP_PROPERTY_NAME:               //0xDC44:
          write_init(cmd, writestringlen(name));
          writestring(name);
          break;
        default:
          write_init(cmd, 0);
          break;
      }
    write_finish();
    return MTP_RESPONSE_OK;
  }

#if 0
uint32_t MTPD_class::getObjectPropValue(struct MTPContainer &cmd) {
  const uint32_t handle = cmd.params[0];
  const uint32_t property = cmd.params[1];

  uint32_t data_size = 0;
  char filename[MAX_FILENAME_LEN];
  uint64_t size;
  uint32_t parent;
  uint16_t store;
  uint32_t dt;
  DateTimeFields dtf;
  uint32_t storage = 0;
  bool info_needed = true;

  char create[64],modify[64];
  storage_->GetObjectInfo(handle, filename, &size, &parent, &store, create, modify);

  // first check if storage info is needed, and if data size is known
  switch (property) 
  {
  case MTP_PROPERTY_DATE_CREATED: // 0xDC08:
    if (storage_->getCreateTime(handle, dt)) {
      breakTime(dt, dtf);
      snprintf(name, MAX_FILENAME_LEN, "%04u%02u%02uT%02u%02u%02u",
               dtf.year + 1900, dtf.mon + 1, dtf.mday, dtf.hour, dtf.min,
               dtf.sec);
    } else {
      name[0] = 0;
    }
    data_size = writestringlen(name);
    info_needed = false;
    break;
  case MTP_PROPERTY_DATE_MODIFIED: // 0xDC09:
    if (storage_->getModifyTime(handle, dt)) {
      breakTime(dt, dtf);
      snprintf(name, MAX_FILENAME_LEN, "%04u%02u%02uT%02u%02u%02u",
               dtf.year + 1900, dtf.mon + 1, dtf.mday, dtf.hour, dtf.min,
               dtf.sec);
    } else {
      name[0] = 0;
    }
    data_size = writestringlen(name);
    info_needed = false;
    break;
  }

  // get actual storage info, if needed
  if (info_needed) {
    storage_->GetObjectInfo(handle, name, &file_size, &parent, &store);
    if (property == MTP_PROPERTY_OBJECT_FILE_NAME || property == MTP_PROPERTY_NAME) {
      data_size = writestringlen(name);
    }
    storage = Store2Storage(store);
  }

  // begin data phase
  write_init(cmd, data_size);
  if (data_size == 0) {
    write_finish(); // TODO: remove this when change to split header/data
    return MTP_RESPONSE_INVALID_OBJECT_PROP_CODE;
  }

  // transmit actual data
  switch (property) {
  case MTP_PROPERTY_STORAGE_ID: // 0xDC01:
    data_size=4;
    write_init(cmd, data_size);
    write32(storage);
    break;
  case MTP_PROPERTY_OBJECT_FORMAT: // 0xDC02:
    data_size=2;
    write_init(cmd, data_size);
    write16((file_size == (uint64_t)-1) ? 0x3001 /*directory*/ : 0x3000 /*file*/);
    break;
  case MTP_PROPERTY_PROTECTION_STATUS: // 0xDC03:
    data_size=2;
    write_init(cmd, data_size);
    write16(0);
    break;
  case MTP_PROPERTY_OBJECT_SIZE: // 0xDC04:
    data_size=8;
    write_init(cmd, data_size);
    write64(file_size);
    printf("\tMTP_PROPERTY_OBJECT_SIZE: %s %llx\n", name, file_size);
    //write32(file_size & 0xfffffffful);
    //write32(file_size >> 32);
    break;
  case MTP_PROPERTY_OBJECT_FILE_NAME: // 0xDC07:
  case MTP_PROPERTY_NAME: // 0xDC44:
    data_size=strlen(name);
    write_init(cmd, data_size);
    writestring(name);
    break;
  case MTP_PROPERTY_DATE_CREATED: // 0xDC08:
  case MTP_PROPERTY_DATE_MODIFIED: // 0xDC09:
    data_size=strlen(name);
    write_init(cmd, data_size);
    writestring(name);
    break;
  case MTP_PROPERTY_PARENT_OBJECT: // 0xDC0B:
    data_size=4;
    write_init(cmd, data_size);
    write32((store == parent) ? 0 : parent);
    break;
  case MTP_PROPERTY_PERSISTENT_UID: // 0xDC41:
    data_size=4+4+4+4;
    write_init(cmd, data_size);
    write32(handle);
    write32(parent);
    write32(storage);
    write32(0);
    break;
  }
  write_finish();
  return MTP_RESPONSE_OK;
}
#endif //0


//  GetObject, MTP 1.1 spec, page 219
//   Command: 1 parameter: ObjectHandle
//   Data: Teensy->PC: Binary Data
//   Response: no parameters
uint32_t MTPD_class::getObject(struct MTPContainer &cmd) {
  uint16_t cb_read = 0;
  uint64_t disk_pos = 0;

  const int object_id = cmd.params[0];
  uint64_t size = storage_->GetSize(object_id);
  uint64_t count_remaining = size;

  write_init(cmd, (size > 0xfffffffful)?  0xfffffffful : size); // limit single size to 32 bit??

  while (count_remaining) 
  {
    if (usb_mtp_status != 0x01) {  printf("GetObject, abort status:%x\n", usb_mtp_status);  return 0;  }

    // Lets make it real simple for now.
//    if(!(cb_read = storage_->read(object_id, disk_pos, (char*)disk_buffer_, sizeof(disk_buffer_)))) ;
//    if (cb_read == 0) break;
    cb_read=sizeof(disk_buffer_);
    storage_->read(object_id, disk_pos, (char*)disk_buffer_, cb_read) ;
    
    size_t cb_written = write(disk_buffer_, cb_read);
    if (cb_written != cb_read) break;

    count_remaining -= cb_read;
    disk_pos += cb_read;
  }
  write_finish();
  return MTP_RESPONSE_OK;
}


//  GetPartialObject, MTP 1.1 spec, page 240
//   Command: 3 parameters: ObjectHandle, Offset in bytes, Maximum number of bytes
//   Data: Teensy->PC: binary data
//   Response: 1 parameter: Actual number of bytes sent
uint32_t MTPD_class::getPartialObject(struct MTPContainer &cmd) 
{
  uint32_t object_id = cmd.params[0];
  uint32_t offset = cmd.params[1];
  uint64_t NumBytes = cmd.params[2];

  uint16_t cb_read = 0;

  if (NumBytes == 0xfffffffful) NumBytes = (uint64_t)-1;
  uint64_t size = storage_->GetSize(object_id);
  
  if (offset >= size) 
  { // writeDataPhaseHeader(cmd, 0); ???
    return MTP_RESPONSE_INVALID_PARAMETER;
  }
  
  if (NumBytes > size - offset)   NumBytes = size - offset; // limit to rest of file
  
  write_init(cmd, NumBytes);

  while (NumBytes) 
  {
    if (usb_mtp_status != 0x01) {  printf("GetObject, abort status:%x\n", usb_mtp_status);  return 0;  }

    // Lets make it real simple for now.
    //if(!(cb_read = storage_->read(object_id, disk_pos, (char*)disk_buffer_, sizeof(disk_buffer_)))) ;
    //if (cb_read == 0) break;
    cb_read=sizeof(disk_buffer_);
    storage_->read(object_id, offset, (char*)disk_buffer_, cb_read) ;


    size_t cb_written = write(disk_buffer_, cb_read);
    if (cb_written != cb_read) break;

    NumBytes -= cb_read;
    offset += cb_read;
  }
  write_finish();
  cmd.params[0] = (NumBytes < 0xfffffffful)? NumBytes : 0xfffffffful;
  return MTP_RESPONSE_OK + (1<<28);
}

//  DeleteObject, MTP 1.1 spec, page 221
//   Command: 2 parameters: ObjectHandle, [ObjectFormatCode]
//   Data: none
//   Response: no parameters
uint32_t MTPD_class::deleteObject(struct MTPContainer &cmd) 
{
  uint32_t handle = cmd.params[0];
  uint32_t format= cmd.params[1];
  if (format) return MTP_RESPONSE_SPECIFICATION_BY_FORMAT_UNSUPPORTED;
  //
  if (!storage_->DeleteObject(handle)) {
    return 0x2012; // partial deletion
  }
  return MTP_RESPONSE_OK;
}

//  MoveObject, MTP 1.1 spec, page 238
//   Command: 3 parameters: ObjectHandle, StorageID target, ObjectHandle of the new ParentObject
//   Data: none
//   Response: no parameters
uint32_t MTPD_class::moveObject(struct MTPContainer &cmd) 
{
  uint32_t handle = cmd.params[0];
  uint32_t newStorage= cmd.params[1];  
  uint32_t newHandle= cmd.params[2];  

  uint32_t store1 = Storage2Store(newStorage);
  if (newHandle == 0) newHandle = store1;

  if (storage_->move(handle, store1, newHandle))
    return MTP_RESPONSE_OK;
  else
    return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}

//  CopyObject, MTP 1.1 spec, page 239
//   Command: 3 parameters: ObjectHandle, StorageID target, ObjectHandle of the new ParentObject
//   Data: none
//   Response: no parameters
uint32_t MTPD_class::copyObject(struct MTPContainer &cmd) 
{
  uint32_t handle = cmd.params[0];
  uint32_t newStorage= cmd.params[1];  
  uint32_t newHandle= cmd.params[2];  
  
  uint32_t store1 = Storage2Store(newStorage);
  if (newHandle == 0) newHandle = store1;

  if (storage_->copy(handle, store1, newHandle))
    return MTP_RESPONSE_OK;
  else
    return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}

//  FormatStore, MTP 1.1 spec, page 228
//   Command: 2 parameters: StorageID, [FileSystem Format]
//   Data: none
//   Response: no parameters
uint32_t MTPD_class::formatStore(struct MTPContainer &cmd) 
{
  const uint32_t store = Storage2Store(cmd.params[0]);
  const uint32_t format = cmd.params[1];
  //To be done
  (void) store;
  (void) format;
  return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}

#if 0
bool MTPD_class::read_init(struct MTPHeader *header) {
  if (!read(header, sizeof(struct MTPHeader))) return false;
  if (header && header->type != MTP_CONTAINER_TYPE_DATA) return false;
  // TODO: when we later implement split header + data USB optimization
  //       described in MTP 1.1 spec pages 281-282, we should check that
  //       receive_buffer.data is NULL.  Return false if unexpected data.
  return true;
}

bool MTPD_class::readstring(char *buffer, uint32_t buffer_size) {
  uint8_t len;
  if (!read8(&len)) return false;
  if (len == 0) {
    if (buffer) *buffer = 0; // empty string
    return true;
  }
  unsigned int buffer_index = 0;
  for (unsigned int string_index = 0; string_index < len; string_index++) {
    uint16_t c;
    if (!read16(&c)) return false;
    if (string_index == (unsigned)(len-1) && c != 0) return false; // last char16 must be zero
    if (buffer) {
      // encode Unicode16 -> UTF8
      if (c < 0x80 && buffer_index < buffer_size-2) {
        buffer[buffer_index++] = c & 0x7F;
      } else if (c < 0x800 && buffer_index < buffer_size-3) {
        buffer[buffer_index++] = 0xC0 | ((c >> 6) & 0x1F);
        buffer[buffer_index++] = 0x80 | (c & 0x3F);
      } else if (buffer_index < buffer_size-4) {
        buffer[buffer_index++] = 0xE0 | ((c >> 12) & 0x0F);
        buffer[buffer_index++] = 0x80 | ((c >> 6) & 0x3F);
        buffer[buffer_index++] = 0x80 | (c & 0x3F);
      } else {
        while (buffer_index < buffer_size) buffer[buffer_index++] = 0;
        buffer = nullptr;
      }
    }
  }
  if (buffer) buffer[buffer_index] = 0; // redundant?? (last char16 must be zero)
  return true;
}

bool MTPD_class::readDateTimeString(uint32_t *pdt) {
  char dtb[20]; // let it take care of the conversions.
  if (!readstring(dtb, sizeof(dtb))) return false;
  if (dtb[0] == 0) {
    // host sent empty string, use time from Teensy's RTC
    *pdt = Teensy3Clock.get();
    return true;
  }
  //printf("  DateTime string: %s\n", dtb);
  //                            01234567890123456
  // format of expected String: YYYYMMDDThhmmss.s
  if (strlen(dtb) < 15) return false;
  for (int i=0; i < 15; i++) {
    if (i == 8) {
      if (dtb[i] != 'T') return false;
    } else {
      if (dtb[i] < '0' || dtb[i] > '9') return false;
    }
  }
  DateTimeFields dtf;
  // Quick and dirty!
  uint16_t year = ((dtb[0] - '0') * 1000) + ((dtb[1] - '0') * 100) +
                  ((dtb[2] - '0') * 10) + (dtb[3] - '0');
  dtf.year = year - 1900;                               // range 70-206
  dtf.mon = ((dtb[4] - '0') * 10) + (dtb[5] - '0') - 1; // zero based not 1
  dtf.mday = ((dtb[6] - '0') * 10) + (dtb[7] - '0');
  dtf.wday = 0; // hopefully not needed...
  dtf.hour = ((dtb[9] - '0') * 10) + (dtb[10] - '0');
  dtf.min = ((dtb[11] - '0') * 10) + (dtb[12] - '0');
  dtf.sec = ((dtb[13] - '0') * 10) + (dtb[14] - '0');
  *pdt = makeTime(dtf);
  //printf(">> date/Time: %x %u/%u/%u %u:%u:%u\n", *pdt, dtf.mon + 1, dtf.mday,
         //year, dtf.hour, dtf.min, dtf.sec);
  return true;
}


bool MTPD_class::read(void *ptr, uint32_t size) {
  char *data = (char *)ptr;
  while (size > 0) {
    if (receive_buffer.data == NULL) {
      if (!receive_bulk(100)) {
        if (data) memset(data, 0, size);
        return false;
      }
    }
    // TODO: what happens if read spans multiple packets?  Do any cases exist?
    uint32_t to_copy = receive_buffer.len - receive_buffer.index;
    if (to_copy > size) to_copy = size;
    if (data) {
      memcpy(data, receive_buffer.data + receive_buffer.index, to_copy);
      data += to_copy;
    }
    size -= to_copy;
    receive_buffer.index += to_copy;
    if (receive_buffer.index >= receive_buffer.len) {
      free_received_bulk();
    }
  }
  return true;
}



// SetObjectPropValue, MTP 1.1 spec, page 246
//   Command: 2 parameters: ObjectHandle, ObjectPropCode
//   Data: PC->Teensy: ObjectProp Value
//   Response: no parameters
uint32_t MTPD_class::setObjectPropValue(struct MTPContainer &cmd) 
{
  uint32_t object_id = cmd.params[0];
  uint32_t property_code = cmd.params[1];

  struct MTPHeader header;
  if (!read_init(&header)) return MTP_RESPONSE_INVALID_DATASET;

  if (property_code == 0xDC07) 
  {
    char filename[MAX_FILENAME_LEN];
    if (readstring(filename, sizeof(filename)) && (true)) 
    { 
    storage_->rename(object_id, filename);
      return MTP_RESPONSE_OK;
    } 
    else 
    {
      return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
    }
  }
  read(NULL, header.len - sizeof(header)); // discard ObjectProp Value
  return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}
#endif //0

// SendObjectInfo, MTP 1.1 spec, page 223
//   Command: 2 parameters: Destination StorageID, Parent ObjectHandle
//   Data: PC->Teensy: ObjectInfo
//   Response: 3 parameters: Destination StorageID, Parent ObjectHandle, ObjectHandle
 // MTP 1.1 spec, page 223
uint32_t MTPD_class::sendObjectInfo(struct MTPContainer &cmd) 
{ uint32_t storage = cmd.params[0];
  uint32_t parent = cmd.params[1];

  uint32_t store = Storage2Store(storage);
  
  struct MTPHeader header;
  if (!read_init(&header)) return MTP_RESPONSE_INVALID_DATASET;

  // receive ObjectInfo Dataset, MTP 1.1 spec, page 50
  char filename[MAX_FILENAME_LEN];
  uint16_t oformat;
  uint32_t file_size;
  if (!(read(NULL, 4)                          // StorageID (unused)
   && read16(&oformat)                       // ObjectFormatCode
   && read(NULL, 2)                          // Protection Status (unused)
   && read32(&file_size)                     // Object Compressed Size
   && read(NULL, 40)                         // Image info (unused)
   && readstring(filename, sizeof(filename)) // Filename
   && readDateTimeString(&dtCreated_)        // Date Created
   && readDateTimeString(&dtModified_)       // Date Modified
   && readstring(NULL, 0)                    // Keywords
   && (true))) 
    return MTP_RESPONSE_INVALID_DATASET;

  // Lets see if we have enough room to store this file:
  uint64_t free_space = storage_->totalSize(store) - storage_->usedSize(store);
  if (file_size == 0xFFFFFFFFUL) {
    printf("Size of object == 0xffffffff - Indicates >= 4GB file!\n \t?TODO: query real size? FS supports this - FAT32 no?\n");
  }

  if (file_size > free_space) 
  {
    printf("Size of object:%u is > free space: %llu\n", file_size, free_space);
    return MTP_RESPONSE_STORAGE_FULL;
  }

  const bool dir = (oformat == 0x3001);
  object_id_ = storage_->Create(store, parent, dir, filename);
  if (object_id_ == 0xFFFFFFFFUL) 
  {
    return MTP_RESPONSE_SPECIFICATION_OF_DESTINATION_UNSUPPORTED;
  }

  if (dir) 
  {
    // lets see if we should update the date and time stamps.
    // if it is dirctory, then sendObject will not be called, so do it now.
    #if 0
    if (!storage_->updateDateTimeStamps(object_id_, dtCreated_, dtModified_)) 
    {
      // BUGBUG: failed to update, maybe FS needs little time to settle in
      // before trying this.
      for (uint8_t i = 0; i < 10; i++) 
      {
        printf("!!!(%d) Try delay and call update time stamps again\n", i);
        delay(25);
        if (storage_->updateDateTimeStamps(object_id_, dtCreated_, dtModified_)) break;
      }
    }
    #endif

  storage_->close();
  }
  cmd.params[2] = object_id_;
  return MTP_RESPONSE_OK | (3<<28); // response with 3 params
}

// SendObject, MTP 1.1 spec, page 225
//   Command: no parameters
//   Data: PC->Teensy: Binary Data
//   Response: no parameters
uint32_t MTPD_class::sendObject(struct MTPContainer &cmd) 
{
  MTPHeader header;
  if (!read_init(&header)) return MTP_RESPONSE_PARAMETER_NOT_SUPPORTED;

  uint64_t size = header.len - sizeof(header);
  printf("SendObject: %llu(0x%llx) bytes, id=%x\n", size, size, object_id_);
  // TODO: check size matches file_size from SendObjectInfo
  // TODO: check if object_id_
  // TODO: should we do storage_->Create() here?  Can we preallocate file size?
  uint32_t ret = MTP_RESPONSE_OK;
  uint64_t pos = 0;
  //uint32_t count_reads = 0;
  //uint32_t to_copy_prev = 0;

  // index into our disk buffer.

  bool huge_file = (size == 0xfffffffful);
  if (huge_file) size = (uint64_t)-1;
  uint64_t cb_left = size;

  // lets go ahead and copy the rest of the first receive buffer into
  // our disk buffer, so we don't have to play with starting index and the like...
  uint16_t disk_buffer_index = receive_buffer.len - receive_buffer.index;
  memcpy((char*)disk_buffer_, (char *)&receive_buffer.data[receive_buffer.index], disk_buffer_index);
  pos = disk_buffer_index;
  free_received_bulk();


  while (huge_file || (pos < size)) 
  {
    if (!receive_bulk(100)) 
    {
      if (pos <= 0xfffffffful) {
        printf("SO: receive failed pos:%llu size:%llu\n", pos, size);
        ret = MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
      } else {
        printf("SO: receive failed pos:%llu large file EOF\n", pos);
      }
      break;
    }

    uint32_t to_copy = receive_buffer.len;

    uint16_t cb_buffer_avail = sizeof(disk_buffer_) - disk_buffer_index;

    // See if this will fill the buffer;
    if (cb_buffer_avail <= to_copy) 
    { // have more data than buffer
      memcpy(&disk_buffer_[disk_buffer_index], (char*)&receive_buffer.data[receive_buffer.index], cb_buffer_avail);
      disk_buffer_index = 0;

      if (storage_->write((char*)disk_buffer_, sizeof(disk_buffer_)) != sizeof(disk_buffer_)) 
      {
        ret = MTP_RESPONSE_OPERATION_NOT_SUPPORTED; // good response?
        break;
      }

      if (cb_buffer_avail != to_copy) 
      {
        // copy in the remaining.
        disk_buffer_index = to_copy - cb_buffer_avail;
        memcpy(disk_buffer_, (char*)&receive_buffer.data[cb_buffer_avail], disk_buffer_index);
      }
    } 
    else
    { // copy all to buffer
      memcpy(&disk_buffer_[disk_buffer_index], (char*)receive_buffer.data, to_copy);
      disk_buffer_index += to_copy;  
    }

    pos += to_copy;
    cb_left -= to_copy; // 

    free_received_bulk();
    if ((to_copy < 512) && (size == (uint64_t)-1) && (pos > 0xfffffffful)){
      printf("SendObject large EOF Detected: %lluu\n", pos);
      break;
    }
  }

  // clear out any trailing. 
  while (pos < size) 
  {
    // consume remaining incoming data, if we aborted for any reason
    if (receive_buffer.data == NULL && !receive_bulk(250)) break; 
    uint16_t cb_packet = receive_buffer.len - receive_buffer.index;   
    pos += cb_packet;
    free_received_bulk();
    if (cb_packet < 512) break;
  }

  // write out anything left in our disk buffer... 
  if (disk_buffer_index) 
  {
    if (storage_->write((char*)disk_buffer_, disk_buffer_index) != disk_buffer_index) 
    {
      ret = MTP_RESPONSE_OPERATION_NOT_SUPPORTED; 
    }
  }

  #if 0
  storage_->updateDateTimeStamps(object_id_, dtCreated_, dtModified_); // to be implemented
  #endif
storage_->close();

  if (ret == MTP_RESPONSE_OK) object_id_ = 0; // SendObjectInfo can not be reused after success
  return ret;
}

// SetObjectPropValue, MTP 1.1 spec, page 246
//   Command: 2 parameters: ObjectHandle, ObjectPropCode
//   Data: PC->Teensy: ObjectProp Value
//   Response: no parameters
#if 0
uint32_t MTPD_class::setObjectPropValue(struct MTPContainer &cmd) 
{
  uint32_t object_id = cmd.params[0];
  uint32_t property_code = cmd.params[1];

  struct MTPHeader header;
  if (!read_init(&header)) return MTP_RESPONSE_INVALID_DATASET;

  if (property_code == 0xDC07) 
  {
    char filename[MTP_MAX_FILENAME_LEN];
    if (readstring(filename, sizeof(filename)) && (true)) 
    {   
    storage_->rename(object_id, filename);
      return MTP_RESPONSE_OK;
    } 
    else 
    {
      return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
    }
  }
  read(NULL, header.len - sizeof(header)); // discard ObjectProp Value
  return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}
#endif


//***************************************************************************
//  Generic data read / write
//***************************************************************************
bool MTPD_class::read_init(struct MTPHeader *header) 
{
  if (!read(header, sizeof(struct MTPHeader))) return false;
  if (header && header->type != MTP_CONTAINER_TYPE_DATA) return false;
  // TODO: when we later implement split header + data USB optimization
  //       described in MTP 1.1 spec pages 281-282, we should check that
  //       receive_buffer.data is NULL.  Return false if unexpected data.
  return true;
}

bool MTPD_class::readstring(char *buffer, uint32_t buffer_size) 
{
  uint8_t len;
  if (!read8(&len)) return false;
  if (len == 0) {
    if (buffer) *buffer = 0; // empty string
    return true;
  }

  unsigned int buffer_index = 0;
  for (unsigned int string_index = 0; string_index < len; string_index++) 
  {
    uint16_t c;
    if (!read16(&c)) return false;
    if (string_index == (unsigned)(len-1) && c != 0) return false; // last char16 must be zero
    if (buffer) 
    {
      // encode Unicode16 -> UTF8
      if (c < 0x80 && buffer_index < buffer_size-2) {
        buffer[buffer_index++] = c & 0x7F;
      } else if (c < 0x800 && buffer_index < buffer_size-3) {
        buffer[buffer_index++] = 0xC0 | ((c >> 6) & 0x1F);
        buffer[buffer_index++] = 0x80 | (c & 0x3F);
      } else if (buffer_index < buffer_size-4) {
        buffer[buffer_index++] = 0xE0 | ((c >> 12) & 0x0F);
        buffer[buffer_index++] = 0x80 | ((c >> 6) & 0x3F);
        buffer[buffer_index++] = 0x80 | (c & 0x3F);
      } else {
        while (buffer_index < buffer_size) buffer[buffer_index++] = 0;
        buffer = nullptr;
      }
    }
  }
  if (buffer) buffer[buffer_index] = 0; // redundant?? (last char16 must be zero)
  return true;
}

bool MTPD_class::readDateTimeString(uint32_t *pdt) {
  char dtb[20]; // let it take care of the conversions.
  if (!readstring(dtb, sizeof(dtb))) return false;

  if (dtb[0] == 0) {
    // host sent empty string, use time from Teensy's RTC
    //*pdt = Teensy3Clock.get(); // use 0 for now (rtc_get();)
    return true;
  }

  //printf("  DateTime string: %s\n", dtb);
  //                            01234567890123456
  // format of expected String: YYYYMMDDThhmmss.s
  if (strlen(dtb) < 15) return false;
  for (int i=0; i < 15; i++) {
    if (i == 8) {
      if (dtb[i] != 'T') return false;
    } else {
      if (dtb[i] < '0' || dtb[i] > '9') return false;
    }
  }
  DateTimeFields dtf;
  // Quick and dirty!
  uint16_t year = ((dtb[0] - '0') * 1000) + ((dtb[1] - '0') * 100) +
                  ((dtb[2] - '0') * 10) + (dtb[3] - '0');
  dtf.year = year - 1900;                               // range 70-206
  dtf.mon = ((dtb[4] - '0') * 10) + (dtb[5] - '0') - 1; // zero based not 1
  dtf.mday = ((dtb[6] - '0') * 10) + (dtb[7] - '0');
  dtf.wday = 0; // hopefully not needed...
  dtf.hour = ((dtb[9] - '0') * 10) + (dtb[10] - '0');
  dtf.min = ((dtb[11] - '0') * 10) + (dtb[12] - '0');
  dtf.sec = ((dtb[13] - '0') * 10) + (dtb[14] - '0');
  *pdt = makeTime(dtf);
  //printf(">> date/Time: %x %u/%u/%u %u:%u:%u\n", *pdt, dtf.mon + 1, dtf.mday,
         //year, dtf.hour, dtf.min, dtf.sec);
  return true;
}

bool MTPD_class::read(void *ptr, uint32_t size) 
{
  char *data = (char *)ptr;
  while (size > 0) {
    if (receive_buffer.data == NULL) {
      if (!receive_bulk(100)) 
      { if (data) memset(data, 0, size);
         return false;
      }
    }
    // TODO: what happens if read spans multiple packets?  Do any cases exist?
    uint32_t to_copy = receive_buffer.len - receive_buffer.index;
    if (to_copy > size) to_copy = size;
    if (data) {
      memcpy(data, receive_buffer.data + receive_buffer.index, to_copy);
      data += to_copy;
    }
    size -= to_copy;
    receive_buffer.index += to_copy;
    if (receive_buffer.index >= receive_buffer.len) {
      free_received_bulk();
    }
  }
  return true;
}

size_t MTPD_class::write(const void *ptr, size_t len) {
  size_t len_in = len;
  const char *data = (const char *)ptr;
  while (len > 0) 
  {
    if (usb_mtp_status != 0x01) { printf("write, abort\n");  return 0;  }

    if (transmit_buffer.data == NULL) allocate_transmit_bulk();
    unsigned int avail = transmit_buffer.size - transmit_buffer.len;
    unsigned int to_copy = len;
    if (to_copy > avail) to_copy = avail;
    memcpy(transmit_buffer.data + transmit_buffer.len, data, to_copy);
    data += to_copy;
    len -= to_copy;
    transmit_buffer.len += to_copy;
    if (transmit_buffer.len >= transmit_buffer.size) {
      transmit_bulk();
    }
  }
  return len_in; // for now we are not detecting errors.
}

void MTPD_class::write_init(struct MTPContainer &container, uint32_t data_size)
{
  container.len = data_size + 12;
  container.type = MTP_CONTAINER_TYPE_DATA;
  // container.op reused from received command container
  // container.transaction_id reused from received command container
  write(&container, 12);
  // TODO: when we later implement split header + data USB optimization
  //       described in MTP 1.1 spec pages 281-282, we will need to
  //       call transmit_bulk() here to transmit a partial packet
}

void MTPD_class::write_finish() {
  if (transmit_buffer.data == NULL) {
    if (!write_transfer_open) return;
    printf("send a ZLP\n");
    allocate_transmit_bulk();
  }
  transmit_bulk();
}


// SetObjectPropValue, MTP 1.1 spec, page 246
//   Command: 2 parameters: ObjectHandle, ObjectPropCode
//   Data: PC->Teensy: ObjectProp Value
//   Response: no parameters
uint32_t MTPD_class::setObjectPropValue(struct MTPContainer &cmd) {
  uint32_t object_id = cmd.params[0];
  uint32_t property_code = cmd.params[1];

  struct MTPHeader header;
  if (!read_init(&header)) return MTP_RESPONSE_INVALID_DATASET;

  if (property_code == 0xDC07) {
    char filename[MAX_FILENAME_LEN];
    if (readstring(filename, sizeof(filename))
     && (true)) {   // TODO: read complete function (handle ZLP)
      printf("setObjectPropValue, rename id=%x to \"%s\"\n", object_id, filename);
    storage_->rename(object_id, filename);
      return MTP_RESPONSE_OK;
    } else {
      return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
    }
  }
  read(NULL, header.len - sizeof(header)); // discard ObjectProp Value
  return MTP_RESPONSE_OPERATION_NOT_SUPPORTED;
}

  bool MTPD_class::receive_bulk(uint32_t timeout) { // T4
    if (usb_mtp_status != 0x01) {
      receive_buffer.data = NULL;
      return false;
    }

    receive_buffer.index = 0;
    receive_buffer.size = MTP_RX_SIZE;
    receive_buffer.usb = NULL;
    receive_buffer.len = usb_mtp_recv(rx_data_buffer, timeout);

    if (receive_buffer.len > 0) {
      // FIXME: need way to receive ZLP
      receive_buffer.data = rx_data_buffer;
      return true;
    } else {
      receive_buffer.data = NULL;
      return false;
    }
  }

  void MTPD_class::free_received_bulk() { // T4
    receive_buffer.len = 0;
    receive_buffer.data = NULL;
  }

  void MTPD_class::allocate_transmit_bulk() { // T4
    transmit_buffer.len = 0;
    transmit_buffer.index = 0;
    transmit_buffer.size = usb_mtp_txSize();
    transmit_buffer.data = tx_data_buffer;
    transmit_buffer.usb = NULL;
  }

  int MTPD_class::transmit_bulk() { // T4
    int r = 0;
    if (usb_mtp_status == 0x01) {
      write_transfer_open = (transmit_buffer.len > 0 && (transmit_buffer.len & transmit_packet_size_mask) == 0);
      usb_mtp_send(transmit_buffer.data, transmit_buffer.len, 50);
    }
    transmit_buffer.len = 0;
    transmit_buffer.index = 0;
    transmit_buffer.data = NULL;
    return r;
  }

#endif // MTP_MODE
#endif
