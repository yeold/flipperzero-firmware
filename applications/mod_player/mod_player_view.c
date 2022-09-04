#include "mod_player_view.h"

#define DATA_COUNT 116

struct ModPlayerView {
    View* view;
    ModPlayerCtrlCallback callback;
    void* context;
};

typedef struct {
    bool play;
    float volume;
    size_t start;
    size_t end;
    size_t current;
    uint8_t data[DATA_COUNT];
} ModPlayerViewModel;

float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min + 1) + out_min;
}

static void mod_player_view_draw_callback(Canvas* canvas, void* _model) {
    ModPlayerViewModel* model = _model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    uint8_t x_pos = 0;
    uint8_t y_pos = 0;

    // volume
    x_pos = 124;
    y_pos = 0;
    const float volume = (64 / 10.0f) * model->volume;
    canvas_draw_frame(canvas, x_pos, y_pos, 4, 64);
    canvas_draw_box(canvas, x_pos, y_pos + (64 - volume), 4, volume);

    // play / pause
    x_pos = 58;
    y_pos = 55;
    if(!model->play) {
        canvas_draw_line(canvas, x_pos, y_pos, x_pos + 8, y_pos + 4);
        canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos + 8, y_pos + 4);
        canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos, y_pos);
    } else {
        canvas_draw_box(canvas, x_pos, y_pos, 3, 9);
        canvas_draw_box(canvas, x_pos + 4, y_pos, 3, 9);
    }

    x_pos = 78;
    y_pos = 55;
    canvas_draw_line(canvas, x_pos, y_pos, x_pos + 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos + 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos, y_pos);

    x_pos = 82;
    y_pos = 55;
    canvas_draw_line(canvas, x_pos, y_pos, x_pos + 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos + 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos, y_pos);

    x_pos = 40;
    y_pos = 55;
    canvas_draw_line(canvas, x_pos, y_pos, x_pos - 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos - 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos, y_pos);

    x_pos = 44;
    y_pos = 55;
    canvas_draw_line(canvas, x_pos, y_pos, x_pos - 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos - 4, y_pos + 4);
    canvas_draw_line(canvas, x_pos, y_pos + 8, x_pos, y_pos);

    // len
    x_pos = 4;
    y_pos = 47;
    const uint8_t play_len = 116;
    uint8_t play_pos = map(model->current, model->start, model->end, 0, play_len - 4);

    canvas_draw_frame(canvas, x_pos, y_pos, play_len, 4);
    canvas_draw_box(canvas, x_pos + play_pos, y_pos - 2, 4, 8);
    canvas_draw_box(canvas, x_pos, y_pos, play_pos, 4);

    // osc
    x_pos = 4;
    y_pos = 0;
    for(size_t i = 1; i < DATA_COUNT; i++) {
        canvas_draw_line(canvas, x_pos + i - 1, model->data[i - 1], x_pos + i, model->data[i]);
    }
}

static bool mod_player_view_input_callback(InputEvent* event, void* context) {
    ModPlayerView* mod_player_view = context;
    bool consumed = false;

    if(mod_player_view->callback) {
        if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
            if(event->key == InputKeyUp) {
                mod_player_view->callback(ModPlayerCtrlVolUp, mod_player_view->context);
                consumed = true;
            } else if(event->key == InputKeyDown) {
                mod_player_view->callback(ModPlayerCtrlVolDn, mod_player_view->context);
                consumed = true;
            } else if(event->key == InputKeyLeft) {
                mod_player_view->callback(ModPlayerCtrlMoveL, mod_player_view->context);
                consumed = true;
            } else if(event->key == InputKeyRight) {
                mod_player_view->callback(ModPlayerCtrlMoveR, mod_player_view->context);
                consumed = true;
            } else if(event->key == InputKeyOk) {
                mod_player_view->callback(ModPlayerCtrlOk, mod_player_view->context);
                consumed = true;
            } else if(event->key == InputKeyBack) {
                mod_player_view->callback(ModPlayerCtrlBack, mod_player_view->context);
                consumed = true;
            }
        }
    }

    return consumed;
}

ModPlayerView* mod_player_view_alloc() {
    ModPlayerView* mod_view = malloc(sizeof(ModPlayerView));
    mod_view->view = view_alloc();
    view_set_context(mod_view->view, mod_view);
    view_allocate_model(mod_view->view, ViewModelTypeLocking, sizeof(ModPlayerViewModel));
    view_set_draw_callback(mod_view->view, mod_player_view_draw_callback);
    view_set_input_callback(mod_view->view, mod_player_view_input_callback);

    return mod_view;
}

void mod_player_view_free(ModPlayerView* mod_view) {
    furi_assert(mod_view);
    view_free(mod_view->view);
    free(mod_view);
}

View* mod_player_view_get_view(ModPlayerView* mod_view) {
    furi_assert(mod_view);
    return mod_view->view;
}

void mod_player_view_set_volume(ModPlayerView* mod_view, float volume) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            model->volume = volume;
            return true;
        });
}

void mod_player_view_set_start(ModPlayerView* mod_view, size_t start) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            model->start = start;
            return true;
        });
}

void mod_player_view_set_end(ModPlayerView* mod_view, size_t end) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            model->end = end;
            return true;
        });
}

void mod_player_view_set_current(ModPlayerView* mod_view, size_t current) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            model->current = current;
            return true;
        });
}

void mod_player_view_set_play(ModPlayerView* mod_view, bool play) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            model->play = play;
            return true;
        });
}

void mod_player_view_set_data(ModPlayerView* mod_view, uint16_t* data, size_t data_count) {
    furi_assert(mod_view);
    with_view_model(
        mod_view->view, (ModPlayerViewModel * model) {
            size_t inc = (data_count / DATA_COUNT) - 1;

            for(size_t i = 0; i < DATA_COUNT; i++) {
                model->data[i] = *data / 6;
                if(model->data[i] > 42) model->data[i] = 42;
                data += inc;
            }
            return true;
        });
}

void mod_player_view_set_ctrl_callback(ModPlayerView* mod_view, ModPlayerCtrlCallback callback) {
    furi_assert(mod_view);
    mod_view->callback = callback;
}

void mod_player_view_set_context(ModPlayerView* mod_view, void* context) {
    furi_assert(mod_view);
    mod_view->context = context;
}