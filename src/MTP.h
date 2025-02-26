// MTP.h - Teensy MTP Responder library
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
// nov 2024: modified from MTP_Teensy (PJRC) by WMXZ 

#ifndef MTP_H
#define MTP_H
#define MTP_MODE 0

//#define __IMXRT1062__
//#define USB_MTPDISK

#if defined __IMXRT1062__
  #if !defined(USB_MTPDISK) && !defined(USB_MTPDISK_SERIAL)
    #error "You need to select USB Type: 'MTP Disk (Experimental)'"
  #endif

  #include "core_pins.h"
  #include "usb_dev.h"

  #include "Storage.h"
  // modify strings if needed (see MTP.cpp how they are used)
  #define MTP_MANUF "PJRC"
  #define MTP_MODEL "Teensy"
  #define MTP_VERS  "1.0"
  #define MTP_SERNR "1234"
  #define MTP_NAME  "Teensy"

  #define USE_EVENTS 0
  #if USE_EVENTS==1
    extern "C" 	int usb_mtp_sendEvent(const void *buffer, uint32_t len, uint32_t timeout);
  #endif
#else
  #include "Storage.h"
  // modify strings if needed (see MTP.cpp how they are used)
  #define MTP_MANUF "WMXZ"
  #define MTP_MODEL "uPAM"
  #define MTP_VERS  "1.0"
  #define MTP_SERNR "1234"
  #define MTP_NAME  "uPAM"

  #define USE_EVENTS 0
#endif

#if MTP_MODE==0
// MTP Responder.
class MTPD {
public:

  explicit MTPD(MTPStorageInterface* storage): storage_(storage) {}

private:
  MTPStorageInterface* storage_;

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
  } __attribute__((__may_alias__)) ;

  uint32_t mtp_rx_size_;
  uint32_t mtp_tx_size_;

#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
  usb_packet_t *data_buffer_ = NULL;
  void get_buffer() ;
  void receive_buffer() ;
//  inline MTPContainer *contains (usb_packet_t *receive_buffer) { return (MTPContainer*)(receive_buffer->buf);  }
// possible events for T3.xx ?

#elif defined(__IMXRT1062__)
  #define MTP_RX_SIZE MTP_RX_SIZE_480 
  #define MTP_TX_SIZE MTP_TX_SIZE_480 
  
  uint8_t rx_data_buffer[MTP_RX_SIZE] __attribute__ ((aligned(32)));
  uint8_t tx_data_buffer[MTP_TX_SIZE] __attribute__ ((aligned(32)));

  #define DISK_BUFFER_SIZE 8*1024
  uint8_t disk_buffer[DISK_BUFFER_SIZE] __attribute__ ((aligned(32)));
  uint32_t disk_pos=0;

  int push_packet(uint8_t *data_buffer, uint32_t len);
  int fetch_packet(uint8_t *data_buffer);
  int pull_packet(uint8_t *data_buffer);

#endif

  bool write_get_length_ = false;

  uint32_t write_length_ = 0;
  void write_init(uint16_t op, uint32_t transaction_id, uint32_t length);
  void write_finish(void);
  void writeo(const char *data, int len) ;
  void write(const char *data, int len) ;

  void write8 (uint8_t  x) ;
  void write16(uint16_t x) ;
  void write32(uint32_t x) ;
  void write64(uint64_t x) ;

  void writestring(const char* str) ;

  void WriteDescriptor() ;
  void WriteStorageIDs() ;

  void GetStorageInfo(uint32_t storage) ;

  uint32_t GetNumObjects(uint32_t storage, uint32_t parent) ;

  void GetObjectHandles(uint32_t storage, uint32_t parent) ;
  
  void GetObjectInfo(uint32_t handle) ;
  void GetObject(uint32_t object_id) ;
  void getObjecto(int32_t object_id);
  uint32_t GetPartialObject(uint32_t object_id, uint32_t offset, uint32_t NumBytes) ;
  
  void read(char* data, uint32_t size) ;

  uint32_t ReadMTPHeader() ;

  uint8_t read8() ;
  uint16_t read16() ;
  uint32_t read32() ;
  void readstring(char* buffer) ;

//  void read_until_short_packet() ;

  uint32_t SendObjectInfo(uint32_t storage, uint32_t parent) ;
  bool SendObject() ;

  void GetDevicePropValue(uint32_t prop) ;
  void GetDevicePropDesc(uint32_t prop) ;
  void getObjectPropsSupported(uint32_t p1) ;

  void getObjectPropDesc(uint32_t p1, uint32_t p2) ;
  void getObjectPropValue(uint32_t p1, uint32_t p2) ;

  uint32_t setObjectPropValue(uint32_t p1, uint32_t p2) ;

  static MTPD *g_pmtpd_interval;
  static void _interval_timer_handler();
  static IntervalTimer g_intervaltimer;
  void processIntervalTimer();

  uint32_t deleteObject(uint32_t p1) ;
  uint32_t copyObject(uint32_t p1,uint32_t p2, uint32_t p3) ;
  uint32_t moveObject(uint32_t p1,uint32_t p2, uint32_t p3) ;
  void openSession(uint32_t id) ;

  uint32_t TID;  


#if USE_EVENTS==1
  int usb_init_events(void);
  int send_Event(uint16_t eventCode);
  int send_Event(uint16_t eventCode, uint32_t p1);
  int send_Event(uint16_t eventCode, uint32_t p1, uint32_t p2);
  int send_Event(uint16_t eventCode, uint32_t p1, uint32_t p2, uint32_t p3);
#endif

public:
  void loop(void) ;
  void test(void) ;

#if USE_EVENTS==1
  int send_addObjectEvent(uint32_t p1);
  int send_removeObjectEvent(uint32_t p1);
  int send_StorageInfoChangedEvent(uint32_t p1);
  int send_StorageRemovedEvent(uint32_t p1);
  int send_DeviceResetEvent(void);
#endif
};

#elif MTP_MODE==1
class MTPD_class {

  #define MTP_RX_SIZE MTP_RX_SIZE_480 
  #define MTP_TX_SIZE MTP_TX_SIZE_480 
  
  uint8_t rx_data_buffer[MTP_RX_SIZE] __attribute__ ((aligned(32)));
  uint8_t tx_data_buffer[MTP_TX_SIZE] __attribute__ ((aligned(32)));

  uint16_t transmit_packet_size_mask = 0x01FF;

  static uint8_t disk_buffer_[DISK_BUFFER_SIZE] __attribute__((aligned(32)));

  static uint32_t sessionID_;
  
public:
  explicit MTPD_class(MTPStorageInterface* storage): storage_(storage) {}

  void loop(void);
private:
  struct MTPHeader {
    uint32_t len;            // 0
    uint16_t type;           // 4
    uint16_t op;             // 6
    uint32_t transaction_id; // 8
  };

  struct MTPContainer {
    uint32_t len;            // 0
    uint16_t type;           // 4
    uint16_t op;             // 6
    uint32_t transaction_id; // 8
    uint32_t params[5];      // 12
  };

  typedef struct {
    uint16_t len;   // number of data bytes
    uint16_t index; // position in processing data
    uint16_t size;  // total size of buffer
    uint8_t *data;  // pointer to the data
    void *usb;      // packet info (needed on Teensy 3)
  } packet_buffer_t;

  packet_buffer_t receive_buffer  = {0, 0, 0, NULL, NULL};
  packet_buffer_t transmit_buffer = {0, 0, 0, NULL, NULL};
  packet_buffer_t event_buffer    = {0, 0, 0, NULL, NULL};

  static uint8_t disk_buffer_[DISK_BUFFER_SIZE] __attribute__((aligned(32)));

  static uint32_t sessionID_;
  bool write_transfer_open = false;

  uint32_t TID;  

  bool receive_bulk(uint32_t timeout);
  void free_received_bulk();
  void allocate_transmit_bulk();
  int transmit_bulk();

  uint8_t usb_mtp_status;

  bool read_init(struct MTPHeader *header);
  bool readstring(char *buffer, uint32_t buffer_size);
  bool readDateTimeString(uint32_t *pdt);
  bool read(void *ptr, uint32_t size);

  bool read8(uint8_t *n) { return read(n, 1); }
  bool read16(uint16_t *n) { return read(n, 2); }
  bool read32(uint32_t *n) { return read(n, 4); }

  void write_init(struct MTPContainer &container, uint32_t data_size);
  void write_finish();
  
  size_t write(const void *ptr, size_t len);

  void write8 (uint8_t  x) { write((char*)&x, sizeof(x)); }
  void write16(uint16_t x) { write((char*)&x, sizeof(x)); }
  void write32(uint32_t x) { write((char*)&x, sizeof(x)); }
  void write64(uint64_t x) { write((char*)&x, sizeof(x)); }

  void writestring(const char* str) ;
  uint32_t writestringlen(const char *str);

  uint32_t getDeviceInfo(struct MTPContainer &cmd);
  uint32_t getDevicePropDesc(struct MTPContainer &cmd);
  uint32_t getDevicePropValue(struct MTPContainer &cmd);

  uint32_t openSession(struct MTPContainer &cmd) ;
  uint32_t getStorageIDs(struct MTPContainer &cmd);
  uint32_t getStorageInfo(struct MTPContainer &cmd);
  uint32_t getNumObjects(struct MTPContainer &cmd);
  uint32_t getObjectHandles(struct MTPContainer &cmd);
  uint32_t getObjectPropsSupported(struct MTPContainer &cmd) ;
  uint32_t getObjectPropDesc(struct MTPContainer &cmd) ;
  uint32_t getObjectPropValue(struct MTPContainer &cmd);

  uint32_t getObjectInfo(struct MTPContainer &cmd) ;
  uint32_t getObject(struct MTPContainer &cmd);
  uint32_t getPartialObject(struct MTPContainer &cmd) ;
  uint32_t deleteObject(struct MTPContainer &cmd) ;
  uint32_t moveObject(struct MTPContainer &cmd) ;
  uint32_t copyObject(struct MTPContainer &cmd) ;

uint32_t formatStore(struct MTPContainer &cmd) ;

uint32_t sendObjectInfo(struct MTPContainer &cmd) ;
uint32_t sendObject(struct MTPContainer &cmd);

uint32_t setObjectPropValue(struct MTPContainer &cmd);

  MTPStorageInterface* storage_;

  uint32_t object_id_ = 0;
  uint32_t dtCreated_ = 0;
  uint32_t dtModified_ = 0;


};
#endif // MTP_MODE

#endif
