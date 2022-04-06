# MTP_t4

MTP Responder for Teensy 3.x and 4.x

Uses SD interface, which interfaces to Bill Greiman's SdFat_V2 as distributed via Teenyduino supporting exFAT and SDIO. 

code is based on https://github.com/yoonghm/MTP with modification by WMXZ

see also https://forum.pjrc.com/threads/43050-MTP-Responder-Contribution for discussions

files in different copy-to directories contain modifications of cores and need to be copied to cores/teensy4, cores/teensy3 and hardware/avr, respectively. These files are only necessary until Teensyduino has integrated full MTP into cores functionality

(before TD 1.54 final) needs USB2 https://github.com/WMXZ-EU/USB2 for T4.x. (uses here usb1.h and usb1.c)


## Features
 - Supports multiple MTP-disks (SDIO, multiple SPI disks, LittleFS_xxx disks)
 - copying files from Teensy to PC  and from PC to Teensy is working
 - disk I/O to/from PC is buffered to get some speed-up overcoming uSD latency issues
 - both Serialemu and true Serial may be used- True Serial port is, however, showing up as Everything in Com port. This is a workaround to get Serial working.
 - deletion of files
 - recursive deletion of directories
 - creation of directories
 - moving files and directories within and cross MTP-disk disks
 - copying files and directories within and cross MTP-disk disks

## Limitations
 - Maximal filename length is 256 but can be changed in Storage.h by changing the MAX_FILENAME_LEN definition
 - within-MTP copy not yet implemented (i.e no within-disk and cross-disk copy)
 - creation of files using file explorer is not supported, but directories can be created
 - No creation and modification timestamps are shown
 
## Reset of Session
Modification of disk content (directories and Files) by Teensy is only be visible on PC when done before mounting the MTP device. To refresh disk content it is necessary to unmount and remount Teensy MTP device. AFAIK: On Windows this can be done by using device manager and disable and reanable Teensy (found under portable Device). On Linux this is done with standard muount/unmount commands.

Session may be reset from Teensy by sending a reset event. This is shown in mtp-test example where sending the character 'r' from PC to Teensy generates a reset event. It is suggested to close file explorer before reseting mtp

In scipts directory is a powershell script that unmounts/mounts the Teensy portable device

## Examples
 - mtp-basic:  basic MTP program
 - mtp-test:   basic MTP test program
 - mtp-logger: basic data logger with MTP access
 - mtp-audioRecorder: example about using mtp-logger as sgtl5000 audioRecorder
 
## Installation:
 - If you wanted to use USB_MTP_SERIAL  
   - T4.x edit teensy/avr/cores/teensy4/usb_desc.h with content of 'modifications_for_cores_teensy4' (insert after USB_MTPDISK)
   - T3.x edit teensy/avr/cores/teensy3/usb_desc.h with content of 'modifications_for_cores_teensy3' (insert after USB_MTPDISK)
   - edit teensy/avr/boards.txt with content of 'modifications_for_teensy_avr' (copy to end of file)
 - install also USB2 from WMXZ github if cores does not have "usb_mtp.h"
 - install LittleFS from https://github.com/PaulStoffregen/LittleFS for use of LittleFS basd filesystems
 - remove "Time.h" in "libraries/Time" to eliminate compiler warnings

 ## Known Issues
   - copying of files and directories work but are not displayed in file explorer, manual unmount/mount sequence required
   - deleting large nested directories may generate a time-out error and brick MTP. No work around known, only restart Teensy
   
 ## Scripts
 There are some useful scripts for Windows PowerShell in scrips directory. open them with right-click "Run with PowerShell"
  - MTPdir list the files in Teensy 
  - MTPreset to reset MTP (disable/enable)
 
 ## ToBeDone
 - show creation and modification timestamps

