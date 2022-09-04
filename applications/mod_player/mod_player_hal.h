#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void mod_player_speaker_init();

void mod_player_speaker_start();

void mod_player_speaker_stop();

void mod_player_dma_init(uint32_t address, size_t size);

void mod_player_dma_start();

void mod_player_dma_stop();

#ifdef __cplusplus
}
#endif