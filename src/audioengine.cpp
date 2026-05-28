#include "audioengine.h"

#include "gstaudioengine.h"
#include <gst/gst.h>

#include <memory>

std::unique_ptr<AudioEngine> createAudioEngine() {
    return std::make_unique<GstAudioEngine>();
}

void initializeAudioBackendRuntime() {
    gst_init(nullptr, nullptr);
}

const char* selectedAudioBackendName() {
    return "gstreamer";
}
