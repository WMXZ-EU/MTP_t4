/*
 * Example on how to extend the MTP logger to interface with SGTL5000 audio board
 * 
 * sampling frequency may be selected from a list of frequencies
 * only sgtl5000 controll object is needed from audio library
 *  
 * CONFIGURATION  (Audio board)
 * Teensy 3 (I2S0)
 * PIN  11 (6)  PTC6  MCLK
 * PIN  23 (6)  PTC2  TX_FS
 * PIN   9 (6)  PTC3  TX_BCLK
 * PIN  13 (4)  PTC5  RXD0
 * PIN  30 (4)  PTC11 RXD1 (T3.2)
 * PIN  38 (4)  PTC11 RXD1 (T3.5/6)
 * PIN  22 (6)  PTC1  TXD0
 * PIN  15 (6)  PTC0  TXD1
 * 
 * Pin  10  CS
 * Pin   7  MOSI
 * Pin  12  MISO
 * Pin  14  SCK
 * 
 * Teensy 4
 * PIN  23 (3)  MCLK
 * PIN  20 (3)  RX_FS
 * PIN  21 (3)  RX_BCLK
 * PIN   8 (3)  RXD0
 * PIN   6 (3)  RXD1
 * PIN   7 (3)  TXD0
 * PIN  32 (3)  TXD1
 * 
 * Pin  10  CS
 * Pin  11  MOSI
 * Pin  12  MISO
 * Pin  13  SCK
 * 
 */
#include "Arduino.h"

#define USE_SDIO 0  // set to 1 if sdio exists and should be used

#define NDAT 128L // number of data samples per audio block
#define NCH_I2S 2 // number of total I2S channels
#define N_ADC 1   // number of I2S data ports
#define FRAME_I2S (NCH_I2S/N_ADC) // frame size for each I2S data port

const int ichan[]= {0,1}; // channels to store, must be less or equal than NCH_I2S
const int NCH_ACQ = sizeof(ichan)/4;

#define NBUF_ACQ (NCH_ACQ*NDAT)
#define NBUF_I2S (NCH_I2S*NDAT)

#define NDBL 8 // number of buffers for disk-write (multiple of NBUF_ACQ to speed up disk transfer)

#include "control_sgtl5000.h"
AudioControlSGTL5000 audioShield;

//#define AUDIO_SELECT AUDIO_INPUT_LINEIN 
#define AUDIO_SELECT AUDIO_INPUT_MIC

#define MicGain 0 // (0 - 64) dB

// for SGTL5000
// need MCLK = 256*FS
// BCLK = 2*32*FS =64*FS
// BCLK = MCLK/4 =MCLK/(2*BIT_DIV)

const uint32_t BIT_DIV  = 2;   
const int ovr = 2*FRAME_I2S*32; // factor 2 is buildin in bitclock generation

// nominal frequencies for sgtl5000
const uint32_t fsamps[] = {8000, 16000, 32000, 44100, 48000, 96000, 192000};
#define IFR 5
const int32_t fsamp=fsamps[IFR];

/************************************************************************************************/
#include "SD.h"
#include "MTP.h"

#if defined(__IMXRT1062__)
  // following only as long usb_mtp is not included in cores
  #if !__has_include("usb_mtp.h")
    #include "usb1_mtp.h"
  #endif
#else
  #ifndef BUILTIN_SDCARD 
    #define BUILTIN_SDCARD 254
  #endif
  void usb_mtp_configure(void) {}
#endif

#if USE_EVENTS==1
  extern "C" int usb_init_events(void);
#else
  int usb_init_events(void) {}
#endif

/****  Start device specific change area  ****/
#define USE_SD  1
#define USE_LITTLEFS 0 // set to zero if no LtttleFS is existing or is to be used

#if USE_SD==1
  // edit SPI to reflect your configuration (following is for T4.x)
  #if defined(KINETISK)   // Teensy 3 Audio board
    #define SD_MOSI 7
    #define SD_MISO 12
    #define SD_SCK  14
  #else                  // Teensy 4 Audio board
    #define SD_MOSI 11
    #define SD_MISO 12
    #define SD_SCK  13
  #endif

  #define SPI_SPEED SD_SCK_MHZ(33)  // adjust to sd card 

  #if USE_SDIO==1
    const char *sd_str[]={"sdio","sd1"}; // edit to reflect your configuration
    const int cs[] = {BUILTIN_SDCARD,10}; // edit to reflect your configuration
  #else
    const char *sd_str[]={"sd1"}; // edit to reflect your configuration
    const int cs[] = {10}; // edit to reflect your configuration
  #endif
  const int nsd = sizeof(sd_str)/sizeof(const char *);

SDClass sdx[nsd];
#endif

//LittleFS classes
#if USE_LITTLEFS==1
  #include "LittleFS.h"
  const char *lfs_str[]={"RAM1","RAM2"};     // edit to reflect your configuration
  const int lfs_size[] = {2'000'000,4'000'000};
  const int nfs = sizeof(lfs_size)/sizeof(int);

  LittleFS_RAM ramfs[nfs]; // needs to be declared if LittleFS is used in storage.h
#endif

MTPStorage_SD storage;
MTPD       mtpd(&storage);


void storage_configure()
{
  #if USE_SD==1
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    for(int ii=0; ii<nsd; ii++)
    { 
      #if defined(BUILTIN_SDCARD)
        if(cs[ii] == BUILTIN_SDCARD)
        {
          if(!sdx[ii].sdfs.begin(SdioConfig(FIFO_SDIO))) 
          { Serial.printf("SDIO Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
          }
          else
          {
            storage.addFilesystem(sdx[ii], sd_str[ii]);
            uint64_t totalSize = sdx[ii].totalSize();
            uint64_t usedSize  = sdx[ii].usedSize();
            Serial.printf("SDIO Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
            Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
          }
        }
        else if(cs[ii]<BUILTIN_SDCARD)
      #endif
      {
        pinMode(cs[ii],OUTPUT); digitalWriteFast(cs[ii],HIGH);
        if(!sdx[ii].sdfs.begin(SdSpiConfig(cs[ii], SHARED_SPI, SPI_SPEED))) 
        { Serial.printf("SD Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
        }
        else
        {
          storage.addFilesystem(sdx[ii], sd_str[ii]);
          uint64_t totalSize = sdx[ii].totalSize();
          uint64_t usedSize  = sdx[ii].usedSize();
          Serial.printf("SD Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
          Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
        }
      }
    }
    #endif

    #if USE_LITTLEFS==1
    for(int ii=0; ii<nfs;ii++)
    {
      { if(!ramfs[ii].begin(lfs_size[ii])) { Serial.println("No storage"); while(1);}
        storage.addFilesystem(ramfs[ii], lfs_str[ii]);
      }
      uint64_t totalSize = ramfs[ii].totalSize();
      uint64_t usedSize  = ramfs[ii].usedSize();
      Serial.printf("Storage %d %s ",ii,lfs_str[ii]); Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
    }
    #endif
}
/****  End of device specific change area  ****/

void logg(uint32_t del, const char *txt);

int16_t state;
int16_t do_logger(uint16_t store, int16_t state);
int16_t do_menu(int16_t state);
int16_t check_filing(int16_t state);

void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) ;

void acq_setup(void);
void acq_stop();
void acq_init(int32_t fsamp);
int16_t acq_check(int16_t state);

void SGTL5000_modification(const int32_t fs);
void printTimestamp(uint32_t tt);

#include "TimeLib.h" // for setSyncProvider

void setup()
{ while(!Serial && millis()<3000); 
  Serial.println("MTP logger");
  setSyncProvider((long int (*)()) rtc_get);

  printTimestamp(rtc_get());

  #if USE_EVENTS==1
    usb_init_events();
  #endif

  #if !__has_include("usb_mtp.h")
    usb_mtp_configure();
  #endif
  storage_configure();

  #if USE_SD==1
    // Set Time callback // needed for SDFat
    FsDateTime::callback = dateTime;
  #endif

  acq_setup();
  acq_stop();
  acq_init(fsamp);

  audioShield.enable();
  audioShield.inputSelect(AUDIO_SELECT);  //AUDIO_INPUT_LINEIN or AUDIO_INPUT_MIC

  delay(10);
  SGTL5000_modification(fsamp); // must be called after I2S initialization stabilized 
  
  
  #if AUDIO_SELECT == AUDIO_INPUT_MIC
    audioShield.micGain(MicGain);
  #endif

  state=-1;

  Serial.println("Setup done");
  Serial.println(" Enter 's' to start, 'q' to stop acquisition and 'r' to restart MTP");
  Serial.flush();
}

uint32_t loop_count=0;
void loop()
{ loop_count++;
  state = do_menu(state);
  state = acq_check(state);
  state = check_filing(state);
  //
  if(state<0)
    mtpd.loop();
  else
    state=do_logger(0,state);

  if(state>=0) logg(1000,"loop");
  //asm("wfi"); // may wait forever on T4.x
}

/**************** Online logging *******************************/
extern uint32_t loop_count, acq_count, acq_miss, maxDel;
extern uint16_t maxCount;
void logg(uint32_t del, const char *txt)
{ static uint32_t to;
  if(millis()-to > del)
  {
    Serial.printf("%s: %6d %4d; %4d %4d; %4d\n",
            txt,loop_count, acq_count, acq_miss,maxCount, maxDel); 
    loop_count=0;
    acq_count=0;
    acq_miss=0;
    maxCount=0;
    maxDel=0;
    to=millis();
  }
}

/*************************** Circular Buffer ********************/
#if defined(ARDUINO_TEENSY41)
  #define HAVE_PSRAM 1
#else
  #define HAVE_PSRAM 0
#endif

  #if HAVE_PSRAM==1
    #define MAXBUF (1000) // 3000 kB   // < 5461 for 16 MByte PSRAM

//    extern "C" uint8_t external_psram_size;
//    uint8_t size = external_psram_size;
//    uint32_t *memory_begin = (uint32_t *)(0x70000000);
//    uint32_t *data_buffer = memory_begin;

    uint32_t *data_buffer = (uint32_t *)extmem_malloc(MAXBUF*128*sizeof(uint32_t));

  #else
    #if defined(ARDUINO_TEENSY41)
      #define MAXBUF (((500/NCH_ACQ)/NDBL)*NDBL)           // 256 kB (4*500*128)
    #elif defined(ARDUINO_TEENSY40)
      #define MAXBUF (((500/NCH_ACQ)/NDBL)*NDBL)           // 256 kB (4*500*128)
    #elif defined(__MK66FX1M0__)
      #define MAXBUF (((150/NCH_ACQ)/NDBL)*NDBL)           // 128 kB (4*250*128)
    #elif defined(__MK64FX512__)
      #define MAXBUF (((100/NCH_ACQ)/NDBL)*NDBL)           // 51.2 kB (4*100*128)
    #elif defined(__MK20DX256__)
      #define MAXBUF (((50/NCH_ACQ)/NDBL)*NDBL)              // 25.6 kB (4*50*128)
    #endif
    uint32_t data_buffer[MAXBUF*NBUF_ACQ];
  #endif

static uint16_t front_ = 0, rear_ = 0;
uint16_t getCount () { if(front_ >= rear_) return front_ - rear_; return front_+ MAXBUF -rear_; }
uint16_t maxCount=0;

void resetData(void) {  front_ = 0;  rear_ = 0; }

uint16_t pushData(uint32_t * src)
{ uint16_t f =front_ + 1;
  if(f >= MAXBUF) f=0;
  if(f == rear_) return 0;

  uint32_t *ptr= data_buffer+f*NBUF_ACQ;
  memcpy(ptr,src,NBUF_ACQ*4);
  front_ = f;
  //
  uint16_t count;
  count = (front_ >= rear_) ? (front_ - rear_) : front_+ (MAXBUF -rear_) ;
  if(count>maxCount) maxCount=count;
  //
  return 1;
}

uint16_t pullData(uint32_t * dst, uint32_t ndbl)
{ uint16_t r = (rear_/ndbl) ;
  if(r == (front_/ndbl)) return 0;
  if(++r >= (MAXBUF/ndbl)) r=0;
  uint32_t *ptr= data_buffer + r*ndbl*NBUF_ACQ;
  memcpy(dst,ptr,ndbl*NBUF_ACQ*4);
  rear_ = r*ndbl;
  return 1;
}

/*************************** Filing *****************************/
int16_t file_open(uint16_t store);
int16_t file_writeHeader(void);
int16_t file_writeData(void *diskBuffer, uint32_t ndbl);
int16_t file_close(void);

#define NBUF_DISK (NDBL*NBUF_ACQ)
uint32_t diskBuffer[NBUF_DISK];
uint32_t maxDel=0;

int16_t do_logger(uint16_t store, int16_t state)
{ uint32_t to=millis();
  if(pullData(diskBuffer,NDBL))
  {
    if(state==0)
    { // acquisition is running, need to open file
      if(!file_open(store)) return -2;
      state=1;
    }
    if(state==1)
    { // file just opended, need to write header
      if(!file_writeHeader()) return -3;
      state=2;
      
    }
    if(state>=2)
    { // write data to disk
      if(!file_writeData(diskBuffer,NBUF_DISK*4)) return -4;
    }
  }

  if(state==3)
  { // close file, but continue acquisition
    if(!file_close()) return -5;
    state=0;
  }

  if(state==4)
  { // close file and stop acquisition
    if(!file_close()) return -6;
    state=-1;
  }

  uint32_t dt=millis()-to;
  if(dt>maxDel) maxDel=dt;

  return state;
}

/******************** Menu interface ***************************/
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
      Serial.println("\nStart");
      break;
    case 'q': // stop acquisition
      if(state<0) return state;
      state=4;
      Serial.println("\nStop");
      break;
#if USE_EVENTS==1
    case 'r': 
      Serial.println("Reset");
      mtpd.send_DeviceResetEvent();
      break;
#endif
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
#include "SD.h"
extern SDClass sdx[];
static File mfile;

char header[512];

void makeHeader(char *header);
int16_t makeFilename(char *filename);
int16_t checkPath(uint16_t store, char *filename);

int16_t file_open(uint16_t store)
{ char filename[80];
  if(!makeFilename(filename)) return 0;
  if(!checkPath(store, filename)) return 0;
  mfile = sdx[store].open(filename,FILE_WRITE);
  return !(!mfile);
}

int16_t file_writeHeader(void)
{ if(!mfile) return 0;
  makeHeader(header);
  size_t nb = mfile.write(header,512);
  return (nb==512);
}

int16_t file_writeData(void *diskBuffer, uint32_t nd)
{ if(!mfile) return 0;
  uint32_t nb = mfile.write(diskBuffer,nd);
  return (nb==nd);
}

int16_t file_close(void)
{ mfile.close();
  return (!mfile);
}

/*
 * Custom Implementation
 * 
 */

/****************** Time Utilities *****************************/
#include "TimeLib.h"
void printTimestamp(uint32_t tt)
{
  tmElements_t tm;
  breakTime(tt, tm);
  Serial.printf("Now: %04d-%02d-%02d_%02d:%02d:%02d\r\n", 
                      tmYearToCalendar(tm.Year), tm.Month, tm.Day, tm.Hour, tm.Minute, tm.Second);
}

/************************ some utilities modified from time.cpp ************************/
// leap year calculator expects year argument as years offset from 1970
#define LEAP_YEAR(Y) ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; 

void day2date(uint32_t dd, uint32_t *day, uint32_t *month, uint32_t *year)
{
  uint32_t yy= 0;
  uint32_t days = 0;
  while((unsigned)(days += (LEAP_YEAR(yy) ? 366 : 365)) <= dd) {yy++;}
  
  days -= LEAP_YEAR(yy) ? 366 : 365;
  dd  -= days; // now it is days in this year, starting at 0

  uint32_t mm=0;
  uint32_t monthLength=0;
  for (mm=0; mm<12; mm++) 
  {
    monthLength = monthDays[mm];
    if ((mm==1) && (LEAP_YEAR(yy))) monthLength++;
    if (dd >= monthLength) { dd -= monthLength; } else { break; }
  }

  *month =mm + 1;   // jan is month 1  
  *day  = dd + 1;   // day of month
  *year = yy + 1970;
}

void date2day(uint32_t *dd, uint32_t day, uint32_t month, uint32_t year)
{
  day -= 1;
  month -= 1;
  year -= 1970;
  uint32_t dx=0;
  for (uint32_t ii = 0; ii < year; ii++) { dx += LEAP_YEAR(ii)? 366: 365; } 
  for (uint32_t ii = 0; ii < month; ii++)
  {
    dx += monthDays[ii];
    if((ii==2) && (LEAP_YEAR(year))) dx++; // after feb check for leap year
  }
  *dd = dx + day;
}

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

/****************** File Utilities *****************************/
  #if USE_SD==1
    // Call back for file timestamps.  Only called for file create and sync()
    void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
    { *date = FS_DATE(year(), month(), day());
      *time = FS_TIME(hour(), minute(), second());
      *ms10 = second() & 1 ? 100 : 0;
    }
  #endif


void makeHeader(char *header)
{
  memset(header,0,512);
  sprintf(header,"WMXZ");
  int32_t *ihdr=(int32_t *)&header[strlen(header)+1];
  uint32_t *uhdr=(uint32_t *)&header[strlen(header)+1];
  ihdr[0]=1; // version number
  uhdr[1]=rtc_get();
  uhdr[2]=micros();
}

int16_t makeFilename(char *filename)
{
  uint32_t tt = rtc_get();
  int hh,mm,ss;
  int dd;
  ss= tt % 60; tt /= 60;
  mm= tt % 60; tt /= 60;
  hh= tt % 24; tt /= 24;
  dd= tt;
  sprintf(filename,"/%d/%02d_%02d_%02d.raw",dd,hh,mm,ss);
  Serial.println(filename);
  return 1;
}

int16_t checkPath(uint16_t store, char *filename)
{
  int ln=strlen(filename);
  int i1=-1;
  for(int ii=0;ii<ln;ii++) if(filename[ii]=='/') i1=ii;
  if(i1<0) return 1; // no path
  filename[i1]=0;
  if(!sdx[store].exists(filename))
  { Serial.println(filename); 
    if(!sdx[store].mkdir(filename)) return 0;
  }

  filename[i1]='/';
  return 1;
}

uint32_t t_on = 60;
int16_t check_filing(int16_t state)
{
  static uint32_t to;
  if(state==2)
  {
    uint32_t tt = rtc_get();
    uint32_t dt = tt % t_on;
    if(dt<to) state = 3;
    to = dt;
  }
  return state;
}

/****************** Data Acquisition *******************************************/

  static uint32_t tdm_rx_buffer[2*NBUF_I2S];
  static uint32_t acq_rx_buffer[NBUF_ACQ];
  #define I2S_DMA_PRIO 6

  #include "DMAChannel.h"
  DMAChannel dma;

  void acq_isr(void);

  #if defined(KINETISK)
  //Teensy 3.2, 3.6 or 3.5
      #define MCLK_SRC  3

    // estimate MCLK to generate precise sampling frequency (typically equal or greater desired frequency)
    void Adjust_MCLK(int32_t fsamp, int32_t *mult, int32_t *div)
    {
      float A = (float)F_PLL / (BIT_DIV*ovr);
      float X[255];
      for(int ii=0; ii<255;ii++) X[ii]=A/((float) fsamp)*(ii+1);
      int32_t iimin=0;
      float xmin=1.0f;
      for(int ii=0; ii<255;ii++) if((X[ii]<4096.0f) && (fmodf(X[ii],1.0f)<xmin)) {iimin=ii; xmin=fmodf(X[ii],1.0f);}
      *mult=iimin+1;
      *div=(int) X[iimin];
    }

    // estimate MCLK to generate minimal jitter sampling frequency, allowing sampling frequency to differ from desired one
    void Approx_MCLK(int32_t fsamp, int32_t *mult, int32_t *div)
    {
      int ii;
      float A = (float)F_PLL / (BIT_DIV*ovr);
      float X[4095];
      for(ii=0; ii<4095;ii++) X[ii]=A*2.0/(float)(ii+1);
      int32_t iimin=0;
      float xmin=fsamp;
      for(ii=0; ii<4095;ii++) if(abs(X[ii]-fsamp)<xmin) {iimin=ii; xmin=abs(X[ii]-fsamp);}
      *mult=2;
      *div= iimin+1;
    }

    void acq_start(void)
    {
          I2S0_TCSR |= (I2S_TCSR_TE | I2S_TCSR_BCE);
          I2S0_RCSR |= (I2S_RCSR_RE | I2S_RCSR_BCE);
    }
    void acq_stop(void)
    { 
          I2S0_TCSR &= ~(I2S_TCSR_TE | I2S_TCSR_BCE);
          I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
    }

    void acq_setup(void)
    {
        SIM_SCGC6 |= SIM_SCGC6_I2S;
        SIM_SCGC7 |= SIM_SCGC7_DMA;
        SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
    }

    void acq_init(int32_t fsamp)
    {
      int32_t MCLK_MULT, MCLK_DIV;

#define EST_MCKL 0 // 0 precise frequency, 1 minimal jitter
#if EST_MCKL==0
      Adjust_MCLK(fsamp,&MCLK_MULT,&MCLK_DIV);
#elif EST_MCKL==1
      Approx_MCLK(fsamp,&MCLK_MULT,&MCLK_DIV);
#endif
      float fract = (float)MCLK_MULT/((float) MCLK_DIV);
      int32_t mclk0=((F_PLL)*fract);
      int32_t fsamp0=(mclk0/(BIT_DIV*ovr));

      Serial.printf("%d %d %d %d %d %d %d\n",F_CPU, MCLK_MULT, MCLK_DIV, BIT_DIV, mclk0,fsamp,fsamp0); Serial.flush();

/*
P9  PTC3   I2S0_TX_BCLK (6)
P11 PTC6   I2S0_MCLK (6)    I2S0_RX_BCLK (4)
P12 PTC7                    I2S0_RX_FS (4)
P13 PTC5                    I2S0_RXD0 (4)
P23 PTC2   I2S0_TX_FS (6)
P27 PTA15  I2S0_RXD0 (6)
P35 PTC8                    I2S0_MCLK (4)
P36 PTC9                    I2S0_RX_BCLK (4)
P37 PTC10                   I2S0_RX_FS (4)
P38 PTC11                   I2S0_RXD1 (4)
P39 PTA17  I2S0_MCLK (6)
*/
        CORE_PIN11_CONFIG = PORT_PCR_MUX(6);   // PTC6,  I2S0_MCLK
        CORE_PIN9_CONFIG = PORT_PCR_MUX(6);    // PTC3,  I2S0_TX_BCLK
        CORE_PIN23_CONFIG = PORT_PCR_MUX(6);   // PTC2,  I2S0_TX_FS 

        I2S0_RCSR=0;

        // enable MCLK output // MCLK = INP *((MULT)/(DIV))
        I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));
        while(I2S0_MCR & I2S_MCR_DUF);
        I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
        
        // configure transmitter
        I2S0_TMR = 0;
        I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
        I2S0_TCR2 = I2S_TCR2_SYNC(0) 
                    | I2S_TCR2_BCP 
                    | I2S_TCR2_MSEL(1)
                    | I2S_TCR2_BCD 
                    | I2S_TCR2_DIV((BIT_DIV-1));
        I2S0_TCR4 = I2S_TCR4_FRSZ((FRAME_I2S-1)) 
                    | I2S_TCR4_SYWD(31)
                    | I2S_TCR4_MF
                    | I2S_TCR4_FSE 
                    | I2S_TCR4_FSP 
                    | I2S_TCR4_FSD;
        I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

      	// configure receiver (sync'd to transmitter clocks)
        I2S0_RMR=0;
        I2S0_RCR1 = I2S_RCR1_RFW(1); 

        I2S0_RCR2 = I2S_RCR2_SYNC(1) 
                    | I2S_RCR2_BCP 
                    | I2S_RCR2_BCD  // Bit clock in master mode
                    | I2S_RCR2_DIV((BIT_DIV-1)); // divides MCLK down to Bitclock (BIT_DIV)*2
                    
        I2S0_RCR4 = I2S_RCR4_FRSZ((FRAME_I2S-1)) 
                    | I2S_RCR4_SYWD(31)
                    | I2S_RCR4_FSE  // frame sync early
                    | I2S_RCR4_FSP 
                    | I2S_RCR4_FSD  // Frame sync in master mode
                    | I2S_RCR4_MF;
        
        I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);


  dma.begin(true); // Allocate the DMA channel first

#if N_ADC==1
          CORE_PIN13_CONFIG = PORT_PCR_MUX(4);  // PTC5,  I2S0_RXD0

          I2S0_TCR3 = I2S_TCR3_TCE;
          I2S0_RCR3 = I2S_RCR3_RCE;

          dma.TCD->SADDR = &I2S0_RDR0;
          dma.TCD->SOFF = 0;
          dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
          dma.TCD->NBYTES_MLNO = 4;
          dma.TCD->SLAST = 0;
#elif N_ADC==2
          CORE_PIN13_CONFIG = PORT_PCR_MUX(4);  // PTC5,  I2S0_RXD0
        #if defined(__MK20DX256__)
          CORE_PIN30_CONFIG = PORT_PCR_MUX(4); // pin 30, PTC11, I2S0_RXD1
        #elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
          CORE_PIN38_CONFIG = PORT_PCR_MUX(4); // pin 38, PTC11, I2S0_RXD1
        #endif

          I2S0_RCR3 = I2S_RCR3_RCE_2CH;

          dma.TCD->SADDR = &I2S0_RDR0;
          dma.TCD->SOFF = 4;
          dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
          dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
              DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) |
              DMA_TCD_NBYTES_MLOFFYES_NBYTES(8);
          dma.TCD->SLAST = -8;
#endif
          dma.TCD->DADDR = tdm_rx_buffer;
          dma.TCD->DOFF = 4;
          dma.TCD->CITER_ELINKNO = dma.TCD->BITER_ELINKNO = NDAT*FRAME_I2S*2;
          dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
          dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
          dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
          dma.enable();

          I2S0_RCSR = I2S_RCSR_FRDE | I2S_RCSR_FR;
          I2S0_TCSR = I2S_RCSR_FRDE | I2S_RCSR_FR;
          dma.attachInterrupt(acq_isr,I2S_DMA_PRIO*16);	
          acq_start();
    }


  #elif defined(__IMXRT1062__)
  //Teensy 4.x

    #define IMXRT_CACHE_ENABLED 2 // 0=disabled, 1=WT, 2= WB

    /************************* I2S *************************************************/
    
    void set_audioClock(int nfact, int32_t nmult, uint32_t ndiv) // sets PLL4
      {
          CCM_ANALOG_PLL_AUDIO = CCM_ANALOG_PLL_AUDIO_BYPASS | CCM_ANALOG_PLL_AUDIO_ENABLE
                      | CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(2) // 2: 1/4; 1: 1/2; 0: 1/1
                      | CCM_ANALOG_PLL_AUDIO_DIV_SELECT(nfact);

          CCM_ANALOG_PLL_AUDIO_NUM   = nmult & CCM_ANALOG_PLL_AUDIO_NUM_MASK;
          CCM_ANALOG_PLL_AUDIO_DENOM = ndiv & CCM_ANALOG_PLL_AUDIO_DENOM_MASK;
          
          CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN;//Switch on PLL
          while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK)) {}; //Wait for pll-lock
          
          const int div_post_pll = 1; // other values: 2,4
          CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_DIV_MSB | CCM_ANALOG_MISC2_DIV_LSB);
          if(div_post_pll>1) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_LSB;
          if(div_post_pll>3) CCM_ANALOG_MISC2 |= CCM_ANALOG_MISC2_DIV_MSB;
          
          CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_BYPASS;   //Disable Bypass
      }

      void acq_start(void)
      {
          //DMA_SERQ = dma.channel;
          I2S1_RCSR |= (I2S_RCSR_RE | I2S_RCSR_BCE);
      }

      void acq_stop(void)
      {
          I2S1_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
          //DMA_CERQ = dma.channel;     
      }

      void acq_setup(void)
      {

      }

      void acq_init(int32_t fsamp)
      {
          CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

          // if either transmitter or receiver is enabled, do nothing
          if (I2S1_RCSR & I2S_RCSR_RE) return;
          //PLL:
          int ovr = BIT_DIV*2*FRAME_I2S*32;
          // PLL between 27*24 = 648MHz und 54*24=1296MHz
          int n1 = 4;                    //4; //SAI prescaler 4 => (n1*n2) = multiple of 4
          int n2 = 1 + (24000000 * 27) / (fsamp * ovr * n1);

          float C = ((float)fsamp * ovr * n1 * n2) / 24000000.0f;
          // split scale factor into integer and fraction
          int c0 = C;
          int c2 = 10000;
          int c1 = C * c2 - (c0 * c2);
          set_audioClock(c0, c1, c2);

          Serial.printf("fs=%d, n1=%d, n2=%d, %f(>27 && < 54), %d %d/%d\r\n", 
                        fsamp, n1,n2, C, c0,c1,c2);

          // clear SAI1_CLK register locations
          CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
              | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4

          CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
              | CCM_CS1CDR_SAI1_CLK_PRED((n1-1)) // &0x07
              | CCM_CS1CDR_SAI1_CLK_PODF((n2-1)); // &0x3f

          IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
                  | (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK

          I2S1_RMR = 0;
          I2S1_RCR1 = I2S_RCR1_RFW(4);
          I2S1_RCR2 = I2S_RCR2_SYNC(0) 
                      | I2S_TCR2_BCP 
                      | I2S_RCR2_MSEL(1)
                      | I2S_RCR2_BCD 
                      | I2S_RCR2_DIV((BIT_DIV-1));
          
          I2S1_RCR4 = I2S_RCR4_FRSZ((FRAME_I2S-1)) 
                      | I2S_RCR4_SYWD(31) 
                      | I2S_RCR4_MF
                      | I2S_RCR4_FSE 
                      | I2S_RCR4_FSD;
          I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

        	CORE_PIN23_CONFIG = 3;  //1:MCLK 
          CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
          CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
#if N_ADC==1
          I2S1_RCR3 = I2S_RCR3_RCE;
          CORE_PIN8_CONFIG  = 3;  //RX_DATA0
          IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2;

          dma.TCD->SADDR = &I2S1_RDR0;
          dma.TCD->SOFF = 0;
          dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
          dma.TCD->NBYTES_MLNO = 4;
          dma.TCD->SLAST = 0;
#elif N_ADC==2
          I2S1_RCR3 = I2S_RCR3_RCE_2CH;
          CORE_PIN8_CONFIG  = 3;  //RX_DATA0
          CORE_PIN6_CONFIG  = 3;  //RX_DATA1
          IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2; // GPIO_B1_00_ALT3, pg 873
          IOMUXC_SAI1_RX_DATA1_SELECT_INPUT = 1; // GPIO_B0_10_ALT3, pg 873

          dma.TCD->SADDR = &I2S1_RDR0;
          dma.TCD->SOFF = 4;
          dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
          dma.TCD->NBYTES_MLOFFYES = DMA_TCD_NBYTES_SMLOE |
              DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) |
              DMA_TCD_NBYTES_MLOFFYES_NBYTES(8);
          dma.TCD->SLAST = -8;
#endif
          dma.TCD->DADDR = tdm_rx_buffer;
          dma.TCD->DOFF = 4;
          dma.TCD->CITER_ELINKNO = dma.TCD->BITER_ELINKNO = NDAT*FRAME_I2S*2;
          dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
          dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
          dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);
          dma.enable();

          I2S1_RCSR =  I2S_RCSR_FRDE | I2S_RCSR_FR;
          dma.attachInterrupt(acq_isr,I2S_DMA_PRIO*16);	
          acq_start();
      }
  #endif

  uint32_t acq_count=0;
  uint32_t acq_miss=0;

    void acq_isr(void)
    {
        uint32_t daddr;
        uint32_t *src;
        acq_count++;

        daddr = (uint32_t)(dma.TCD->DADDR);
        dma.clearInterrupt();

        if (daddr < (uint32_t)tdm_rx_buffer + sizeof(tdm_rx_buffer) / 2) {
            // DMA is receiving to the first half of the buffer
            // need to remove data from the second half
            src = &tdm_rx_buffer[NBUF_I2S];
        } else {
            // DMA is receiving to the second half of the buffer
            // need to remove data from the first half
            src = &tdm_rx_buffer[0];
        }

        #if IMXRT_CACHE_ENABLED >=1
            arm_dcache_delete((void*)src, sizeof(tdm_rx_buffer) / 2);
        #endif

        for(int jj=0;jj<NCH_ACQ;jj++)
        {
          for(int ii=0; ii<NDAT;ii++)
          {
            acq_rx_buffer[ichan[jj]+ii*NCH_ACQ]=src[ichan[jj]+ii*NCH_I2S];
          }
        }

        if(!pushData(acq_rx_buffer)) acq_miss++;

    }

  int16_t acq_check(int16_t state)
  { if(!state)
    { // start acquisition
      acq_start();
    }
    if(state>3)
    { // stop acquisition
      acq_stop();
    }
    return state;
  }

// ********************************************** following is to change SGTL5000 samling rates ********************
#include "core_pins.h"
#include "Wire.h"

#define SGTL5000_I2C_ADDR_L  0x0A  // CTRL_ADR0_CS pin low (normal configuration)
#define SGTL5000_I2C_ADDR_H  0x2A  // CTRL_ADR0_CS pin high 
#define CHIP_DIG_POWER		0x0002
#define CHIP_CLK_CTRL     0x0004
#define CHIP_I2S_CTRL     0x0006
#define CHIP_ANA_POWER    0x0030 

const int SGTL_ADDR[]={SGTL5000_I2C_ADDR_L,SGTL5000_I2C_ADDR_H};

unsigned int chipRead(int addr, unsigned int reg)
{
  unsigned int val;
  Wire.beginTransmission(SGTL_ADDR[addr]);
  Wire.write(reg >> 8);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(SGTL_ADDR[addr], 2) < 2) return 0;
  val = Wire.read() << 8;
  val |= Wire.read();
  return val;
}

bool chipWrite(int addr, unsigned int reg, unsigned int val)
{
  Wire.beginTransmission(SGTL_ADDR[addr]);
  Wire.write(reg >> 8);
  Wire.write(reg);
  Wire.write(val >> 8);
  Wire.write(val);
  if (Wire.endTransmission() == 0) return true;
  return false;
}

unsigned int chipModify(int addr, unsigned int reg, unsigned int val, unsigned int iMask)
{
  unsigned int val1 = (chipRead(addr, reg)&(~iMask))|val;
  if(!chipWrite(addr, reg,val1)) return 0;
  return val1;
}

void SGTL5000_modification(const int32_t fs)
{ uint32_t sgtl_mode=0;
  if(fs<44000) sgtl_mode=0;
  else if(fs<48000) sgtl_mode=1;
  else if(fs<64000) sgtl_mode=2;
  else sgtl_mode=3;

  //(0: 8kHz, 1: 16 kHz 2:32 kHz, 3:44.1 kHz, 4:48 kHz, 5:96 kHz, 6:192 kHz)

//  write(CHIP_CLK_CTRL, 0x0004);  // 44.1 kHz, 256*Fs
//	write(CHIP_I2S_CTRL, 0x0130); // SCLK=32*Fs, 16bit, I2S format
  chipWrite(0, CHIP_CLK_CTRL, (sgtl_mode<<2));  // 256*Fs| sgtl_mode = 0:32 kHz; 1:44.1 kHz; 2:48 kHz; 3:96 kHz
}

void SGTL5000_enable(void)
{
  chipWrite(0, CHIP_ANA_POWER, 0x40FF); 
  chipWrite(0, CHIP_DIG_POWER, 0x0073); 
}

void SGTL5000_disable(void)
{
  chipWrite(0, CHIP_DIG_POWER, 0); 
  chipWrite(0, CHIP_ANA_POWER, 0); 
}

