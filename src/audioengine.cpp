#include "audioengine.h"

#if defined(BOOMBOX_AUDIO_BACKEND_GSTREAMER)
#include "gstaudioengine.h"
#include <gst/gst.h>
#elif defined(BOOMBOX_AUDIO_BACKEND_MPV)
#include "mpvaudioengine.h"
#elif defined(BOOMBOX_AUDIO_BACKEND_STUB)
#include "stubaudioengine.h"
#else
#error "No audio backend selected"
#endif

#include <memory>

std::unique_ptr<AudioEngine> createAudioEngine() {
#if defined(BOOMBOX_AUDIO_BACKEND_GSTREAMER)
    return std::make_unique<GstAudioEngine>();
#elif defined(BOOMBOX_AUDIO_BACKEND_MPV)
    return std::make_unique<MpvAudioEngine>();
#elif defined(BOOMBOX_AUDIO_BACKEND_STUB)
    return std::make_unique<StubAudioEngine>();
#endif
}

void initializeAudioBackendRuntime() {
#if defined(BOOMBOX_AUDIO_BACKEND_GSTREAMER)
    gst_init(nullptr, nullptr);
#endif
}

const char* selectedAudioBackendName() {
#if defined(BOOMBOX_AUDIO_BACKEND_GSTREAMER)
    return "gstreamer";
#elif defined(BOOMBOX_AUDIO_BACKEND_MPV)
    return "mpv";
#elif defined(BOOMBOX_AUDIO_BACKEND_STUB)
    return "stub";
#else
    return "unknown";
#endif
}
