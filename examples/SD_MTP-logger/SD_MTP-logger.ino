/*
  LittleFS  datalogger

  This example shows how to log data from three analog sensors
  to an storage device such as a FLASH.

  This example code is in the public domain.
*/
#include "SD.h"
#include <MTP.h>
#include <SD_MTP_Callback.h>


#if defined(BUILTIN_SDCARD)
#define CS_SD  BUILTIN_SDCARD
#else
#define CS_SD 10
#endif


// LittleFS supports creating file systems (FS) in multiple memory types.  Depending on the
// memory type you want to use you would uncomment one of the following constructors

SDClass myfs;  // Used to create FS on QSPI NAND flash chips located on the bottom of the T4.1 such as the W25N01G. for the full list of supported NAND flash see  https://github.com/PaulStoffregen/LittleFS#nand-flash

SD_MTP_CB sd_mtp_cb;

File dataFile;  // Specifes that dataFile is of File type

int record_count = 0;
bool write_data = false;
uint32_t diskSize;

static const uint32_t file_system_size = 1024 * 512;

// Add in MTPD objects
MTPStorage_SD storage;
MTPD       mtpd(&storage);

void setup()
{

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {
    // wait for serial port to connect.
  }
  Serial.print(CrashReport);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);

  Serial.print("Initializing SD ...");

  // See if we can initialize SD FS
#if CS_SD == BUILTIN_SDCARD
  if (!myfs.sdfs.begin(SdioConfig(FIFO_SDIO))) {
    Serial.println("SDIO Storage failed or missing");
    // BUGBUG Add the detect insertion?
    pinMode(13, OUTPUT);
    while (1) {
      digitalToggleFast(13);
      delay(250);
    }
  }
#else
  if (!myfs.sdfs.begin(SdSpiConfig(CS_SD, SHARED_SPI, SPI_SPEED))) {
    Serial.printf("SDIO Storage failed or missing on SD Pin: %u\n", CS_SD);
    // BUGBUG Add the detect insertion?
    pinMode(13, OUTPUT);
    while (1) {
      digitalToggleFast(13);
      delay(250);
    }
  }

#endif
  mtpd.begin();
  storage.addFilesystem(myfs, "SD", &sd_mtp_cb, (uint32_t)(void*)&myfs);
  uint64_t totalSize = myfs.totalSize();
  uint64_t usedSize  = myfs.usedSize();
  Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);

  Serial.println("SD initialized.");

  menu();

}

void loop()
{
  if ( Serial.available() ) {
    char rr;
    rr = Serial.read();
    switch (rr) {
    case 'l': listFiles(); break;
    case 'e': eraseFiles(); break;
    case 's':
    {
      Serial.println("\nLogging Data!!!");
      write_data = true;   // sets flag to continue to write data until new command is received
      // opens a file or creates a file if not present,  FILE_WRITE will append data to
      // to the file created.
      dataFile = myfs.open("datalog.txt", FILE_WRITE);
      logData();
    }
    break;
    case 'x': stopLogging(); break;

    case 'd': dumpLog(); break;
    case '\r':
    case '\n':
    case 'h': menu(); break;
    }
    while (Serial.read() != -1) ; // remove rest of characters.
  }
  else mtpd.loop();

  if (write_data) logData();
}

void logData()
{
  // make a string for assembling the data to log:
  String dataString = "";

  // read three sensors and append to the string:
  for (int analogPin = 0; analogPin < 3; analogPin++) {
    int sensor = analogRead(analogPin);
    dataString += String(sensor);
    if (analogPin < 2) {
      dataString += ",";
    }
  }

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    // print to the serial port too:
    Serial.println(dataString);
    record_count += 1;
  } else {
    // if the file isn't open, pop up an error:
    Serial.println("error opening datalog.txt");
  }
  delay(100); // run at a reasonable not-too-fast speed for testing
}

void stopLogging()
{
  Serial.println("\nStopped Logging Data!!!");
  write_data = false;
  // Closes the data file.
  dataFile.close();
  Serial.printf("Records written = %d\n", record_count);
  mtpd.send_DeviceResetEvent();
}


void dumpLog()
{
  Serial.println("\nDumping Log!!!");
  // open the file.
  dataFile = myfs.open("datalog.txt");

  // if the file is available, write to it:
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

void menu()
{
  Serial.println();
  Serial.println("Menu Options:");
  Serial.println("\tl - List files on disk");
  Serial.println("\te - Erase files on disk");
  Serial.println("\ts - Start Logging data (Restarting logger will append records to existing log)");
  Serial.println("\tx - Stop Logging data");
  Serial.println("\td - Dump Log");
  Serial.println("\th - Menu");
  Serial.println();
}

void listFiles()
{
  Serial.print("\n Space Used = ");
  Serial.println(myfs.usedSize());
  Serial.print("Filesystem Size = ");
  Serial.println(myfs.totalSize());

  printDirectory(myfs);
}

extern PFsLib pfsLIB;
void eraseFiles()
{

  PFsVolume partVol;
  if (!partVol.begin(myfs.sdfs.card(), true, 1)) {
    Serial.println("Failed to initialize partition");
    return;
  }
  if (pfsLIB.formatter(partVol)) {
    Serial.println("\nFiles erased !");
    mtpd.send_DeviceResetEvent();
  }
}

void printDirectory(FS &fs) {
  Serial.println("Directory\n---------");
  printDirectory(fs.open("/"), 0);
  Serial.println();
}

void printDirectory(File dir, int numSpaces) {
  while (true) {
    File entry = dir.openNextFile();
    if (! entry) {
      //Serial.println("** no more files **");
      break;
    }
    printSpaces(numSpaces);
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numSpaces + 2);
    } else {
      // files have sizes, directories do not
      printSpaces(36 - numSpaces - strlen(entry.name()));
      Serial.print("  ");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void printSpaces(int num) {
  for (int i = 0; i < num; i++) {
    Serial.print(" ");
  }
}
