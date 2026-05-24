#pragma once

#ifdef BOOMBOX_USING_MPV_STUB

#include <cstdint>

extern "C" {

typedef struct mpv_handle mpv_handle;

typedef struct mpv_event {
    int event_id;
    int error;
    uint64_t reply_userdata;
    void* data;
} mpv_event;

typedef struct mpv_event_end_file {
    int reason;
    int error;
    uint64_t playlist_entry_id;
    uint64_t playlist_insert_id;
    int playlist_insert_num_entries;
} mpv_event_end_file;

typedef struct mpv_event_property {
    const char* name;
    int format;
    void* data;
} mpv_event_property;

typedef struct mpv_event_error {
    int error;
    const char* prefix;
} mpv_event_error;

enum {
    MPV_FORMAT_NONE = 0,
    MPV_FORMAT_STRING = 1,
    MPV_FORMAT_OSD_STRING = 2,
    MPV_FORMAT_FLAG = 3,
    MPV_FORMAT_INT64 = 4,
    MPV_FORMAT_DOUBLE = 5
};

enum {
    MPV_EVENT_NONE = 0,
    MPV_EVENT_SHUTDOWN = 1,
    MPV_EVENT_LOG_MESSAGE = 2,
    MPV_EVENT_GET_PROPERTY_REPLY = 3,
    MPV_EVENT_SET_PROPERTY_REPLY = 4,
    MPV_EVENT_COMMAND_REPLY = 5,
    MPV_EVENT_START_FILE = 6,
    MPV_EVENT_END_FILE = 7,
    MPV_EVENT_FILE_LOADED = 8,
    MPV_EVENT_TRACKS_CHANGED = 9,
    MPV_EVENT_TRACK_SWITCHED = 10,
    MPV_EVENT_IDLE = 11,
    MPV_EVENT_PAUSE = 12,
    MPV_EVENT_UNPAUSE = 13,
    MPV_EVENT_TICK = 14,
    MPV_EVENT_SCRIPT_INPUT_DISPATCH = 15,
    MPV_EVENT_CLIENT_MESSAGE = 16,
    MPV_EVENT_VIDEO_RECONFIG = 17,
    MPV_EVENT_AUDIO_RECONFIG = 18,
    MPV_EVENT_METADATA_UPDATE = 19,
    MPV_EVENT_SEEK = 20,
    MPV_EVENT_PLAYBACK_RESTART = 21,
    MPV_EVENT_PROPERTY_CHANGE = 22,
    MPV_EVENT_CHAPTER_CHANGE = 23,
    MPV_EVENT_QUEUE_OVERFLOW = 24,
    MPV_EVENT_HOOK = 25,
    MPV_EVENT_ERROR = 26
};

enum {
    MPV_END_FILE_REASON_EOF = 0,
    MPV_END_FILE_REASON_STOP = 2,
    MPV_END_FILE_REASON_QUIT = 3,
    MPV_END_FILE_REASON_ERROR = 4,
    MPV_END_FILE_REASON_REDIRECT = 5
};

mpv_handle* mpv_create(void);
int mpv_initialize(mpv_handle* ctx);
void mpv_terminate_destroy(mpv_handle* ctx);
void mpv_detach_destroy(mpv_handle* ctx);
void mpv_destroy(mpv_handle* ctx);
void mpv_wakeup(mpv_handle* ctx);
int mpv_command(mpv_handle* ctx, const char** args);
int mpv_set_option_string(mpv_handle* ctx, const char* name, const char* data);
int mpv_set_property_string(mpv_handle* ctx, const char* name, const char* data);
int mpv_set_property(mpv_handle* ctx, const char* name, int format, void* data);
int mpv_get_property(mpv_handle* ctx, const char* name, int format, void* data);
char* mpv_get_property_string(mpv_handle* ctx, const char* name);
void mpv_free(void* data);
const char* mpv_event_name(int event_id);
const char* mpv_error_string(int error);
mpv_event* mpv_wait_event(mpv_handle* ctx, double timeout);

} // extern "C"

#else

#include <mpv/client.h>

#endif
