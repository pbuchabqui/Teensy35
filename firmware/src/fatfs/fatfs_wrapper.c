/**
 * @file fatfs_wrapper.c
 * @brief FatFS Wrapper for Teensy 3.5 ECU
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * Simplified FatFS wrapper for rusEFI data logging and configuration storage
 * on Teensy 3.5 with SD card.
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#include "fatfs_wrapper.h"
#include "ff.h"
#include "fatfs_k64.h"
#include <string.h>
#include <stdio.h>

//=============================================================================
// Global Variables
//=============================================================================

static FATFS fs;           // FatFS file system object
static FIL file_object;     // File object
static uint8_t fatfs_initialized = 0;

//=============================================================================
// FatFS Wrapper Functions
//=============================================================================

fatfs_result_t fatfs_init(void) {
    FRESULT res;
    
    if (fatfs_initialized) {
        return FATFS_OK;
    }
    
    // Initialize SD card
    if (disk_initialize(0) != RES_OK) {
        return FATFS_ERROR_DISK_INIT;
    }
    
    // Mount file system
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        // Try to format if mount fails
        if (res == FR_NO_FILESYSTEM) {
            MKFS_PARM fs_opt = {0, 0, 0, 0, 0};  // Default formatting options
            res = f_mkfs("0:", &fs_opt, 0, 0);
            if (res != FR_OK) {
                return FATFS_ERROR_FORMAT;
            }
            
            // Try mounting again
            res = f_mount(&fs, "0:", 1);
            if (res != FR_OK) {
                return FATFS_ERROR_MOUNT;
            }
        } else {
            return FATFS_ERROR_MOUNT;
        }
    }
    
    fatfs_initialized = 1;
    return FATFS_OK;
}

fatfs_result_t fatfs_create_directory(const char* path) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_mkdir(path);
    if (res == FR_OK || res == FR_EXIST) {
        return FATFS_OK;
    }
    
    return FATFS_ERROR_CREATE_DIR;
}

fatfs_result_t fatfs_open_file(const char* filename, fatfs_mode_t mode, fatfs_file_t* file) {
    FRESULT res;
    BYTE fatfs_mode;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    // Convert mode to FatFS mode
    switch (mode) {
        case FATFS_MODE_READ:
            fatfs_mode = FA_READ;
            break;
        case FATFS_MODE_WRITE:
            fatfs_mode = FA_WRITE | FA_CREATE_ALWAYS;
            break;
        case FATFS_MODE_APPEND:
            fatfs_mode = FA_WRITE | FA_OPEN_ALWAYS;
            break;
        default:
            return FATFS_ERROR_INVALID_PARAM;
    }
    
    res = f_open(&file_object, filename, fatfs_mode);
    if (res != FR_OK) {
        return FATFS_ERROR_OPEN_FILE;
    }
    
    *file = (fatfs_file_t)&file_object;
    return FATFS_OK;
}

fatfs_result_t fatfs_close_file(fatfs_file_t file) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_close((FIL*)file);
    if (res != FR_OK) {
        return FATFS_ERROR_CLOSE_FILE;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_read_file(fatfs_file_t file, void* buffer, uint32_t size, uint32_t* bytes_read) {
    FRESULT res;
    UINT br;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_read((FIL*)file, buffer, size, &br);
    if (res != FR_OK) {
        return FATFS_ERROR_READ_FILE;
    }
    
    if (bytes_read) {
        *bytes_read = br;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_write_file(fatfs_file_t file, const void* buffer, uint32_t size, uint32_t* bytes_written) {
    FRESULT res;
    UINT bw;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_write((FIL*)file, buffer, size, &bw);
    if (res != FR_OK) {
        return FATFS_ERROR_WRITE_FILE;
    }
    
    if (bytes_written) {
        *bytes_written = bw;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_seek_file(fatfs_file_t file, uint32_t offset) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_lseek((FIL*)file, offset);
    if (res != FR_OK) {
        return FATFS_ERROR_SEEK;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_get_file_size(fatfs_file_t file, uint32_t* size) {
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    if (!size) {
        return FATFS_ERROR_INVALID_PARAM;
    }
    
    *size = f_size((FIL*)file);
    return FATFS_OK;
}

fatfs_result_t fatfs_flush_file(fatfs_file_t file) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_sync((FIL*)file);
    if (res != FR_OK) {
        return FATFS_ERROR_FLUSH;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_delete_file(const char* filename) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    res = f_unlink(filename);
    if (res != FR_OK) {
        return FATFS_ERROR_DELETE;
    }
    
    return FATFS_OK;
}

fatfs_result_t fatfs_file_exists(const char* filename, uint8_t* exists) {
    FILINFO fno;
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    if (!exists) {
        return FATFS_ERROR_INVALID_PARAM;
    }
    
    res = f_stat(filename, &fno);
    *exists = (res == FR_OK) ? 1 : 0;
    
    return FATFS_OK;
}

fatfs_result_t fatfs_get_free_space(uint32_t* free_clusters, uint32_t* free_sectors, uint32_t* free_bytes) {
    FATFS* pfs;
    DWORD fre_clust, fre_sect, tot_sect;
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_ERROR_NOT_INIT;
    }
    
    pfs = &fs;
    res = f_getfree("0:", &fre_clust, &pfs);
    if (res != FR_OK) {
        return FATFS_ERROR_GET_INFO;
    }
    
    // Get total sectors and free sectors
    tot_sect = (pfs->n_fatent - 2) * pfs->csize;
    fre_sect = fre_clust * pfs->csize;
    
    if (free_clusters) *free_clusters = fre_clust;
    if (free_sectors) *free_sectors = fre_sect;
    if (free_bytes) *free_bytes = fre_sect * FF_MAX_SS;
    
    return FATFS_OK;
}

//=============================================================================
// rusEFI Specific Functions
//=============================================================================

fatfs_result_t fatfs_log_data(const char* log_data, uint32_t size) {
    fatfs_file_t file;
    fatfs_result_t res;
    uint32_t bytes_written;
    
    // Create log directory if it doesn't exist
    fatfs_create_directory("/logs");
    
    // Open log file in append mode
    res = fatfs_open_file("/logs/rusefi.log", FATFS_MODE_APPEND, &file);
    if (res != FATFS_OK) {
        return res;
    }
    
    // Write log data
    res = fatfs_write_file(file, log_data, size, &bytes_written);
    if (res != FATFS_OK) {
        fatfs_close_file(file);
        return res;
    }
    
    // Add newline
    res = fatfs_write_file(file, "\r\n", 2, &bytes_written);
    
    fatfs_close_file(file);
    return res;
}

fatfs_result_t fatfs_save_config(const char* config_name, const void* config_data, uint32_t size) {
    fatfs_file_t file;
    fatfs_result_t res;
    uint32_t bytes_written;
    char filename[256];
    
    // Create config directory if it doesn't exist
    fatfs_create_directory("/config");
    
    // Build filename
    snprintf(filename, sizeof(filename), "/config/%s.bin", config_name);
    
    // Save config file
    res = fatfs_open_file(filename, FATFS_MODE_WRITE, &file);
    if (res != FATFS_OK) {
        return res;
    }
    
    res = fatfs_write_file(file, config_data, size, &bytes_written);
    fatfs_close_file(file);
    
    return res;
}

fatfs_result_t fatfs_load_config(const char* config_name, void* config_data, uint32_t size, uint32_t* bytes_read) {
    fatfs_file_t file;
    fatfs_result_t res;
    char filename[256];
    
    // Build filename
    snprintf(filename, sizeof(filename), "/config/%s.bin", config_name);
    
    // Load config file
    res = fatfs_open_file(filename, FATFS_MODE_READ, &file);
    if (res != FATFS_OK) {
        return res;
    }
    
    res = fatfs_read_file(file, config_data, size, bytes_read);
    fatfs_close_file(file);
    
    return res;
}

fatfs_result_t fatfs_shutdown(void) {
    FRESULT res;
    
    if (!fatfs_initialized) {
        return FATFS_OK;
    }
    
    // Unmount file system
    res = f_mount(NULL, "0:", 0);
    if (res != FR_OK) {
        return FATFS_ERROR_UNMOUNT;
    }
    
    fatfs_initialized = 0;
    return FATFS_OK;
}

//=============================================================================
// Error Code to String Conversion
//=============================================================================

const char* fatfs_error_string(fatfs_result_t result) {
    switch (result) {
        case FATFS_OK: return "Success";
        case FATFS_ERROR_NOT_INIT: return "FatFS not initialized";
        case FATFS_ERROR_DISK_INIT: return "Disk initialization failed";
        case FATFS_ERROR_MOUNT: return "Mount failed";
        case FATFS_ERROR_FORMAT: return "Format failed";
        case FATFS_ERROR_CREATE_DIR: return "Create directory failed";
        case FATFS_ERROR_OPEN_FILE: return "Open file failed";
        case FATFS_ERROR_CLOSE_FILE: return "Close file failed";
        case FATFS_ERROR_READ_FILE: return "Read file failed";
        case FATFS_ERROR_WRITE_FILE: return "Write file failed";
        case FATFS_ERROR_SEEK: return "Seek failed";
        case FATFS_ERROR_FLUSH: return "Flush failed";
        case FATFS_ERROR_DELETE: return "Delete failed";
        case FATFS_ERROR_GET_INFO: return "Get info failed";
        case FATFS_ERROR_UNMOUNT: return "Unmount failed";
        case FATFS_ERROR_INVALID_PARAM: return "Invalid parameter";
        default: return "Unknown error";
    }
}
