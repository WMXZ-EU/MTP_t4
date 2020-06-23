#include "Arduino.h"
  #ifndef USE_SDIO
    #define USE_SDIO 1 // change to 1 if using SDIO 
  #endif

  #include "MTP.h"
  #include "usb1_mtp.h"

  MTPStorage_SD storage;
  MTPD       mtpd(&storage);


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

int16_t file_open(void);
int16_t file_writeHeader(void);
int16_t file_writeData(void);
int16_t file_close(void);

int16_t do_logger(int16_t state)
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
  if(state==2)
  { // fetch data from buffer
    // write data to disk
    if(!file_writeData()) return -1;
    state=2;
    
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

/************ File System Interface *****************************/
int16_t file_open(void)
{
  return 1;
}
int16_t file_writeHeader(void)
{
  return 1;
}
int16_t file_writeData(void)
{
  return 1;
}
int16_t file_close(void)
{
  return 1;
}


int16_t do_menu(int16_t state)
{ // check Serial input
  // change state of program
  // set state to 0 to start acquisition
  // set state to 4 to stop acquisition
  // if stopped allow change of parameters
  return state;
}