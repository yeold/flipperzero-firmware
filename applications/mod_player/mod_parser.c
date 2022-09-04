#include "mod_parser.h"

#define TAG "ModParser"

const char* format_text(FormatTag tag) {
    switch(tag) {
    case FormatTagPCM:
        return "PCM";
    case FormatTagIEEE_FLOAT:
        return "IEEE FLOAT";
    default:
        return "Unknown";
    }
};

struct ModParser {
    ModHeaderChunk header;
    ModFormatChunk format;
    ModDataChunk data;
    size_t mod_data_start;
    size_t mod_data_end;
};

ModParser* mod_parser_alloc() {
    return malloc(sizeof(ModParser));
}

void mod_parser_free(ModParser* parser) {
    free(parser);
}

bool mod_parser_parse(ModParser* parser, Stream* stream) {
    stream_read(stream, (uint8_t*)&parser->header, sizeof(ModHeaderChunk));
    stream_read(stream, (uint8_t*)&parser->format, sizeof(ModFormatChunk));
    stream_read(stream, (uint8_t*)&parser->data, sizeof(ModDataChunk));

    if(memcmp(parser->header.riff, "RIFF", 4) != 0 ||
       memcmp(parser->header.mode, "WAVE", 4) != 0) {
        FURI_LOG_E(TAG, "WAV: wrong header");
        return false;
    }

    if(memcmp(parser->format.fmt, "fmt ", 4) != 0) {
        FURI_LOG_E(TAG, "WAV: wrong format");
        return false;
    }

    if(parser->format.tag != FormatTagPCM || memcmp(parser->data.data, "data", 4) != 0) {
        FURI_LOG_E(
            TAG,
            "WAV: non-PCM format %u, next '%lu'",
            parser->format.tag,
            (uint32_t)parser->data.data);
        return false;
    }

    FURI_LOG_I(
        TAG,
        "Format tag: %s, ch: %u, smplrate: %lu, bps: %lu, bits: %u",
        format_text(parser->format.tag),
        parser->format.channels,
        parser->format.sample_rate,
        parser->format.byte_per_sec,
        parser->format.bits_per_sample);

    parser->mod_data_start = stream_tell(stream);
    parser->mod_data_end = parser->mod_data_start + parser->data.size;

    FURI_LOG_I(TAG, "data: %u - %u", parser->mod_data_start, parser->mod_data_end);

    return true;
}

size_t mod_parser_get_data_start(ModParser* parser) {
    return parser->mod_data_start;
}

size_t mod_parser_get_data_end(ModParser* parser) {
    return parser->mod_data_end;
}

size_t mod_parser_get_data_len(ModParser* parser) {
    return parser->mod_data_end - parser->mod_data_start;
}
