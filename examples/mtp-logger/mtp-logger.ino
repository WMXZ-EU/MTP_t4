#include "Arduino.h"
#ifndef USE_SDIO
  #define USE_SDIO 1 // change to 1 if using SDIO 
#endif

#define NCH 4
#define NBUF_ACQ 128*NCH

#include "MTP.h"
#include "usb1_mtp.h"

MTPStorage_SD storage;
MTPD       mtpd(&storage);


void logg(uint32_t del, const char *txt);

int16_t state;
int16_t do_logger(int16_t state);
int16_t do_menu(int16_t state);

void setup()
{ 
  while(!Serial && millis()<3000); 
  usb_mtp_configure();
  if(!Storage_init()) {Serial.println("No storage"); while(1);};

  Serial.println("MTP logger");

  #if USE_SDIO==1
    pinMode(13,OUTPUT);
  #endif
  state=-1;
}

void loop()
{ 
  state = do_menu(state);
  //
  if(state<0)
    mtpd.loop();
  else
    state=do_logger(state);

  logg(1000,"loop");
  //asm("wfi"); // may wait forever on T4.x
}

/*************************** Circular Buffer ********************/

#if defined(ARDUINO_TEENSY41)
  #define HAVE_PSRAM 1
  #if HAVE_PSRAM==1
    extern "C" uint8_t external_psram_size;
    uint8_t size = external_psram_size;
    uint32_t *memory_begin = (uint32_t *)(0x70000000);

    #define MAXBUF (1000) // < 5461
    uint32_t *data_buffer = memory_begin;
  #else
    #define MAXBUF (46)
    uint32_t data_buffer[MAXBUF*NBUF_ACQ];
  #endif
#else
    #define MAXBUF (46)
    uint32_t data_buffer[MAXBUF*NBUF_ACQ];
#endif

static uint16_t front_ = 0, rear_ = 0;
uint16_t getCount () { if(front_ >= rear_) return front_ - rear_; return front_+ MAXBUF -rear_; }

uint16_t pushData(uint32_t * src)
{ uint16_t f =front_ + 1;
  if(f >= MAXBUF) f=0;
  if(f == rear_) return 0;

  uint32_t *ptr= data_buffer+f*NBUF_ACQ;
  memcpy(ptr,src,NBUF_ACQ*4);
  front_ = f;
  return 1;
}

uint16_t pullData(uint32_t * dst, uint16_t ndbl)
{ uint16_t r = rear_ ;
  if(r == (front_/ndbl)) return 0;
  if(++r >= MAXBUF/ndbl) r=0;
  uint32_t *ptr= data_buffer+r*ndbl*NBUF_ACQ;
  memcpy(dst,ptr,ndbl*NBUF_ACQ*4);
  rear_ = r;
  return 1;
}

/*************************** Filing *****************************/
int16_t file_open(void);
int16_t file_writeHeader(void);
int16_t file_writeData(uint32_t *diskBuffer, uint16_t ndbl);
int16_t file_close(void);
#define NDBL 2
#define NBUF_DISK (NDBL*NBUF_ACQ)
uint32_t diskBuffer[NBUF_DISK];

int16_t do_logger(int16_t state)
{
  if(pullData(diskBuffer,NDBL))
  {
    if(state==0)
    { // acquisition is running, need to open file
      if(!file_open()) return -1;
      state=1;
    }
    if(state==1)
    { // file just opended, need to write header
      if(!file_writeHeader()) return -1;
      state=2;
      
    }
    if(state>=2)
    { // write data to disk
      if(!file_writeData(diskBuffer,NBUF_DISK)) return -1;
    }
  }

  if(state==3)
  { // close file, but continue acquisition
    if(!file_close()) return -1;
    state=0;
    
  }
  if(state==4)
  { // close file and stop acquisition
    if(!file_close()) return -1;
    state=-1;
    
  }
  return state;
}

/******************** Menu ***************************/
void do_menu1(void);
void do_menu2(void);
void do_menu3(void);

int16_t do_menu(int16_t state)
{ // check Serial input
  if(!Serial.available()) return state;
  char cc = Serial.read();
  switch(cc)
  {
    case 's': // start acquisition
      if(state>=0) return state;
      state=0;
      break;
    case 'q': // stop acquisition
      if(state<0) return state;
      state=4;
      break;
    case '?': // get parameters
      do_menu1();
      break;
    case '!': // set parameters
      if(state>=0) return state;
      do_menu2();
      break;
    case ':': // misc commands
      if(state>=0) return state;
      do_menu3();
      break;
    default:
      break;
  }
  return state;
}

/************ Basic File System Interface *************************/
extern SdFs sd;
static FsFile mfile;
char header[512];

void makeHeader(char *header);

int16_t file_open(void)
{ char filename[80];
  mfile = sd.open(filename,FILE_WRITE);
  return mfile.isOpen();
}
int16_t file_writeHeader(void)
{ if(!mfile.isOpen()) return 0;
  makeHeader(header);
  size_t nb = mfile.write(header,512);
  return (nb==512);
}
int16_t file_writeData(uint32_t *diskBuffer, uint16_t nd)
{
  if(!mfile.isOpen()) return 0;
  size_t nb = mfile.write(diskBuffer,4*nd);
  return (nb==4*nd);
}
int16_t file_close(void)
{ return mfile.close();
}

/*
 * Custom Implementation
 * 
 */

/************* Menu implementation ******************************/
void do_menu1(void)
{  // get parameters
}
void do_menu2(void)
{  // set parameters
}
void do_menu3(void)
{  // misc commands
}

/**************** Online logging *******************************/
void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    Serial.println(txt); 
    #if USE_SDIO==1
        digitalWriteFast(13,!digitalReadFast(13));
    #endif
    to=millis();
  }
}
/****************** File Utilities *****************************/
void makeHeader(char *header)
{

}