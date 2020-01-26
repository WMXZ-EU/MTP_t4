# MTP_t4

MTP Responder for Teensy 4.0 and T3.x

needs Bill Greiman`s SdFat-beta https://github.com/greiman/SdFat-beta to support both Teensy 4.0 and ExFAT filesystems
 
code is based on https://github.com/yoonghm/MTP

see also https://forum.pjrc.com/threads/43050-MTP-Responder-Contribution for discussions

files in copy-of-core contain modifications of core and need to be copied to cores/teensy4. 

copying files from Teensy to PC  and from PC to Teensy is working

disk I/O is buffered to get some speed-up overcoming uSD latency issues


Installation:
 - copy files from "copy-to-core" to cores/Teensy4. (This will modify existing routines; NB Do not copy to cores/teensy3.)
 - download SdFat-beta from Bill Greiman`s github
 - copy in src directory file "SdFat.h" to "SdFat-beta.h"
 - edit SdFatConfig.h following Bill`s instructions e.g.:
   - #define SDFAT_FILE_TYPE 3
 - in MTP_config.h edit defines to use SdFat-beta or SDIO e.g.:
   - #define USE_SDFAT_BETA 1
   - #define USE_SDIO 0
 - remove 3 comments in "\hardware\teensy\avr\boards.txt" for
   - teensy40.menu.usb.mtp=MTP Disk (Experimental)
   - teensy40.menu.usb.mtp.build.usbtype=USB_MTPDISK
   - teensy40.menu.usb.mtp.fake_serial=teensy_gateway
 - add 2 lines in "\hardware\teensy\avr\boards.txt" for
   - teensy40.menu.usb.mtp=MTP Disk + Serial (Experimental)
   - teensy40.menu.usb.mtp.build.usbtype=USB_MTPDISK_SERIAL

To be done:
 - check SdFat compatibility
 
