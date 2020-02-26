# MTP_t4

MTP Responder for Teensy 4.0 and T3.x

needs Bill Greiman`s SdFat-beta https://github.com/greiman/SdFat-beta to support both Teensy 4.0 and ExFAT filesystems
 
code is based on https://github.com/yoonghm/MTP with modification by WMXZ

see also https://forum.pjrc.com/threads/43050-MTP-Responder-Contribution for discussions

files in different copy-to directories contain modifications of cores and need to be copied to cores/teensy4, cores/teensy3 and hardware/avr, respectively. These files are only necessary until Teensyduino has integrated full MTP into cores functionality

## Features
copying files from Teensy to PC  and from PC to Teensy is working

disk I/O is buffered to get some speed-up overcoming uSD latency issues

both Serialemu and true Serial may be used- True Serial port is, however, showing up as Everything in Com port. This is a workaround to get Serial working.


## Reset of Session
Modification of disk content (directories and Files) by Teensy is only be visible on PC when done before mounting the MTP device. To refresh disk content it is necessary to unmount and remount Teensy MTP device. On Windows this can be done by using device manager and disable and reanable Teensy (found under portable Device). On Linux this is done with standard muount/unmount commands.

## Installation:
 - download SdFat-beta from Bill Greiman`s github
   - in src directory copy file "SdFat.h" to "SdFat-beta.h"
   - edit SdFatConfig.h following Bill`s instructions e.g.:
     - #define SDFAT_FILE_TYPE 3
 - in Storage.h edit defines to use  SDIO e.g.:
   - #define USE_SDIO 0
 - copy content of copy-to-cores_teensy4 to cores/teensy4 orverwriting existing files
 - copy content of copy-to-cores_teensy3 to cores/teensy3 orverwriting existing file
 - copy content of copy-to-teensy_avr to teensy/avr orverwriting existing file
  

## To be done:
- check SdFat compatibility
- fix Serial port issue
 
