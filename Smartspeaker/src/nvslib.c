#include "nvslib.h"
#include "nvs_flash.h"
#include "esp_partition.h"

// nice esp logging functionality out-of-the box
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
static const char* TAG = "NVSLIB";

// NVS NameSpace for this project
const static char* nvs_namespace = "nvs_syringepump";
static nvs_handle _nvs_handle;

bool nvslib_begin() {

esp_err_t err = nvs_flash_init();

// Error-Handling for unsuccessfull NVS Init 
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Cannot initialize flash memory");
    if (err != ESP_ERR_NVS_NO_FREE_PAGES)
    { return false; }
    // erase and re-initialize
    ESP_LOGE(TAG, "Try to re-init the partition");
    const esp_partition_t *nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    if (nvs_partition == NULL)
    { return false; }
    err = esp_partition_erase_range(nvs_partition, 0, nvs_partition->size);
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        return false;
    }
    ESP_LOGE(TAG, "NVS partition re-formatted");
}

// No Error-Handling required
err = nvs_open(nvs_namespace, NVS_READWRITE, &_nvs_handle); 
if (err != ESP_OK) 
{ return false; }

return true;
}

void nvslib_close() {
    nvs_close(_nvs_handle);
}

bool nvslib_commit() {
    esp_err_t err = nvs_commit(_nvs_handle);
    if (err != ESP_OK) 
    { return false; } 
    else 
    { return true; }
}

bool nvslib_erase(char* key, bool forceCommit)
{
    esp_err_t err = nvs_erase_key(_nvs_handle, key);
    if (err != ESP_OK) 
    { return false; }
    return forceCommit ? nvslib_commit(): true;
}

bool nvslib_erase_all(bool forceCommit)
{
    esp_err_t err = nvs_erase_all(_nvs_handle);
    if (err != ESP_OK)
    { return false; }
    return forceCommit ? nvslib_commit() : true;
}

bool nvslib_save_blob(char* key, uint8_t *blob, size_t length, bool forceCommit)
{
    ESP_LOGI(TAG, "Object addr: [0x%X], length = [%d]\n", (int32_t)blob, length);
    if (length == 0) {
        return false;
    }
    esp_err_t err = nvs_set_blob(_nvs_handle, key, blob, length);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Save Blob: err = [0x%X]\n", err);
        return false;
    }
    return forceCommit ? nvslib_commit() : true;
}

size_t nvslib_get_blob_size(char* key) {
    size_t required_size;
    esp_err_t err = nvs_get_blob(_nvs_handle, key, NULL, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Get Blob size : err = [0x%X]\n", err);
        return 0;
    }
    return required_size;
}

bool nvslib_get_blob(char* key, uint8_t *blob, size_t length) {
    if (length == 0)
    { 
        ESP_LOGE(TAG, "Length is %d\n", 0);
        return false; }

    size_t required_size = nvslib_get_blob_size(key);

    if (required_size == 0)
    { 
        ESP_LOGE(TAG, "Required size is %d\n", 0);
        return false; }
    if (length < required_size)
    { 
        ESP_LOGE(TAG, "Length smaller then %s\n", "required size");
        return false; }

    esp_err_t err = nvs_get_blob(_nvs_handle, key, blob, &required_size);
    if (err)
    {
        ESP_LOGE(TAG, "Object err = [0x%X]\n", err);
        return false;
    }

    return true;
}

bool nvslib_get_partition_sha256(uint8_t* shaResult, bool print) {
    esp_err_t esp_err;
    const esp_partition_t *nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    if (nvs_partition == NULL)
    {
        ESP_LOGE(TAG, "nvs_partition not found\n"); 
        return false;
    }
    esp_err = esp_partition_get_sha256(nvs_partition, shaResult);
    if (esp_err != ESP_OK) {
        ESP_LOGE(TAG, "SHA256 err = [0x%X]\n", esp_err);
        return false;
    }
    if (print) {
        for (int i = 0; i < 32; i++) { //sha256 requires 32 chars!
            printf("%02x", (int)shaResult[i]);
        }
        printf("\n");
    }
    return true;
}