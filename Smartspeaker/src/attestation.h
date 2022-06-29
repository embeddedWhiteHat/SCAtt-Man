#include <stdint.h>

/**
 * @brief Reads Partition-Information
 * Has to be called once prior to first Attestation to get Pointers to
 * each Partition
 */
void attestation_initPartitionTable();

int attestation_calculateHash(uint8_t *nonce, uint8_t *buffer);