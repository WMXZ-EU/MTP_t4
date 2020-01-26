#include "Arduino.h"

  #include "MTP.h"

  MTPStorage_SD storage;
  MTPD       mtpd(&storage);

void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    Serial.println(txt); 
    to=millis();
  }
}

void setup()
{ 
  //while(!Serial || millis()<4000);
  Serial.println("MTP test");
  //Serial1.begin(115200);

  storage.init();

}

void loop()
{ 
  mtpd.loop();

  logg(1000,"loop");
  
}
