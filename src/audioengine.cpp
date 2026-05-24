#include "audioengine.h"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstdio>

// Helper function to log messages
static void logMessage(const std::string& level, const std::string& message) {
    std::cerr << "[AudioEngine " << level << "] " << message << std::endl;
}

// Helper function to check if file exists
static bool fileExists(const std::string& filePath) {
    std::ifstream f(filePath);
    return f.good();
}

static double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

static double dbToLevel(double dbValue) {
    if (!std::isfinite(dbValue)) {
        return 0.0;
    }

    // Map a practical dBFS range to [0, 1] so UI movement remains visible.
    // RMS around -60dB should be near silent, peaks close to 0dB near full scale.
    const double normalized = (dbValue + 60.0) / 55.0;
    return clamp01(normalized);
}

static bool parseDoubleString(const char* text, double& value) {
    if (!text) {
        return false;
    }
    try {
        value = std::stod(std::string(text));
        return std::isfinite(value);
    } catch (...) {
        return false;
    }
}

static bool readStringProperty(mpv_handle* handle, const char* property, std::string& value) {
    char* asString = mpv_get_property_string(handle, property);
    if (!asString) {
        return false;
    }
    value = asString;
    mpv_free(asString);
    return true;
}

static bool readDoubleProperty(mpv_handle* handle, const char* property, double& value) {
    double numeric = 0.0;
    if (mpv_get_property(handle, property, MPV_FORMAT_DOUBLE, &numeric) >= 0 && std::isfinite(numeric)) {
        value = numeric;
        return true;
    }

    char* asString = mpv_get_property_string(handle, property);
    if (!asString) {
        return false;
    }

    const bool ok = parseDoubleString(asString, value);
    mpv_free(asString);
    return ok;
}

namespace {
constexpr int kMpvEventPause = 19;
constexpr int kMpvEventUnpause = 20;
constexpr int kMpvEventError = 21;

struct MpvEventErrorPayload {
    int error;
};
} // namespace

AudioEngine::AudioEngine()
    : mHandle(nullptr), mState(PlaybackState::Stopped) {
    initializeMpv();
}

AudioEngine::~AudioEngine() {
    cleanupMpv();
}

AudioEngine::AudioEngine(AudioEngine&& other) noexcept
    : mHandle(other.mHandle),
      mState(other.mState),
      mOnTrackFinished(std::move(other.mOnTrackFinished)),
      mOnError(std::move(other.mOnError)) {
    other.mHandle = nullptr;
}

AudioEngine& AudioEngine::operator=(AudioEngine&& other) noexcept {
    if (this != &other) {
        cleanupMpv();
        mHandle = other.mHandle;
        mState = other.mState;
        mOnTrackFinished = std::move(other.mOnTrackFinished);
        mOnError = std::move(other.mOnError);
        other.mHandle = nullptr;
    }
    return *this;
}

void AudioEngine::initializeMpv() {
    if (mHandle) {
        logMessage("WARN", "AudioEngine already initialized");
        return;
    }

    // Create MPV context
    mHandle = mpv_create();
    if (!mHandle) {
        logMessage("ERROR", "Failed to create mpv context");
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "Failed to create mpv context");
        }
        return;
    }

    // Set options
    mpv_set_option_string(mHandle, "audio-only", "yes");
    mpv_set_option_string(mHandle, "vo", "null");
    mpv_set_option_string(mHandle, "af", "@bbxstats:lavfi=[astats=metadata=1:reset=1:length=0.05]");

    // Initialize the context
    int result = mpv_initialize(mHandle);
    if (result < 0) {
        logMessage("ERROR", "Failed to initialize mpv: " + std::string(mpv_error_string(result)));
        mpv_terminate_destroy(mHandle);
        mHandle = nullptr;
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, 
                     "Failed to initialize mpv: " + std::string(mpv_error_string(result)));
        }
        return;
    }

    logMessage("INFO", "AudioEngine initialized successfully");
    mState = PlaybackState::Stopped;
}

void AudioEngine::cleanupMpv() {
    if (!mHandle) {
        return;
    }

    // Stop playback if playing
    if (mState == PlaybackState::Playing || mState == PlaybackState::Paused) {
        const char* cmd[] = {"stop", nullptr};
        mpv_command(mHandle, cmd);
    }

    // Wait a bit for pending commands
    mpv_wakeup(mHandle);

    // Terminate and destroy the handle
    mpv_terminate_destroy(mHandle);
    mHandle = nullptr;
    mState = PlaybackState::Stopped;

    logMessage("INFO", "AudioEngine cleaned up");
}

void AudioEngine::play(const std::string& filePath) {
    if (!mHandle) {
        logMessage("ERROR", "AudioEngine not initialized");
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "AudioEngine not initialized");
        }
        return;
    }

    try {
        // Check if file exists first
        if (!fileExists(filePath)) {
            logMessage("ERROR", "File not found: " + filePath);
            if (mOnError) {
                mOnError(ErrorCode::FileNotFound, "File not found: " + filePath);
            }
            return;
        }

        // Load the file
        const char* args[] = {"loadfile", filePath.c_str(), nullptr};
        int result = mpv_command(mHandle, args);

        if (result < 0) {
            std::string errorStr = mpv_error_string(result);
            logMessage("ERROR", "Failed to load file: " + filePath + " - " + errorStr);
            
            // Determine error type based on mpv error
            ErrorCode errorCode = mapMpvError(result);
            if (mOnError) {
                mOnError(errorCode, "Failed to load file: " + filePath);
            }
            return;
        }

        mState = PlaybackState::Playing;
        logMessage("INFO", "Playing: " + filePath);
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in play(): " + std::string(e.what()));
        if (mOnError) {
            mOnError(ErrorCode::UnknownError, std::string("Exception: ") + e.what());
        }
    } catch (...) {
        logMessage("ERROR", "Unknown exception in play()");
        if (mOnError) {
            mOnError(ErrorCode::UnknownError, "Unknown exception occurred");
        }
    }
}

void AudioEngine::pause() {
    try {
        if (!mHandle || mState != PlaybackState::Playing) {
            return;
        }

        const char* args[] = {"set", "pause", "yes", nullptr};
        int result = mpv_command(mHandle, args);

        if (result < 0) {
            logMessage("WARN", "Failed to pause: " + std::string(mpv_error_string(result)));
            if (mOnError) {
                mOnError(ErrorCode::PlaybackFailed, "Failed to pause playback");
            }
            return;
        }

        mState = PlaybackState::Paused;
        logMessage("INFO", "Playback paused");
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in pause(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in pause()");
    }
}

void AudioEngine::resume() {
    try {
        if (!mHandle || mState != PlaybackState::Paused) {
            return;
        }

        const char* args[] = {"set", "pause", "no", nullptr};
        int result = mpv_command(mHandle, args);

        if (result < 0) {
            logMessage("WARN", "Failed to resume: " + std::string(mpv_error_string(result)));
            if (mOnError) {
                mOnError(ErrorCode::PlaybackFailed, "Failed to resume playback");
            }
            return;
        }

        mState = PlaybackState::Playing;
        logMessage("INFO", "Playback resumed");
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in resume(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in resume()");
    }
}

void AudioEngine::stop() {
    if (!mHandle) {
        return;
    }

    const char* args[] = {"stop", nullptr};
    mpv_command(mHandle, args);

    mState = PlaybackState::Stopped;
    logMessage("INFO", "Playback stopped");
}

void AudioEngine::next() {
    if (!mHandle) {
        return;
    }

    logMessage("INFO", "Skipping to next track");

    // Trigger track finished callback to let app handle next track logic
    if (mOnTrackFinished) {
        mOnTrackFinished();
    }
}

void AudioEngine::previous() {
    if (!mHandle) {
        return;
    }

    logMessage("INFO", "Skipping to previous track");

    // For simplicity, restart the current track
    const char* args[] = {"seek", "0", "absolute", nullptr};
    mpv_command(mHandle, args);
}

void AudioEngine::seek(double positionSeconds) {
    try {
        if (!mHandle) {
            return;
        }

        std::string posStr = std::to_string(positionSeconds);
        const char* args[] = {"seek", posStr.c_str(), "absolute", nullptr};
        int result = mpv_command(mHandle, args);

        if (result < 0) {
            logMessage("WARN", "Seek failed: " + std::string(mpv_error_string(result)));
        } else {
            logMessage("INFO", "Seeked to: " + posStr + "s");
        }
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in seek(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in seek()");
    }
}

void AudioEngine::setVolume(int level) {
    if (!mHandle) {
        return;
    }

    // Clamp volume to 0-100
    int volume = level < 0 ? 0 : (level > 100 ? 100 : level);

    int volInt = volume;
    int result = mpv_set_property(mHandle, "volume", MPV_FORMAT_INT64, &volInt);

    if (result < 0) {
        logMessage("WARN", "Failed to set volume: " + std::string(mpv_error_string(result)));
    } else {
        logMessage("INFO", "Volume set to: " + std::to_string(volume));
    }
}

int AudioEngine::getVolume() const {
    if (!mHandle) {
        return 0;
    }

    int64_t volume = 0;
    int result = mpv_get_property(mHandle, "volume", MPV_FORMAT_INT64, &volume);

    if (result < 0) {
        logMessage("WARN", "Failed to get volume: " + std::string(mpv_error_string(result)));
        return 0;
    }

    return static_cast<int>(volume);
}

double AudioEngine::getCurrentPosition() const {
    if (!mHandle) {
        return 0.0;
    }

    double pos = 0.0;
    int result = mpv_get_property(mHandle, "time-pos", MPV_FORMAT_DOUBLE, &pos);

    if (result < 0) {
        // Not an error - property might not be available during certain states
        return 0.0;
    }

    return pos;
}

double AudioEngine::getDuration() const {
    if (!mHandle) {
        return 0.0;
    }

    double duration = 0.0;
    int result = mpv_get_property(mHandle, "duration", MPV_FORMAT_DOUBLE, &duration);

    if (result < 0) {
        // Not an error - property might not be available during certain states
        return 0.0;
    }

    return duration;
}

double AudioEngine::getReactiveLevel() const {
    if (!mHandle || mState != PlaybackState::Playing) {
        return 0.0;
    }

    // mpv exposes filter metadata at af-metadata/<filter-label>/...
    // For robustness, we try our explicit label first and then common implicit
    // lavfi labels used by mpv when no explicit label is available.
    const char* labels[] = {
        "bbxstats",
        "lavfi.00",
        "lavfi.0",
        "lavfi.01",
        "lavfi.1"
    };

    double bestLevel = 0.0;
    char propertyName[256] = {};
    for (const char* label : labels) {
        int64_t metadataCount = 0;
        std::snprintf(propertyName, sizeof(propertyName), "af-metadata/%s/list/count", label);
        if (mpv_get_property(mHandle, propertyName, MPV_FORMAT_INT64, &metadataCount) >= 0 && metadataCount > 0) {
            for (int64_t i = 0; i < metadataCount; ++i) {
                char keyProperty[256] = {};
                std::snprintf(keyProperty, sizeof(keyProperty), "af-metadata/%s/list/%lld/key", label, static_cast<long long>(i));
                std::string key;
                if (!readStringProperty(mHandle, keyProperty, key)) {
                    continue;
                }

                if (key.find("RMS_level") == std::string::npos && key.find("Peak_level") == std::string::npos) {
                    continue;
                }

                char valueProperty[256] = {};
                std::snprintf(valueProperty, sizeof(valueProperty), "af-metadata/%s/list/%lld/value", label, static_cast<long long>(i));
                double dbValue = 0.0;
                if (readDoubleProperty(mHandle, valueProperty, dbValue)) {
                    bestLevel = std::max(bestLevel, dbToLevel(dbValue));
                }
            }
        }
    }

    return clamp01(bestLevel);
}

bool AudioEngine::isPlaying() const {
    return mState == PlaybackState::Playing;
}

bool AudioEngine::isInitialized() const {
    return mHandle != nullptr;
}

AudioEngine::PlaybackState AudioEngine::getPlaybackState() const {
    return mState;
}

void AudioEngine::setOnTrackFinished(TrackFinishedCallback callback) {
    mOnTrackFinished = callback;
}

void AudioEngine::setOnError(ErrorCallback callback) {
    mOnError = callback;
}

void AudioEngine::processEvents() {
    if (!mHandle) {
        return;
    }

    try {
        // Process all pending events from mpv
        while (true) {
            mpv_event* event = mpv_wait_event(mHandle, 0);
            if (!event) {
                logMessage("WARN", "mpv_wait_event returned null event");
                break;
            }

            if (event->event_id == MPV_EVENT_NONE) {
                break;
            }

            handleEvent(event);
        }
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in processEvents(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in processEvents()");
    }
}

void AudioEngine::handleEvent(const mpv_event* event) {
    if (!event) {
        return;
    }

    try {
        const int eventId = static_cast<int>(event->event_id);
        switch (eventId) {
            case MPV_EVENT_FILE_LOADED:
                logMessage("INFO", "File loaded successfully");
                if (mState == PlaybackState::Stopped) {
                    mState = PlaybackState::Playing;
                }
                break;

            case MPV_EVENT_END_FILE: {
                mpv_event_end_file* eof = static_cast<mpv_event_end_file*>(event->data);
                if (!eof) {
                    logMessage("WARN", "END_FILE event had no payload");
                    break;
                }

                if (eof->reason == MPV_END_FILE_REASON_ERROR) {
                    // Handle playback error on end file
                    logMessage("ERROR", "Track ended with error: " + std::string(mpv_error_string(eof->error)));
                    mState = PlaybackState::Stopped;
                    if (mOnError) {
                        ErrorCode errorCode = mapMpvError(eof->error);
                        mOnError(errorCode, "Playback error: track ended with error");
                    }
                } else if (eof->reason == MPV_END_FILE_REASON_EOF) {
                    logMessage("INFO", "Track finished");
                    mState = PlaybackState::Stopped;
                    if (mOnTrackFinished) {
                        mOnTrackFinished();
                    }
                } else {
                    // Non-EOF end reasons happen during manual track switches and should
                    // not trigger automatic random-next behavior.
                    logMessage("DEBUG", "Track ended for non-EOF reason; skipping auto-next");
                }
                break;
            }

            case MPV_EVENT_PLAYBACK_RESTART:
                logMessage("INFO", "Playback restarted");
                mState = PlaybackState::Playing;
                break;

            case MPV_EVENT_SEEK: {
                logMessage("INFO", "Seek event");
                break;
            }

            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property* prop = static_cast<mpv_event_property*>(event->data);
                if (!prop) {
                    logMessage("WARN", "Property change event had no payload");
                    break;
                }

                const char* propertyName = prop->name ? prop->name : "<unknown>";
                logMessage("DEBUG", "Property changed: " + std::string(propertyName));
                break;
            }

            case MPV_EVENT_SHUTDOWN:
                logMessage("INFO", "MPV shutting down");
                mState = PlaybackState::Stopped;
                break;

            default:
                if (eventId == kMpvEventPause) {
                    logMessage("INFO", "Paused by event");
                    mState = PlaybackState::Paused;
                    break;
                }

                if (eventId == kMpvEventUnpause) {
                    logMessage("INFO", "Unpaused by event");
                    mState = PlaybackState::Playing;
                    break;
                }

                if (eventId == kMpvEventError) {
                    auto* error = static_cast<MpvEventErrorPayload*>(event->data);
                    if (!error) {
                        logMessage("WARN", "MPV Error event had no payload; ignoring");
                        break;
                    }

                    logMessage("ERROR", "MPV Error: " + std::string(mpv_error_string(error->error)));
                    mState = PlaybackState::Stopped;
                    if (mOnError) {
                        mOnError(mapMpvError(error->error),
                                "MPV Error: " + std::string(mpv_error_string(error->error)));
                    }
                    break;
                }

                // Ignore other events.
                break;
        }
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in handleEvent(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in handleEvent()");
    }
}

AudioEngine::ErrorCode AudioEngine::mapMpvError(int mpvError) {
    // Map mpv error codes to our ErrorCode enum
    // MPV_ERROR_INVALID_PARAMETER = -1
    // MPV_ERROR_OPTION_NOT_FOUND = -2
    // MPV_ERROR_OPTION_FORMAT = -3
    // MPV_ERROR_OPTION_ERROR = -4
    // MPV_ERROR_PROPERTY_NOT_FOUND = -5
    // MPV_ERROR_PROPERTY_FORMAT = -6
    // MPV_ERROR_PROPERTY_UNAVAILABLE = -7
    // MPV_ERROR_PROPERTY_ERROR = -8
    // MPV_ERROR_COMMAND = -9
    // MPV_ERROR_LOADING_FAILED = -10
    // MPV_ERROR_AO_INIT_FAILED = -11
    // MPV_ERROR_VO_INIT_FAILED = -12
    // MPV_ERROR_NOTHING_TO_PLAY = -13
    // MPV_ERROR_UNKNOWN_FORMAT = -14
    // MPV_ERROR_UNSUPPORTED_PROTOCOL = -15
    // MPV_ERROR_NOT_IMPLEMENTED = -16
    switch (mpvError) {
        case 0:  // MPV_ERROR_SUCCESS
            return ErrorCode::NoError;
        case -10:  // MPV_ERROR_LOADING_FAILED
            return ErrorCode::FileNotFound;
        case -14:  // MPV_ERROR_UNKNOWN_FORMAT
            return ErrorCode::UnsupportedCodec;
        case -11:  // MPV_ERROR_AO_INIT_FAILED
        case -12:  // MPV_ERROR_VO_INIT_FAILED
            return ErrorCode::DeviceError;
        case 1:  // MPV_ERROR_GENERIC
        default:
            return ErrorCode::PlaybackFailed;
    }
}

int AudioEngine::sendCommand(const char** args) {
    if (!mHandle || !args) {
        return -1;
    }

    return mpv_command(mHandle, args);
}

void* AudioEngine::getProperty(const char* property, const char* /* type */) {
    if (!mHandle || !property) {
        return nullptr;
    }

    void* data = nullptr;
    mpv_get_property(mHandle, property, MPV_FORMAT_DOUBLE, &data);
    return data;
}

void AudioEngine::setProperty(const char* property, const char* /* type */, void* value) {
    if (!mHandle || !property || !value) {
        return;
    }

    mpv_set_property(mHandle, property, MPV_FORMAT_DOUBLE, value);
}
