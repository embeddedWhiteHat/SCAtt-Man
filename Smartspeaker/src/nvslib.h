/*
 * A small libray to quickly store a blob within the nvs system.
 * This library is a limited adaptation of the Arduino nvs abstraction (C++)
 * https://github.com/rpolitex/ArduinoNvs
 */

#ifndef NVSLIB_H
#define NVSLIB_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initializes the nvs flash library,
 * if a nvs partition already exists it will be reinited,
 * otherwise the entire partition will be erased and reformated.
 * In case of success this will return true, otherwise false will indicate
 * that there is an issue with the partition and it can't be initialized or reformated.
 */
bool nvslib_begin();

/**
 * This function closes the ns partition.
 * Call if you no longer need access to the stored values
 * or add further keys.
 */
void nvslib_close();

/**
 * All changes made by the other function,
 * take only effect after nvslib_commit
 * has been called. Commit writes changes to
 * the data partition. 
 */
bool nvslib_commit();

/**
 * Erase a single key from the nvs partition.
 * @param key (The key for the key/value store)
 * @param forceCommit (if true, commit directly)
 */
bool nvslib_erase(char* key, bool forceCommit);


/**
 * Erase all keys from the nvs partition
 * @param forceCommit (if true, commit directly)
 */
bool nvslib_erase_all(bool forceCommit);

/**
 * Save a binary object in the nvs partition
 * @param key (key to reference the blob)
 * @param blob (pointer to the binary object)
 * @param length (size of the binary object as size_t)
 * @param forceCommit (If true, commit directly)
 */
bool nvslib_save_blob(char* key, uint8_t *blob, size_t length, bool forceCommit);

/**
 * Get the size of a binary object stored in the nvs partition
 * @param key (the key referencing the stored object)
 */
size_t nvslib_get_blob_size(char* key);

/**
 * Retrives the blob from the nvs partition
 * @param key (the key referencing the blob object in the nvs partition)
 * @param blob (pointer to a storage space for the blob)
 * @param length (blob size)
 */
bool nvslib_get_blob(char* key, uint8_t *blob, size_t length);

/**
 * Retrieve the SHA256 value of the used nvs partition
 * @param shaResult (pointer to an array of 32 bytes (uint8_t) )
 * @param print (true in order to print to console)
 */
bool nvslib_get_partition_sha256(uint8_t* shaResult, bool print);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif