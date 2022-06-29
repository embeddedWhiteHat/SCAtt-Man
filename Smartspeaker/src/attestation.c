#include <string.h>
#include "esp_partition.h"
#include "mbedtls/sha256.h"

static const esp_partition_t* part_nvs;
static const esp_partition_t* part_phy;
static const esp_partition_t* part_factory;

static const esp_partition_t* get_partition(esp_partition_type_t part_type, esp_partition_subtype_t part_subtype) {
    esp_partition_iterator_t part_iterator = esp_partition_find(part_type, part_subtype, NULL );
    return esp_partition_get(part_iterator);
}

void attestation_initPartitionTable(){
    part_nvs = get_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS);
    part_phy = get_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY);
    part_factory = get_partition(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY);
}

static void buffer_part_to_sha256(mbedtls_sha256_context* ctx,const esp_partition_t* part){
    uint8_t buffer[256];

    for(size_t cur_offset=0; cur_offset < part -> size; cur_offset += sizeof(buffer)){
        // Check if there still is 256 Bytes left, otherwise cut to left size
        size_t bytes_to_read = sizeof(buffer);
        if(part->size - cur_offset < sizeof(buffer)){
            bytes_to_read = part->size - cur_offset;
        }

        esp_partition_read(part, cur_offset, buffer, bytes_to_read);
        mbedtls_sha256_update_ret(ctx, buffer, bytes_to_read);
    }
}

void attestation_calculateHash(uint8_t *nonce, uint8_t *dest){
    // init sha256 with nonce 
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, nonce, 4);

    
    buffer_part_to_sha256(&ctx, part_nvs);
    buffer_part_to_sha256(&ctx, part_phy);
    buffer_part_to_sha256(&ctx, part_factory);

    mbedtls_sha256_finish_ret(&ctx, dest);
    mbedtls_sha256_free(&ctx);
}