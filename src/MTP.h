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
// modified for T4

#ifndef MTP_H
#define MTP_H

#if !defined(USB_MTPDISK)
  #error "You need to select USB Type: 'MTP Disk (Experimental)'"
#endif

#include "usb_mtp.h"
#include "MTP_Storage.h"

// MTP Responder.

class MTPD {
public:
  explicit MTPD(MTPStorageInterface* storage) : storage_(storage) {}

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
  } __attribute__((__may_alias__))  ;

#if defined(__IMXRT1062__)
  void PrintPacket(const uint8_t *x, int len) ;

  uint8_t data_buffer[MTP_RX_SIZE] __attribute__ ((used, aligned(32)));

#else

  void PrintPacket(const usb_packet_t *x);

  inline MTPContainer *contains (usb_packet_t *receive_buffer)
  { return (MTPContainer*)(receive_buffer->buf);
  }

  usb_packet_t *data_buffer_ = NULL;

  void get_buffer();
  void receive_buffer();
  #endif

  bool write_get_length_ = false;
  uint32_t write_length_ = 0;
  
  void write(const char *data, int len) ;
  void write8 (uint8_t  x) ;
  void write16(uint16_t x) ;
  void write32(uint32_t x) ;
  void write64(uint64_t x) ;
  void writestring(const char* str) ;
  void WriteDescriptor() ;

  void GetDevicePropValue(uint32_t prop) ;
  void GetDevicePropDesc(uint32_t prop) ;
  void WriteStorageIDs() ;
  void GetStorageInfo(uint32_t storage) ;
  uint32_t GetNumObjects(uint32_t storage, uint32_t parent) ;
  void GetObjectHandles(uint32_t storage, uint32_t parent) ;
  void GetObjectInfo(uint32_t handle) ;
  void GetObject(uint32_t object_id) ;

/*************************************************************************************/
  void read(char* data, uint32_t size) ;
  uint32_t ReadMTPHeader() ;
  uint8_t read8() ;
  uint16_t read16() ;
  uint32_t read32() ;
  void readstring(char* buffer) ;
  void read_until_short_packet() ;

  uint32_t SendObjectInfo(uint32_t storage, uint32_t parent) ;
  void SendObject() ;

public:
  void loop(void) ;
};
#endif
