/* Stub implementation of libmpv for compilation */

#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "mpv_client_compat.h"
#include <cstdlib>
#include <cstring>

mpv_handle *mpv_create(void) {
    return nullptr;
}

int mpv_initialize(mpv_handle *ctx) {
    return -1;
}

void mpv_terminate_destroy(mpv_handle *ctx) {
}

void mpv_detach_destroy(mpv_handle *ctx) {
}

void mpv_destroy(mpv_handle *ctx) {
}

void mpv_wakeup(mpv_handle *ctx) {
}

int mpv_command(mpv_handle *ctx, const char **args) {
    return -1;
}

int mpv_set_option_string(mpv_handle *ctx, const char *name, const char *data) {
    return -1;
}

int mpv_set_property_string(mpv_handle *ctx, const char *name, const char *data) {
    return -1;
}

int mpv_set_property(mpv_handle *ctx, const char *name, int format, void *data) {
    return -1;
}

int mpv_get_property(mpv_handle *ctx, const char *name, int format, void *data) {
    return -1;
}

char *mpv_get_property_string(mpv_handle *ctx, const char *name) {
    return nullptr;
}

void mpv_free(void *data) {
}

const char *mpv_event_name(int event_id) {
    return "unknown";
}

const char *mpv_error_string(int error) {
    return "unknown error";
}

mpv_event *mpv_wait_event(mpv_handle *ctx, double timeout) {
    static mpv_event event = {MPV_EVENT_NONE, 0, 0, nullptr};
    return &event;
}
