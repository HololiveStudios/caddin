#ifndef PET_CORE_PERSIST_H
#define PET_CORE_PERSIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PERSIST_MAGIC 0x43414444u /* "CADD" em ASCII (Big Endian) */
#define PERSIST_VERSION 2

/* Cabeçalho binário estrito para verificação de integridade */
#pragma pack(push, 1)
typedef struct {
    uint32_t magic;         /* PERSIST_MAGIC */
    uint32_t version;       /* PERSIST_VERSION (2) */
    uint32_t neuron_count;  /* Quantidade de neurônios no momento da gravação */
    uint32_t payload_size;  /* Tamanho dos dados após o header em bytes */
    uint32_t crc32;         /* Checksum CRC32 do payload */
    uint64_t timestamp;     /* Unix timestamp da gravação */
    uint8_t  reserved[32];  /* Expansão futura */
} PersistHeader;
#pragma pack(pop)

/* Cálculo de CRC32 (IEEE 802.3) */
uint32_t persist_crc32(const void *data, size_t length);

/* Salva dados com header de integridade e escrita atômica via arquivo temporário + rename */
bool persist_save_atomic(const char *path, uint32_t neuron_count, const void *payload, size_t payload_size);

/* Carrega dados validando magic, versão, contagem de neurônios e checksum CRC32.
 * Retorna true se válido e copia até max_size bytes. Se corrompido, retorna false sem crashar. */
bool persist_load_safe(const char *path, uint32_t expected_neurons, void *out_payload, size_t max_size, size_t *out_read_size);

#endif /* PET_CORE_PERSIST_H */
