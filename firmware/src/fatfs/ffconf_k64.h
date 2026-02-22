/**
 * @file ffconf_k64.h
 * @brief FatFS Configuration for Teensy 3.5 (Kinetis K64)
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * FatFS R0.16 configuration optimized for Teensy 3.5
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef FFCONF_K64_H
#define FFCONF_K64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// FatFS Configuration Options
//=============================================================================

#define FFCONF_DEF  86604   // Revision ID

//-----------------------------------------------------------------------
// Function Configurations
//-----------------------------------------------------------------------

#define FF_FS_READONLY   0     // 0:Read/Write or 1:Read only
#define FF_FS_MINIMIZE    0     // 0 to 3
#define FF_USE_STRFUNC    2     // 0:Disable or 1-2:Enable
#define FF_USE_FIND       1     // 0:Disable or 1:Enable
#define FF_USE_MKFS       1     // 0:Disable or 1:Enable
#define FF_USE_FASTSEEK   1     // 0:Disable or 1:Enable
#define FF_USE_EXPAND     0     // 0:Disable or 1:Enable
#define FF_USE_CHMOD      0     // 0:Disable or 1:Enable
#define FF_USE_LABEL      1     // 0:Disable or 1:Enable
#define FF_USE_UNICODE    0     // 0:Disable or 1:Enable
#define FF_USE_LFN        0     // 0:Disable or 1:Enable with static LFN working buffer
#define FF_MAX_LFN        255    // Maximum LFN length to handle (12 to 255)
#define FF_LFN_UNICODE    0     // 0:ANSI/OEM or 1:Unicode
#define FF_LFN_BUF        255    // Size of LFN buffer (must be >= FF_MAX_LFN)
#define FF_SFN_BUF        12     // Size of SFN buffer
#define FF_STRF_ENCODE    3     // 0:ANSI/OEM, 1:UTF-16LE, 2:UTF-16BE, 3:UTF-8
#define FF_FS_RPATH       0     // 0 to 2

//-----------------------------------------------------------------------
// Drive Configurations
//-----------------------------------------------------------------------

#define FF_VOLUMES        1     // Number of volumes (logical drives) to be used
#define FF_STR_VOLUME_ID  0     // 0:Use only 0-9 for drive ID, 1:Use strings for drive ID
#define FF_VOLUME_STRS    "RAM","NAND","CF","SD","SD2","USB","USB2","USB3"
#define FF_MULTI_PARTITION 0     // 0:Single partition, 1:Multiple partition
#define FF_MIN_SS         512    // 512, 1024, 2048 or 4096
#define FF_MAX_SS         512    // 512, 1024, 2048 or 4096
#define FF_LBA64         0     // 0:Use 32-bit LBA, 1:Use 64-bit LBA
#define FF_MIN_GPT        0x100000 // Minimum number of sectors to create GPT

//-----------------------------------------------------------------------
// System Configurations
//-----------------------------------------------------------------------

#define FF_FS_TINY        0     // 0:Normal or 1:Tiny
#define FF_FS_EXFAT       0     // 0:Disable or 1:Enable
#define FF_FS_NORTC       1     // 0:Enable RTC feature, 1:Disable RTC feature
#define FF_FS_NOFSINFO    0     // 0:Enable FSINFO, 1:Disable FSINFO
#define FF_FS_LOCK        0     // 0:Disable or 1:Enable
#define FF_FS_REENTRANT   0     // 0:Disable or 1:Enable
#define FF_FS_TIMEOUT     1000   // Timeout period in unit of time ticks
#define FF_SYNC_t         HANDLE // O/S dependent sync object type

//-----------------------------------------------------------------------
// Stack and Heap for Teensy 3.5
//-----------------------------------------------------------------------

#define FF_USE_LONGLONG   1     // 0:Use 32-bit file size, 1:Use 64-bit file size
#define FF_WORD_ACCESS    0     // 0:Byte-by-byte, 1:Word access

//-----------------------------------------------------------------------
// Teensy 3.5 Specific Optimizations
//-----------------------------------------------------------------------

// Optimize for K64 memory constraints
#define FF_FS_REENTRANT   0     // Disable reentrancy to save RAM
#define FF_FS_LOCK        0     // Disable file locking to save RAM
#define FF_USE_LFN        0     // Disable long filenames to save RAM
#define FF_FS_EXFAT       0     // Disable exFAT to save Flash

// SD card specific settings
#define FF_MIN_SS         512    // SD card sector size
#define FF_MAX_SS         512    // SD card sector size
#define FF_VOLUMES        1     // Single SD card volume

#ifdef __cplusplus
}
#endif

#endif // FFCONF_K64_H
