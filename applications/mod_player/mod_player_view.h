#pragma once
#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ModPlayerView ModPlayerView;

typedef enum {
    ModPlayerCtrlVolUp,
    ModPlayerCtrlVolDn,
    ModPlayerCtrlMoveL,
    ModPlayerCtrlMoveR,
    ModPlayerCtrlOk,
    ModPlayerCtrlBack,
} ModPlayerCtrl;

typedef void (*ModPlayerCtrlCallback)(ModPlayerCtrl ctrl, void* context);

ModPlayerView* mod_player_view_alloc();

void mod_player_view_free(ModPlayerView* mod_view);

View* mod_player_view_get_view(ModPlayerView* mod_view);

void mod_player_view_set_volume(ModPlayerView* mod_view, float volume);

void mod_player_view_set_start(ModPlayerView* mod_view, size_t start);

void mod_player_view_set_end(ModPlayerView* mod_view, size_t end);

void mod_player_view_set_current(ModPlayerView* mod_view, size_t current);

void mod_player_view_set_play(ModPlayerView* mod_view, bool play);

void mod_player_view_set_data(ModPlayerView* mod_view, uint16_t* data, size_t data_count);

void mod_player_view_set_ctrl_callback(ModPlayerView* mod_view, ModPlayerCtrlCallback callback);

void mod_player_view_set_context(ModPlayerView* mod_view, void* context);

#ifdef __cplusplus
}
#endif