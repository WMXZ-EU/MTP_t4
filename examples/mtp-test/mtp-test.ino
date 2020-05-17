#include "Arduino.h"
#ifndef USE_SDIO
  #define USE_SDIO 0 // change to 1 if using SDIO 
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

void setup()
{ 
  while(!Serial && millis()<3000); 
  usb_mtp_configure();
  Storage_init();

  Serial.println("MTP test");

#if USE_SDIO==1
  pinMode(13,OUTPUT);
#endif

}

void loop()
{ 
  mtpd.loop();

  logg(1000,"loop");
  //asm("wfi"); // may wait forever on T4.x
}
