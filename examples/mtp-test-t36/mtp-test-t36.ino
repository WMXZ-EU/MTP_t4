#include "core_pins.h"
#include "usb_serial.h"

  #include "usb2_serial.h"
  usb2_serial_class HSerial; // defined in usb2_serial.h

  extern "C" void usb2_init();

  #include "MTP2.h"

MTPStorage_SD storage;
MTPD       mtpd(&storage);

void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    digitalWriteFast(13,!digitalReadFast(13));
    HSerial.println(txt); 
    to=millis();
  }
}

void setup()
{ 
  //while(!Serial || millis()<4000);
  Serial.println("MTP test");

  storage.init();

 	usb2_init();

  pinMode(13,OUTPUT);
}

void loop()
{ 
  mtpd.loop();

  logg(1000,"loop");
  
}
