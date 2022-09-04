#pragma once
#include <toolbox/stream/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FormatTagPCM = 0x0001,
    FormatTagIEEE_FLOAT = 0x0003,
} FormatTag;

typedef struct {
    uint8_t riff[4];
    uint32_t size;
    uint8_t mode[4];
} ModHeaderChunk;

typedef struct {
    uint8_t fmt[4];
    uint32_t size;
    uint16_t tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
} ModFormatChunk;

typedef struct {
    uint8_t data[4];
    uint32_t size;
} ModDataChunk;

typedef struct ModParser ModParser;

ModParser* mod_parser_alloc();

void mod_parser_free(ModParser* parser);

bool mod_parser_parse(ModParser* parser, Stream* stream);

size_t mod_parser_get_data_start(ModParser* parser);

size_t mod_parser_get_data_end(ModParser* parser);

size_t mod_parser_get_data_len(ModParser* parser);

#ifdef __cplusplus
}
#endif