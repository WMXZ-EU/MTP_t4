#include "Arduino.h"

  #include "mtp.h"

  MTPStorage_SD storage;
  MTPD       mtpd(&storage);

void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
//    Serial.println(txt); 
    digitalWriteFast(13,!digitalReadFast(13));
    to=millis();
  }
}

void setup()
{ 
  Serial.println("MTP test");
  //Serial1.begin(115200);

  storage_init();

  pinMode(13,OUTPUT);

}

void loop()
{ 
  mtpd.loop();

  logg(1000,"loop");
  
}
