#pragma once

#if defined(BOOMBOX_USING_MPV_STUB) || defined(BOOMBOX_USING_MPV_COMPAT_HEADER)

#include <cstdint>

extern "C" {

typedef struct mpv_handle mpv_handle;
typedef struct mpv_event mpv_event;
typedef struct mpv_event_log_message mpv_event_log_message;
typedef struct mpv_event_error mpv_event_error;
typedef struct mpv_event_end_file mpv_event_end_file;
typedef struct mpv_event_property mpv_event_property;

typedef enum {
    MPV_FORMAT_NONE = 0,
    MPV_FORMAT_STRING = 1,
    MPV_FORMAT_OSD_STRING = 2,
    MPV_FORMAT_FLAG = 3,
    MPV_FORMAT_INT64 = 4, 
    MPV_FORMAT_DOUBLE = 5,
    MPV_FORMAT_INT = 6,
    MPV_FORMAT_NODE = 7,
    MPV_FORMAT_BYTE_ARRAY = 8,
    MPV_FORMAT_NODE_ARRAY = 9,
    MPV_FORMAT_NODE_MAP = 10
} mpv_format;

typedef enum {
    MPV_EVENT_NONE = 0,
    MPV_EVENT_SHUTDOWN = 1,
    MPV_EVENT_LOG_MESSAGE = 2,
    MPV_EVENT_GET_PROPERTY_REPLY = 3,
    MPV_EVENT_SET_PROPERTY_REPLY = 4,
    MPV_EVENT_COMMAND_REPLY = 5,
    MPV_EVENT_START_FILE = 6,
    MPV_EVENT_END_FILE = 7,
    MPV_EVENT_FILE_LOADED = 8,
    MPV_EVENT_IDLE = 9,
    MPV_EVENT_TICK = 10,
    MPV_EVENT_CLIENT_MESSAGE = 11,
    MPV_EVENT_VIDEO_RECONFIG = 12,
    MPV_EVENT_AUDIO_RECONFIG = 13,
    MPV_EVENT_SEEK = 14,
    MPV_EVENT_PLAYBACK_RESTART = 15,
    MPV_EVENT_PROPERTY_CHANGE = 16,
    MPV_EVENT_QUEUE_OVERFLOW = 17,
    MPV_EVENT_HOOK = 18,
    MPV_EVENT_PAUSE = 19,
    MPV_EVENT_UNPAUSE = 20,
    MPV_EVENT_ERROR = 21
} mpv_event_id;

struct mpv_event_log_message {
    const char* prefix;
    const char* level;
    const char* text;
};

struct mpv_event_error {
    int error;
};

struct mpv_event_end_file {
    int reason;
    int error;
    const char* playlist_entry_filename;
};

#define MPV_END_FILE_REASON_EOF 0
#define MPV_END_FILE_REASON_ERROR 1
#define MPV_END_FILE_REASON_QUIT 2
#define MPV_END_FILE_REASON_REDIRECT 3

struct mpv_event_property {
    const char* name;
    int format;
    void* data;
};

struct mpv_event {
    mpv_event_id event_id;
    int error;
    unsigned int reply_userdata;
    void* data;
};

#define MPV_MAKE_VERSION(major, minor) (((major) << 16) | (minor))
#define MPV_CLIENT_API_VERSION MPV_MAKE_VERSION(2, 0)

mpv_handle *mpv_create(void);
int mpv_initialize(mpv_handle *ctx);
void mpv_terminate_destroy(mpv_handle *ctx);
void mpv_detach_destroy(mpv_handle *ctx);
void mpv_destroy(mpv_handle *ctx);
void mpv_wakeup(mpv_handle *ctx);
int mpv_command(mpv_handle *ctx, const char **args);
int mpv_set_option_string(mpv_handle *ctx, const char *name, const char *data);
int mpv_set_property_string(mpv_handle *ctx, const char *name, const char *data);
int mpv_set_property(mpv_handle *ctx, const char *name, int format, void *data);
int mpv_get_property(mpv_handle *ctx, const char *name, int format, void *data);
char *mpv_get_property_string(mpv_handle *ctx, const char *name);
void mpv_free(void *data);
const char *mpv_event_name(int event_id);
const char *mpv_error_string(int error);
mpv_event *mpv_wait_event(mpv_handle *ctx, double timeout);

} // extern "C"

#else

#include <mpv/client.h>

#endif
