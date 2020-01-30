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

#if !defined(USB_MTPDISK) && !defined(USB_MTPDISK_SERIAL)
      #error "You need to select USB Type: 'MTP Disk (Experimental) or MTP DISK + Serial (Experimental)"
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

  uint8_t data_buffer[MTP_RX_SIZE] __attribute__ ((aligned(32)));

  #define DISK_BUFFER_SIZE 8*1024
  uint8_t disk_buffer[DISK_BUFFER_SIZE] __attribute__ ((aligned(32)));
  uint32_t disk_pos=0;

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

  uint32_t deleteObject(uint32_t p1);
  uint32_t moveObject(uint32_t p1, uint32_t p3);
  void getObjectPropsSupported(uint32_t p1);
  void getObjectPropDesc(uint32_t p1, uint32_t p2);
  void getObjectPropValue(uint32_t p1, uint32_t p2);
  uint32_t setObjectPropValue(uint32_t p1, uint32_t p2);

/*************************************************************************************/
  void read(char* data, uint32_t size) ;
  uint32_t ReadMTPHeader() ;
  uint8_t read8() ;
  uint16_t read16() ;
  uint32_t read32() ;
  uint32_t readstring(char* buffer) ;
  void read_until_short_packet() ;

  void fetch_packet(uint8_t *buffer);

  uint32_t SendObjectInfo(uint32_t storage, uint32_t parent) ;
  void SendObject() ;

  void OpenSession(void);

public:
  void loop(void) ;

};
#endif

/*
// MTP Object Property Codes

#define MTP_PROPERTY_STORAGE_ID                             0xDC01
#define MTP_PROPERTY_OBJECT_FORMAT                          0xDC02
#define MTP_PROPERTY_PROTECTION_STATUS                      0xDC03
#define MTP_PROPERTY_OBJECT_SIZE                            0xDC04
#define MTP_PROPERTY_ASSOCIATION_TYPE                       0xDC05
#define MTP_PROPERTY_ASSOCIATION_DESC                       0xDC06
#define MTP_PROPERTY_OBJECT_FILE_NAME                       0xDC07
#define MTP_PROPERTY_DATE_CREATED                           0xDC08
#define MTP_PROPERTY_DATE_MODIFIED                          0xDC09
#define MTP_PROPERTY_KEYWORDS                               0xDC0A
#define MTP_PROPERTY_PARENT_OBJECT                          0xDC0B
#define MTP_PROPERTY_ALLOWED_FOLDER_CONTENTS                0xDC0C
#define MTP_PROPERTY_HIDDEN                                 0xDC0D
#define MTP_PROPERTY_SYSTEM_OBJECT                          0xDC0E
#define MTP_PROPERTY_PERSISTENT_UID                         0xDC41
#define MTP_PROPERTY_SYNC_ID                                0xDC42
#define MTP_PROPERTY_PROPERTY_BAG                           0xDC43
#define MTP_PROPERTY_NAME                                   0xDC44
#define MTP_PROPERTY_CREATED_BY                             0xDC45
#define MTP_PROPERTY_ARTIST                                 0xDC46
#define MTP_PROPERTY_DATE_AUTHORED                          0xDC47
#define MTP_PROPERTY_DESCRIPTION                            0xDC48
#define MTP_PROPERTY_URL_REFERENCE                          0xDC49
#define MTP_PROPERTY_LANGUAGE_LOCALE                        0xDC4A
#define MTP_PROPERTY_COPYRIGHT_INFORMATION                  0xDC4B
#define MTP_PROPERTY_SOURCE                                 0xDC4C
#define MTP_PROPERTY_ORIGIN_LOCATION                        0xDC4D
#define MTP_PROPERTY_DATE_ADDED                             0xDC4E
#define MTP_PROPERTY_NON_CONSUMABLE                         0xDC4F
#define MTP_PROPERTY_CORRUPT_UNPLAYABLE                     0xDC50
#define MTP_PROPERTY_PRODUCER_SERIAL_NUMBER                 0xDC51
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_FORMAT           0xDC81
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_SIZE             0xDC82
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_HEIGHT           0xDC83
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_WIDTH            0xDC84
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DURATION         0xDC85
#define MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DATA             0xDC86
#define MTP_PROPERTY_WIDTH                                  0xDC87
#define MTP_PROPERTY_HEIGHT                                 0xDC88
#define MTP_PROPERTY_DURATION                               0xDC89
#define MTP_PROPERTY_RATING                                 0xDC8A
#define MTP_PROPERTY_TRACK                                  0xDC8B
#define MTP_PROPERTY_GENRE                                  0xDC8C
#define MTP_PROPERTY_CREDITS                                0xDC8D
#define MTP_PROPERTY_LYRICS                                 0xDC8E
#define MTP_PROPERTY_SUBSCRIPTION_CONTENT_ID                0xDC8F
#define MTP_PROPERTY_PRODUCED_BY                            0xDC90
#define MTP_PROPERTY_USE_COUNT                              0xDC91
#define MTP_PROPERTY_SKIP_COUNT                             0xDC92
#define MTP_PROPERTY_LAST_ACCESSED                          0xDC93
#define MTP_PROPERTY_PARENTAL_RATING                        0xDC94
#define MTP_PROPERTY_META_GENRE                             0xDC95
#define MTP_PROPERTY_COMPOSER                               0xDC96
#define MTP_PROPERTY_EFFECTIVE_RATING                       0xDC97
#define MTP_PROPERTY_SUBTITLE                               0xDC98
#define MTP_PROPERTY_ORIGINAL_RELEASE_DATE                  0xDC99
#define MTP_PROPERTY_ALBUM_NAME                             0xDC9A
#define MTP_PROPERTY_ALBUM_ARTIST                           0xDC9B
#define MTP_PROPERTY_MOOD                                   0xDC9C
#define MTP_PROPERTY_DRM_STATUS                             0xDC9D
#define MTP_PROPERTY_SUB_DESCRIPTION                        0xDC9E
#define MTP_PROPERTY_IS_CROPPED                             0xDCD1
#define MTP_PROPERTY_IS_COLOUR_CORRECTED                    0xDCD2
#define MTP_PROPERTY_IMAGE_BIT_DEPTH                        0xDCD3
#define MTP_PROPERTY_F_NUMBER                               0xDCD4
#define MTP_PROPERTY_EXPOSURE_TIME                          0xDCD5
#define MTP_PROPERTY_EXPOSURE_INDEX                         0xDCD6
#define MTP_PROPERTY_TOTAL_BITRATE                          0xDE91
#define MTP_PROPERTY_BITRATE_TYPE                           0xDE92
#define MTP_PROPERTY_SAMPLE_RATE                            0xDE93
#define MTP_PROPERTY_NUMBER_OF_CHANNELS                     0xDE94
#define MTP_PROPERTY_AUDIO_BIT_DEPTH                        0xDE95
#define MTP_PROPERTY_SCAN_TYPE                              0xDE97
#define MTP_PROPERTY_AUDIO_WAVE_CODEC                       0xDE99
#define MTP_PROPERTY_AUDIO_BITRATE                          0xDE9A
#define MTP_PROPERTY_VIDEO_FOURCC_CODEC                     0xDE9B
#define MTP_PROPERTY_VIDEO_BITRATE                          0xDE9C
#define MTP_PROPERTY_FRAMES_PER_THOUSAND_SECONDS            0xDE9D
#define MTP_PROPERTY_KEYFRAME_DISTANCE                      0xDE9E
#define MTP_PROPERTY_BUFFER_SIZE                            0xDE9F
#define MTP_PROPERTY_ENCODING_QUALITY                       0xDEA0
#define MTP_PROPERTY_ENCODING_PROFILE                       0xDEA1
#define MTP_PROPERTY_DISPLAY_NAME                           0xDCE0
#define MTP_PROPERTY_BODY_TEXT                              0xDCE1
#define MTP_PROPERTY_SUBJECT                                0xDCE2
#define MTP_PROPERTY_PRIORITY                               0xDCE3
#define MTP_PROPERTY_GIVEN_NAME                             0xDD00
#define MTP_PROPERTY_MIDDLE_NAMES                           0xDD01
#define MTP_PROPERTY_FAMILY_NAME                            0xDD02
#define MTP_PROPERTY_PREFIX                                 0xDD03
#define MTP_PROPERTY_SUFFIX                                 0xDD04
#define MTP_PROPERTY_PHONETIC_GIVEN_NAME                    0xDD05
#define MTP_PROPERTY_PHONETIC_FAMILY_NAME                   0xDD06
#define MTP_PROPERTY_EMAIL_PRIMARY                          0xDD07
#define MTP_PROPERTY_EMAIL_PERSONAL_1                       0xDD08
#define MTP_PROPERTY_EMAIL_PERSONAL_2                       0xDD09
#define MTP_PROPERTY_EMAIL_BUSINESS_1                       0xDD0A
#define MTP_PROPERTY_EMAIL_BUSINESS_2                       0xDD0B
#define MTP_PROPERTY_EMAIL_OTHERS                           0xDD0C
#define MTP_PROPERTY_PHONE_NUMBER_PRIMARY                   0xDD0D
#define MTP_PROPERTY_PHONE_NUMBER_PERSONAL                  0xDD0E
#define MTP_PROPERTY_PHONE_NUMBER_PERSONAL_2                0xDD0F
#define MTP_PROPERTY_PHONE_NUMBER_BUSINESS                  0xDD10
#define MTP_PROPERTY_PHONE_NUMBER_BUSINESS_2                0xDD11
#define MTP_PROPERTY_PHONE_NUMBER_MOBILE                    0xDD12
#define MTP_PROPERTY_PHONE_NUMBER_MOBILE_2                  0xDD13
#define MTP_PROPERTY_FAX_NUMBER_PRIMARY                     0xDD14
#define MTP_PROPERTY_FAX_NUMBER_PERSONAL                    0xDD15
#define MTP_PROPERTY_FAX_NUMBER_BUSINESS                    0xDD16
#define MTP_PROPERTY_PAGER_NUMBER                           0xDD17
#define MTP_PROPERTY_PHONE_NUMBER_OTHERS                    0xDD18
#define MTP_PROPERTY_PRIMARY_WEB_ADDRESS                    0xDD19
#define MTP_PROPERTY_PERSONAL_WEB_ADDRESS                   0xDD1A
#define MTP_PROPERTY_BUSINESS_WEB_ADDRESS                   0xDD1B
#define MTP_PROPERTY_INSTANT_MESSANGER_ADDRESS              0xDD1C
#define MTP_PROPERTY_INSTANT_MESSANGER_ADDRESS_2            0xDD1D
#define MTP_PROPERTY_INSTANT_MESSANGER_ADDRESS_3            0xDD1E
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_FULL           0xDD1F
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_1         0xDD20
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_2         0xDD21
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_CITY           0xDD22
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_REGION         0xDD23
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_POSTAL_CODE    0xDD24
#define MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_COUNTRY        0xDD25
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_FULL           0xDD26
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_1         0xDD27
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_2         0xDD28
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_CITY           0xDD29
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_REGION         0xDD2A
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_POSTAL_CODE    0xDD2B
#define MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_COUNTRY        0xDD2C
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_FULL              0xDD2D
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_1            0xDD2E
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_2            0xDD2F
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_CITY              0xDD30
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_REGION            0xDD31
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_POSTAL_CODE       0xDD32
#define MTP_PROPERTY_POSTAL_ADDRESS_OTHER_COUNTRY           0xDD33
#define MTP_PROPERTY_ORGANIZATION_NAME                      0xDD34
#define MTP_PROPERTY_PHONETIC_ORGANIZATION_NAME             0xDD35
#define MTP_PROPERTY_ROLE                                   0xDD36
#define MTP_PROPERTY_BIRTHDATE                              0xDD37
#define MTP_PROPERTY_MESSAGE_TO                             0xDD40
#define MTP_PROPERTY_MESSAGE_CC                             0xDD41
#define MTP_PROPERTY_MESSAGE_BCC                            0xDD42
#define MTP_PROPERTY_MESSAGE_READ                           0xDD43
#define MTP_PROPERTY_MESSAGE_RECEIVED_TIME                  0xDD44
#define MTP_PROPERTY_MESSAGE_SENDER                         0xDD45
#define MTP_PROPERTY_ACTIVITY_BEGIN_TIME                    0xDD50
#define MTP_PROPERTY_ACTIVITY_END_TIME                      0xDD51
#define MTP_PROPERTY_ACTIVITY_LOCATION                      0xDD52
#define MTP_PROPERTY_ACTIVITY_REQUIRED_ATTENDEES            0xDD54
#define MTP_PROPERTY_ACTIVITY_OPTIONAL_ATTENDEES            0xDD55
#define MTP_PROPERTY_ACTIVITY_RESOURCES                     0xDD56
#define MTP_PROPERTY_ACTIVITY_ACCEPTED                      0xDD57
#define MTP_PROPERTY_ACTIVITY_TENTATIVE                     0xDD58
#define MTP_PROPERTY_ACTIVITY_DECLINED                      0xDD59
#define MTP_PROPERTY_ACTIVITY_REMAINDER_TIME                0xDD5A
#define MTP_PROPERTY_ACTIVITY_OWNER                         0xDD5B
#define MTP_PROPERTY_ACTIVITY_STATUS                        0xDD5C
#define MTP_PROPERTY_OWNER                                  0xDD5D
#define MTP_PROPERTY_EDITOR                                 0xDD5E
#define MTP_PROPERTY_WEBMASTER                              0xDD5F
#define MTP_PROPERTY_URL_SOURCE                             0xDD60
#define MTP_PROPERTY_URL_DESTINATION                        0xDD61
#define MTP_PROPERTY_TIME_BOOKMARK                          0xDD62
#define MTP_PROPERTY_OBJECT_BOOKMARK                        0xDD63
#define MTP_PROPERTY_BYTE_BOOKMARK                          0xDD64
#define MTP_PROPERTY_LAST_BUILD_DATE                        0xDD70
#define MTP_PROPERTY_TIME_TO_LIVE                           0xDD71
#define MTP_PROPERTY_MEDIA_GUID                             0xDD72
*/