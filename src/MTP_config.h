#ifndef MTP_CONFIG_H
#define MTP_CONFIG_H

    #define USE_SDFAT_BETA 1
    #define USE_SDIO 1

    #if USE_SDFAT_BETA == 1
        #include "SdFat-beta.h"
    #else
        #include "SdFat.h"
    #endif
    
    #if defined(__MK20DX256__)
        #if USE_SDIO==1
            #undef USE_SDIO
            #define USE_SDIO 0 // T3.2 has no sdio
        #endif
        #define SD_CS  10
        #define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SPI_FULL_SPEED)
        #define SD_MOSI  7
        #define SD_MISO 12
        #define SD_SCK  14

    #elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
        #if USE_SDIO==1
            // Use FIFO SDIO or DMA_SDIO
            #define SD_CONFIG SdioConfig(FIFO_SDIO)
            //  #define SD_CONFIG SdioConfig(DMA_SDIO)
        #else
            #define SD_CS  10
            #define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SPI_FULL_SPEED)
            #define SD_MOSI  7
            #define SD_MISO 12
            #define SD_SCK  14
        #endif
    #elif defined(__IMXRT1062__)
        #if USE_SDIO==1
            // Use FIFO SDIO or DMA_SDIO
            #define SD_CONFIG SdioConfig(FIFO_SDIO)
            //  #define SD_CONFIG SdioConfig(DMA_SDIO)
        #else
            #define SD_CS  10
            #define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SPI_FULL_SPEED)
            #define SD_MOSI 11
            #define SD_MISO 12
            #define SD_SCK  13
        #endif
    #endif

#endif