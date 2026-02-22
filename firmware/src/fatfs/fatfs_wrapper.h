/**
 * @file fatfs_wrapper.h
 * @brief FatFS Wrapper for Teensy 3.5 ECU
 * @version 1.0.0
 * @date 2026-02-21
 * 
 * @copyright Copyright (c) 2026 - GPL v3 License
 * @see https://github.com/pbuchabqui/Teensy35
 */

#ifndef FATFS_WRAPPER_H
#define FATFS_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// FatFS Result Codes
//=============================================================================

typedef enum {
    FATFS_OK = 0,
    FATFS_ERROR_NOT_INIT,
    FATFS_ERROR_DISK_INIT,
    FATFS_ERROR_MOUNT,
    FATFS_ERROR_FORMAT,
    FATFS_ERROR_CREATE_DIR,
    FATFS_ERROR_OPEN_FILE,
    FATFS_ERROR_CLOSE_FILE,
    FATFS_ERROR_READ_FILE,
    FATFS_ERROR_WRITE_FILE,
    FATFS_ERROR_SEEK,
    FATFS_ERROR_FLUSH,
    FATFS_ERROR_DELETE,
    FATFS_ERROR_GET_INFO,
    FATFS_ERROR_UNMOUNT,
    FATFS_ERROR_INVALID_PARAM
} fatfs_result_t;

//=============================================================================
// FatFS File Modes
//=============================================================================

typedef enum {
    FATFS_MODE_READ = 0,
    FATFS_MODE_WRITE,
    FATFS_MODE_APPEND
} fatfs_mode_t;

//=============================================================================
// FatFS File Handle
//=============================================================================

typedef void* fatfs_file_t;

//=============================================================================
// FatFS Function Prototypes
//=============================================================================

// Core functions
fatfs_result_t fatfs_init(void);
fatfs_result_t fatfs_shutdown(void);

// Directory operations
fatfs_result_t fatfs_create_directory(const char* path);

// File operations
fatfs_result_t fatfs_open_file(const char* filename, fatfs_mode_t mode, fatfs_file_t* file);
fatfs_result_t fatfs_close_file(fatfs_file_t file);
fatfs_result_t fatfs_read_file(fatfs_file_t file, void* buffer, uint32_t size, uint32_t* bytes_read);
fatfs_result_t fatfs_write_file(fatfs_file_t file, const void* buffer, uint32_t size, uint32_t* bytes_written);
fatfs_result_t fatfs_seek_file(fatfs_file_t file, uint32_t offset);
fatfs_result_t fatfs_get_file_size(fatfs_file_t file, uint32_t* size);
fatfs_result_t fatfs_flush_file(fatfs_file_t file);
fatfs_result_t fatfs_delete_file(const char* filename);
fatfs_result_t fatfs_file_exists(const char* filename, uint8_t* exists);

// File system information
fatfs_result_t fatfs_get_free_space(uint32_t* free_clusters, uint32_t* free_sectors, uint32_t* free_bytes);

// rusEFI specific functions
fatfs_result_t fatfs_log_data(const char* log_data, uint32_t size);
fatfs_result_t fatfs_save_config(const char* config_name, const void* config_data, uint32_t size);
fatfs_result_t fatfs_load_config(const char* config_name, void* config_data, uint32_t size, uint32_t* bytes_read);

// Utility functions
const char* fatfs_error_string(fatfs_result_t result);

#ifdef __cplusplus
}
#endif

#endif // FATFS_WRAPPER_H
