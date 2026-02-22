/**
 * @file fatfs_k64.h
 * @brief FatFS R0.16 HAL for Teensy 3.5 (Kinetis K64)
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef FATFS_K64_H
#define FATFS_K64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// FatFS Status Codes
//=============================================================================

#define STA_NOINIT      0x01    // Drive not initialized
#define STA_NODISK      0x02    // No medium in the drive
#define STA_PROTECT     0x04    // Write protected

//=============================================================================
// FatFS Command Codes
//=============================================================================

#define CTRL_SYNC       0       // Complete pending write process
#define GET_SECTOR_COUNT    1   // Get media size (number of sectors)
#define GET_SECTOR_SIZE     2   // Get sector size
#define GET_BLOCK_SIZE      3   // Get erase block size
#define CTRL_TRIM           4   // Inform device that the data on the block of sectors is no longer used

//=============================================================================
// FatFS Result Codes
//=============================================================================

#define RES_OK          0       // Successful
#define RES_ERROR       1       // R/W Error
#define RES_WRPRT       2       // Write Protected
#define RES_NOTRDY      3       // Not Ready
#define RES_PARERR      4       // Invalid Parameter

#ifdef __cplusplus
}
#endif

#endif // FATFS_K64_H
