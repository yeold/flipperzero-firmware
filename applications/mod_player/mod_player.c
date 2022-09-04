#include <furi.h>
#include <furi_hal.h>
#include <cli/cli.h>
#include <gui/gui.h>
#include <stm32wbxx_ll_dma.h>
#include <dialogs/dialogs.h>
#include <notification/notification_messages.h>
#include <gui/view_dispatcher.h>
#include <toolbox/stream/file_stream.h>
#include "mod_player_hal.h"
#include "mod_parser.h"
#include "mod_player_view.h"
#include <math.h>
#include "hxcmod.h"

#define TAG "ModPlayer"

#define FURI_HAL_SPEAKER_TIMER TIM16
#define FURI_HAL_SPEAKER_CHANNEL LL_TIM_CHANNEL_CH1
#define DMA_INSTANCE DMA1, LL_DMA_CHANNEL_1

#define MODPLAYER_FOLDER "/ext/mod_player"
#define PLAYER_SAMPLE_RATE 9600
#define SAMPLE_BUFFER_SIZE 8192


msample dmasoundbuffer[SAMPLE_BUFFER_SIZE] __attribute__ ((aligned (4)));

// MOD data
extern const unsigned char mod_data[39424];

modcontext mcontext;

#if 0
static bool open_mod_stream(Stream* stream) {
    DialogsApp* dialogs = furi_record_open("dialogs");
    bool result = false;
    string_t path;
    string_init(path);
    string_set_str(path, MODPLAYER_FOLDER);
    bool ret = dialog_file_browser_show(dialogs, path, path, ".mod", true, &I_music_10px, false);

    furi_record_close("dialogs");
    if(ret) {
        if(!file_stream_open(stream, string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
            FURI_LOG_E(TAG, "Cannot open file \"%s\"", string_get_cstr(path));
        } else {
            result = true;
        }
    }
    string_clear(path);
    return result;
}

#endif

typedef enum {
    ModPlayerEventHalfTransfer,
    ModPlayerEventFullTransfer,
    ModPlayerEventCtrlVolUp,
    ModPlayerEventCtrlVolDn,
    ModPlayerEventCtrlMoveL,
    ModPlayerEventCtrlMoveR,
    ModPlayerEventCtrlOk,
    ModPlayerEventCtrlBack,
} ModPlayerEventType;

typedef struct {
    ModPlayerEventType type;
} ModPlayerEvent;

#if 0
static void mod_player_dma_isr(void* ctx) {


    hxcmod_fillbuffer((modcontext*)&ctx, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, NULL );

}
#endif

typedef struct {
    Storage* storage;
    Stream* stream;
    ModParser* parser;
    uint16_t* sample_buffer;
    uint8_t* tmp_buffer;

    size_t samples_count_half;
    size_t samples_count;

    FuriMessageQueue* queue;

    float volume;
    bool play;

    ModPlayerView* view;
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notification;
} ModPlayerApp;

#if 0
static ModPlayerApp* app_alloc() {
    ModPlayerApp* app = malloc(sizeof(ModPlayerApp));
    app->samples_count_half = 1024 * 4;
    app->samples_count = app->samples_count_half * 2;
    app->storage = furi_record_open(RECORD_STORAGE);
    app->stream = file_stream_alloc(app->storage);
    app->parser = mod_parser_alloc();
    app->sample_buffer = malloc(sizeof(uint16_t) * app->samples_count);
    app->tmp_buffer = malloc(sizeof(uint8_t) * app->samples_count);
    app->queue = furi_message_queue_alloc(10, sizeof(ModPlayerEvent));

    app->volume = 10.0f;
    app->play = true;

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->view = mod_player_view_alloc();

    view_dispatcher_add_view(app->view_dispatcher, 0, mod_player_view_get_view(app->view));
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    app->notification = furi_record_open("notification");
    notification_message(app->notification, &sequence_display_backlight_enforce_on);
    hxcmod_init(&mcontext);
    hxcmod_setcfg(&mcontext, PLAYER_SAMPLE_RATE, 0, 0);

    return app;
}

static void app_free(ModPlayerApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, 0);
    view_dispatcher_free(app->view_dispatcher);
    mod_player_view_free(app->view);
    furi_record_close(RECORD_GUI);

    furi_message_queue_free(app->queue);
    free(app->tmp_buffer);
    free(app->sample_buffer);
    mod_parser_free(app->parser);
    stream_free(app->stream);
    furi_record_close(RECORD_STORAGE);

    hxcmod_unload(&mcontext);

    notification_message(app->notification, &sequence_display_backlight_enforce_auto);
    furi_record_close("notification");
    free(app);
}

// TODO: that works only with 8-bit 2ch audio
static bool fill_data(ModPlayerApp* app, size_t index) {
    uint16_t* sample_buffer_start = &app->sample_buffer[index];
    size_t count = stream_read(app->stream, app->tmp_buffer, app->samples_count);

    for(size_t i = count; i < app->samples_count; i++) {
        app->tmp_buffer[i] = 0;
    }

    for(size_t i = 0; i < app->samples_count; i += 2) {
        float data = app->tmp_buffer[i];
        data -= UINT8_MAX / 2; // to signed
        data /= UINT8_MAX / 2; // scale -1..1

        data *= app->volume; // volume
        data = tanhf(data); // hyperbolic tangent limiter

        data *= UINT8_MAX / 2; // scale -128..127
        data += UINT8_MAX / 2; // to unsigned

        if(data < 0) {
            data = 0;
        }

        if(data > 255) {
            data = 255;
        }

        sample_buffer_start[i / 2] = data;
    }

    mod_player_view_set_data(app->view, sample_buffer_start, app->samples_count_half);

    return count != app->samples_count;
}

static void ctrl_callback(ModPlayerCtrl ctrl, void* ctx) {
    FuriMessageQueue* event_queue = ctx;
    ModPlayerEvent event;

    switch(ctrl) {
    case ModPlayerCtrlVolUp:
        event.type = ModPlayerEventCtrlVolUp;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    case ModPlayerCtrlVolDn:
        event.type = ModPlayerEventCtrlVolDn;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    case ModPlayerCtrlMoveL:
        event.type = ModPlayerEventCtrlMoveL;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    case ModPlayerCtrlMoveR:
        event.type = ModPlayerEventCtrlMoveR;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    case ModPlayerCtrlOk:
        event.type = ModPlayerEventCtrlOk;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    case ModPlayerCtrlBack:
        event.type = ModPlayerEventCtrlBack;
        furi_message_queue_put(event_queue, &event, 0);
        break;
    default:
        break;
    }
}

static void app_run(ModPlayerApp* app) {
    if(!open_mod_stream(app->stream)) return;
    if(!mod_parser_parse(app->parser, app->stream)) return;

    mod_player_view_set_volume(app->view, app->volume);
    mod_player_view_set_start(app->view, mod_parser_get_data_start(app->parser));
    mod_player_view_set_current(app->view, stream_tell(app->stream));
    mod_player_view_set_end(app->view, mod_parser_get_data_end(app->parser));
    mod_player_view_set_play(app->view, app->play);

    mod_player_view_set_context(app->view, app->queue);
    mod_player_view_set_ctrl_callback(app->view, ctrl_callback);

    bool eof = fill_data(app, 0);
    eof = fill_data(app, app->samples_count_half);

    mod_player_speaker_init();
    mod_player_dma_init((uint32_t)app->sample_buffer, app->samples_count);

    furi_hal_interrupt_set_isr(FuriHalInterruptIdDma1Ch1, mod_player_dma_isr, app->queue);

    mod_player_dma_start();
    mod_player_speaker_start();

    ModPlayerEvent event;

    while(1) {
        if(furi_message_queue_get(app->queue, &event, FuriWaitForever) == FuriStatusOk) {
            if(event.type == ModPlayerEventHalfTransfer) {
                eof = fill_data(app, 0);
                mod_player_view_set_current(app->view, stream_tell(app->stream));
                if(eof) {
                    stream_seek(
                        app->stream,
                        mod_parser_get_data_start(app->parser),
                        StreamOffsetFromStart);
                }

            } else if(event.type == ModPlayerEventFullTransfer) {
                eof = fill_data(app, app->samples_count_half);
                mod_player_view_set_current(app->view, stream_tell(app->stream));
                if(eof) {
                    stream_seek(
                        app->stream,
                        mod_parser_get_data_start(app->parser),
                        StreamOffsetFromStart);
                }
            } else if(event.type == ModPlayerEventCtrlVolUp) {
                if(app->volume < 9.9) app->volume += 0.4;
                mod_player_view_set_volume(app->view, app->volume);
            } else if(event.type == ModPlayerEventCtrlVolDn) {
                if(app->volume > 0.01) app->volume -= 0.4;
                mod_player_view_set_volume(app->view, app->volume);
            } else if(event.type == ModPlayerEventCtrlMoveL) {
                int32_t seek = stream_tell(app->stream) - mod_parser_get_data_start(app->parser);
                seek = MIN(seek, (int32_t)(mod_parser_get_data_len(app->parser) / (size_t)100));
                stream_seek(app->stream, -seek, StreamOffsetFromCurrent);
                mod_player_view_set_current(app->view, stream_tell(app->stream));
            } else if(event.type == ModPlayerEventCtrlMoveR) {
                int32_t seek = mod_parser_get_data_end(app->parser) - stream_tell(app->stream);
                seek = MIN(seek, (int32_t)(mod_parser_get_data_len(app->parser) / (size_t)100));
                stream_seek(app->stream, seek, StreamOffsetFromCurrent);
                mod_player_view_set_current(app->view, stream_tell(app->stream));
            } else if(event.type == ModPlayerEventCtrlOk) {
                app->play = !app->play;
                mod_player_view_set_play(app->view, app->play);

                if(!app->play) {
                    mod_player_speaker_stop();
                } else {
                    mod_player_speaker_start();
                }
            } else if(event.type == ModPlayerEventCtrlBack) {
                break;
            }
        }
    }

    mod_player_speaker_stop();
    mod_player_dma_stop();

    furi_hal_interrupt_set_isr(FuriHalInterruptIdDma1Ch1, NULL, NULL);
}

#endif

int32_t mod_player_app() {
    int i,oldpatternpos;

    hxcmod_init(&mcontext);
    hxcmod_setcfg(&mcontext, PLAYER_SAMPLE_RATE, 0, 0);
    hxcmod_load(&mcontext, (void *)&mod_data, sizeof(mod_data));

    // Clear Buffer
    for(i = 0; i < SAMPLE_BUFFER_SIZE; i++)
    {
        dmasoundbuffer[i] = 2048;
    }
    
    hxcmod_fillbuffer( &mcontext, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, NULL );
    
    mod_player_speaker_init();
    mod_player_dma_init((uint32_t)&dmasoundbuffer, sizeof(dmasoundbuffer));

    #if 0
    furi_hal_interrupt_set_isr(FuriHalInterruptIdDma1Ch1, mod_player_dma_isr, &mcontext);
    #endif
    mod_player_dma_start();
    mod_player_speaker_start();

    i = 0;
    oldpatternpos = -1;
    while(1) {
        i++;
        //Print the pattern position if needed.
        if(mcontext.patternpos != oldpatternpos) {
            oldpatternpos = mcontext.patternpos;
            hxcmod_fillbuffer((modcontext*)&mcontext, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, NULL );
        }

        if (i == 600000)
        {
            break;
        }
    }

    mod_player_speaker_stop();
    mod_player_dma_stop();
    hxcmod_unload(&mcontext);

    #if 0
    UNUSED(p);
    ModPlayerApp* app = app_alloc();

    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage_simply_mkdir(storage, MODPLAYER_FOLDER)) {
        FURI_LOG_E(TAG, "Could not create folder %s", MODPLAYER_FOLDER);
    }
    furi_record_close(RECORD_STORAGE);

    app_run(app);
    app_free(app);

    #endif
    return 0;
}
