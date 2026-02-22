/**
 * @file fatfs_k64.c
 * @brief FatFS R0.16 HAL for Teensy 3.5 (Kinetis K64)
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * This file provides the hardware abstraction layer for FatFS R0.16
 * on the Teensy 3.5 platform using SD card via SPI.
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include "ff.h"
#include "fatfs_k64.h"
#include "../hal/spi_k64.h"
#include "../hal/gpio_k64.h"
#include <string.h>

//=============================================================================
// SD Card Pin Configuration for Teensy 3.5
//=============================================================================

// SPI pins for SD card
#define SD_SPI_PORT     SPI_0
#define SD_CS_PORT      GPIO_PORT_B
#define SD_CS_PIN       GPIO_PIN_0
#define SD_MOSI_PORT    GPIO_PORT_D
#define SD_MOSI_PIN     GPIO_PIN_2
#define SD_MISO_PORT    GPIO_PORT_D
#define SD_MISO_PIN     GPIO_PIN_3
#define SD_SCK_PORT     GPIO_PORT_D
#define SD_SCK_PIN      GPIO_PIN_1

//=============================================================================
// SD Card Commands
//=============================================================================

#define CMD0    0       // GO_IDLE_STATE
#define CMD1    1       // SEND_OP_COND
#define CMD8    8       // SEND_IF_COND
#define CMD9    9       // SEND_CSD
#define CMD10   10      // SEND_CID
#define CMD12   12      // STOP_TRANSMISSION
#define CMD13   13      // SEND_STATUS
#define CMD16   16      // SET_BLOCKLEN
#define CMD17   17      // READ_SINGLE_BLOCK
#define CMD18   18      // READ_MULTIPLE_BLOCK
#define CMD23   23      // SET_BLOCK_COUNT
#define CMD24   24      // WRITE_BLOCK
#define CMD25   25      // WRITE_MULTIPLE_BLOCK
#define CMD32   32      // ERASE_WR_BLK_START
#define CMD33   33      // ERASE_WR_BLK_END
#define CMD38   38      // ERASE
#define CMD55   55      // APP_CMD
#define CMD58   58      // READ_OCR
#define ACMD41  41      // SD_SEND_OP_COND

//=============================================================================
// SD Card Response Types
//=============================================================================

#define R1_NO_ERROR      0x00
#define R1_IDLE_STATE    0x01
#define R1_ERASE_RESET   0x02
#define R1_ILLEGAL_CMD   0x04
#define R1_COM_CRC_ERR   0x08
#define R1_ERASE_SEQ_ERR 0x10
#define R1_ADDR_ERR      0x20
#define R1_PARAM_ERR     0x40

//=============================================================================
// Global Variables
//=============================================================================

static volatile DSTATUS sd_status = STA_NOINIT;
static uint8_t sd_card_type = 0;
static uint32_t sd_sectors = 0;
static uint8_t sd_csd[16];

//=============================================================================
// Low-level SPI Functions
//=============================================================================

static uint8_t sd_spi_write(uint8_t data) {
    uint8_t response;
    spi_transmit_receive(SD_SPI_PORT, &data, &response, 1);
    return response;
}

static void sd_spi_write_block(const uint8_t* data, uint16_t length) {
    uint8_t dummy[length];
    spi_transmit_receive(SD_SPI_PORT, data, dummy, length);
}

static void sd_spi_read_block(uint8_t* data, uint16_t length) {
    uint8_t dummy[length];
    memset(dummy, 0xFF, length);
    spi_transmit_receive(SD_SPI_PORT, dummy, data, length);
}

//=============================================================================
// SD Card Command Functions
//=============================================================================

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
    uint8_t response;
    uint8_t retry = 10;
    
    // Send command
    sd_spi_write(0xFF);  // Dummy byte
    sd_spi_write(cmd | 0x40);  // Command with start bit
    sd_spi_write((arg >> 24) & 0xFF);
    sd_spi_write((arg >> 16) & 0xFF);
    sd_spi_write((arg >> 8) & 0xFF);
    sd_spi_write(arg & 0xFF);
    
    // Send CRC (dummy for most commands)
    if (cmd == CMD0) {
        sd_spi_write(0x95);  // Valid CRC for CMD0
    } else if (cmd == CMD8) {
        sd_spi_write(0x87);  // Valid CRC for CMD8
    } else {
        sd_spi_write(0xFF);  // Dummy CRC
    }
    
    // Wait for response
    do {
        response = sd_spi_write(0xFF);
    } while ((response & 0x80) && --retry);
    
    return response;
}

static uint8_t sd_wait_data_token(void) {
    uint16_t timeout = 10000;
    uint8_t token;
    
    do {
        token = sd_spi_write(0xFF);
    } while ((token == 0xFF) && --timeout);
    
    return token;
}

//=============================================================================
// SD Card Initialization
//=============================================================================

static DSTATUS sd_initialize(void) {
    uint8_t response;
    uint16_t timeout;
    
    // Configure CS pin as output
    gpio_set_direction(SD_CS_PORT, SD_CS_PIN, GPIO_DIR_OUTPUT);
    gpio_set(SD_CS_PORT, SD_CS_PIN);  // CS high (inactive)
    
    // Configure MISO as input
    gpio_set_direction(SD_MISO_PORT, SD_MISO_PIN, GPIO_DIR_INPUT);
    
    // Initialize SPI
    spi_init(SD_SPI_PORT, SPI_MODE_MASTER, SPI_CLOCK_DIV256);  // Start slow
    
    // Set CS high
    gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
    
    // Send 80 clock cycles with CS high
    for (int i = 0; i < 10; i++) {
        sd_spi_write(0xFF);
    }
    
    // Set CS low
    gpio_write(SD_CS_PORT, SD_CS_PIN, 0);
    
    // Send CMD0 (reset card)
    timeout = 1000;
    do {
        response = sd_send_cmd(CMD0, 0);
    } while ((response != R1_IDLE_STATE) && --timeout);
    
    if (response != R1_IDLE_STATE) {
        gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
        return STA_NOINIT;
    }
    
    // Check if card supports CMD8 (SDv2)
    response = sd_send_cmd(CMD8, 0x000001AA);
    if (response == R1_IDLE_STATE) {
        // Card is SDv2
        sd_card_type = 2;
        
        // Get response to CMD8
        for (int i = 0; i < 4; i++) {
            sd_spi_write(0xFF);
        }
        
        // Send ACMD41 (initialize card)
        timeout = 1000;
        do {
            sd_send_cmd(CMD55, 0);
            response = sd_send_cmd(ACMD41, 0x40000000);
        } while ((response != R1_NO_ERROR) && --timeout);
        
        if (response != R1_NO_ERROR) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return STA_NOINIT;
        }
        
        // Read OCR
        sd_send_cmd(CMD58, 0);
        for (int i = 0; i < 4; i++) {
            sd_spi_write(0xFF);
        }
    } else {
        // Card is SDv1 or MMC
        sd_card_type = 1;
        
        // Send ACMD41 (initialize card)
        timeout = 1000;
        do {
            sd_send_cmd(CMD55, 0);
            response = sd_send_cmd(ACMD41, 0);
        } while ((response != R1_NO_ERROR) && --timeout);
        
        if (response != R1_NO_ERROR) {
            // Try MMC initialization
            timeout = 1000;
            do {
                response = sd_send_cmd(CMD1, 0);
            } while ((response != R1_NO_ERROR) && --timeout);
            
            if (response != R1_NO_ERROR) {
                gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
                return STA_NOINIT;
            }
            sd_card_type = 0;
        }
    }
    
    // Set block length to 512
    sd_send_cmd(CMD16, 512);
    
    // Increase SPI speed
    spi_init(SD_SPI_PORT, SPI_MODE_MASTER, SPI_CLOCK_DIV4);  // Fast speed
    
    // Set CS high
    gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
    
    return RES_OK;
}

//=============================================================================
// FatFS Disk I/O Functions
//=============================================================================

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    
    if (sd_status & STA_NOINIT) {
        sd_status = sd_initialize();
    }
    
    return sd_status;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    
    return sd_status;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || (sd_status & STA_NOINIT)) {
        return RES_NOTRDY;
    }
    
    gpio_write(SD_CS_PORT, SD_CS_PIN, 0);
    
    // Read single or multiple blocks
    if (count == 1) {
        // Single block read
        if (sd_send_cmd(CMD17, sector) != R1_NO_ERROR) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
        
        // Wait for data token
        if (sd_wait_data_token() != 0xFE) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
        
        // Read data
        sd_spi_read_block(buff, 512);
        
        // Read CRC (2 bytes)
        sd_spi_write(0xFF);
        sd_spi_write(0xFF);
    } else {
        // Multiple block read
        if (sd_send_cmd(CMD18, sector) != R1_NO_ERROR) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
        
        for (UINT i = 0; i < count; i++) {
            // Wait for data token
            if (sd_wait_data_token() != 0xFE) {
                gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
                return RES_ERROR;
            }
            
            // Read data
            sd_spi_read_block(buff + (i * 512), 512);
            
            // Read CRC (2 bytes)
            sd_spi_write(0xFF);
            sd_spi_write(0xFF);
        }
        
        // Stop transmission
        sd_send_cmd(CMD12, 0);
    }
    
    gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
    
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || (sd_status & STA_NOINIT)) {
        return RES_NOTRDY;
    }
    
    gpio_write(SD_CS_PORT, SD_CS_PIN, 0);
    
    // Write single or multiple blocks
    if (count == 1) {
        // Single block write
        if (sd_send_cmd(CMD24, sector) != R1_NO_ERROR) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
        
        // Send data token
        sd_spi_write(0xFE);
        
        // Send data
        sd_spi_write_block(buff, 512);
        
        // Send dummy CRC
        sd_spi_write(0xFF);
        sd_spi_write(0xFF);
        
        // Check response
        uint8_t response = sd_spi_write(0xFF);
        if ((response & 0x1F) != 0x05) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
    } else {
        // Multiple block write
        if (sd_send_cmd(CMD25, sector) != R1_NO_ERROR) {
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_ERROR;
        }
        
        for (UINT i = 0; i < count; i++) {
            // Send data token
            sd_spi_write(0xFC);
            
            // Send data
            sd_spi_write_block(buff + (i * 512), 512);
            
            // Send dummy CRC
            sd_spi_write(0xFF);
            sd_spi_write(0xFF);
            
            // Check response
            uint8_t response = sd_spi_write(0xFF);
            if ((response & 0x1F) != 0x05) {
                gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
                return RES_ERROR;
            }
        }
        
        // Stop transmission
        sd_spi_write(0xFD);
        sd_spi_write(0xFF);
    }
    
    gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
    
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    if (pdrv != 0 || (sd_status & STA_NOINIT)) {
        return RES_NOTRDY;
    }
    
    switch (cmd) {
        case CTRL_SYNC:
            // Ensure all pending write operations are completed
            gpio_write(SD_CS_PORT, SD_CS_PIN, 0);
            sd_spi_write(0xFF);
            gpio_write(SD_CS_PORT, SD_CS_PIN, 1);
            return RES_OK;
            
        case GET_SECTOR_COUNT:
            // Return number of sectors on the disk
            *(DWORD*)buff = sd_sectors;
            return RES_OK;
            
        case GET_SECTOR_SIZE:
            // Return sector size (always 512 for SD cards)
            *(WORD*)buff = 512;
            return RES_OK;
            
        case GET_BLOCK_SIZE:
            // Return erase block size in sectors
            *(DWORD*)buff = 32;  // 16KB erase block
            return RES_OK;
            
        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void) {
    // Return current time as FAT timestamp
    // This is a simplified implementation
    // In a real implementation, you would get the actual time from RTC
    return ((2026 - 1980) << 25) | (2 << 21) | (21 << 16);  // 2026-02-21 00:00:00
}
