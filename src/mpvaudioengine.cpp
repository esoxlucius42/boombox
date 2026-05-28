#include "mpvaudioengine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void logMessage(const std::string& level, const std::string& message) {
    std::cerr << "[AudioEngine " << level << "] " << message << std::endl;
}

bool fileExists(const std::string& filePath) {
    std::ifstream f(filePath);
    return f.good();
}

bool parseDoubleString(const char* text, double& value) {
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

bool readStringProperty(mpv_handle* handle, const char* property, std::string& value) {
    char* asString = mpv_get_property_string(handle, property);
    if (!asString) {
        return false;
    }
    value = asString;
    mpv_free(asString);
    return true;
}

bool readDoubleProperty(mpv_handle* handle, const char* property, double& value) {
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

bool readFlagProperty(mpv_handle* handle, const char* property, bool& value) {
    int flag = 0;
    if (mpv_get_property(handle, property, MPV_FORMAT_FLAG, &flag) >= 0) {
        value = flag != 0;
        return true;
    }

    int64_t intValue = 0;
    if (mpv_get_property(handle, property, MPV_FORMAT_INT64, &intValue) >= 0) {
        value = intValue != 0;
        return true;
    }

    std::string text;
    if (!readStringProperty(handle, property, text)) {
        return false;
    }

    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (text == "yes" || text == "true" || text == "1") {
        value = true;
        return true;
    }
    if (text == "no" || text == "false" || text == "0") {
        value = false;
        return true;
    }

    return false;
}

std::string boolToText(bool value) {
    return value ? "yes" : "no";
}

void setOptionWithLog(mpv_handle* handle, const char* option, const char* value) {
    const int result = mpv_set_option_string(handle, option, value);
    if (result < 0) {
        logMessage("WARN",
                   std::string("Failed to set mpv option '") + option + "' to '" + value +
                       "': " + mpv_error_string(result));
    }
}

constexpr int kMpvEventPause = 19;
constexpr int kMpvEventUnpause = 20;
constexpr int kMpvEventError = 21;
constexpr auto kDiagnosticSampleInterval = std::chrono::milliseconds(1000);
constexpr auto kPlaybackStallThreshold = std::chrono::milliseconds(2000);

struct MpvEventErrorPayload {
    int error;
};
} // namespace

MpvAudioEngine::MpvAudioEngine() {
    initializeMpv();
}

MpvAudioEngine::~MpvAudioEngine() {
    cleanupMpv();
}

void MpvAudioEngine::initializeMpv() {
    if (mHandle) {
        logMessage("WARN", "AudioEngine already initialized");
        return;
    }

    mHandle = mpv_create();
    if (!mHandle) {
        logMessage("ERROR", "Failed to create mpv context");
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "Failed to create mpv context");
        }
        return;
    }

    setOptionWithLog(mHandle, "audio-only", "yes");
    setOptionWithLog(mHandle, "vo", "null");
    setOptionWithLog(mHandle, "cache", "yes");
    setOptionWithLog(mHandle, "demuxer-max-bytes", "67108864");
    setOptionWithLog(mHandle, "demuxer-max-back-bytes", "16777216");
    setOptionWithLog(mHandle, "audio-buffer", "0.40");

    const int result = mpv_initialize(mHandle);
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

void MpvAudioEngine::cleanupMpv() {
    if (!mHandle) {
        return;
    }

    if (mState == PlaybackState::Playing || mState == PlaybackState::Paused) {
        const char* cmd[] = {"stop", nullptr};
        mpv_command(mHandle, cmd);
    }

    mpv_wakeup(mHandle);
    mpv_terminate_destroy(mHandle);
    mHandle = nullptr;
    mState = PlaybackState::Stopped;

    logMessage("INFO", "AudioEngine cleaned up");
}

void MpvAudioEngine::play(const std::string& filePath) {
    if (!mHandle) {
        logMessage("ERROR", "AudioEngine not initialized");
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "AudioEngine not initialized");
        }
        return;
    }

    try {
        if (!fileExists(filePath)) {
            logMessage("ERROR", "File not found: " + filePath);
            if (mOnError) {
                mOnError(ErrorCode::FileNotFound, "File not found: " + filePath);
            }
            return;
        }

        const char* args[] = {"loadfile", filePath.c_str(), nullptr};
        const int result = mpv_command(mHandle, args);

        if (result < 0) {
            const std::string errorStr = mpv_error_string(result);
            logMessage("ERROR", "Failed to load file: " + filePath + " - " + errorStr);

            if (mOnError) {
                mOnError(mapMpvError(result), "Failed to load file: " + filePath);
            }
            return;
        }

        mState = PlaybackState::Playing;
        resetPlaybackProgressTracking();
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

void MpvAudioEngine::pause() {
    try {
        if (!mHandle || mState != PlaybackState::Playing) {
            return;
        }

        const char* args[] = {"set", "pause", "yes", nullptr};
        const int result = mpv_command(mHandle, args);

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

void MpvAudioEngine::resume() {
    try {
        if (!mHandle || mState != PlaybackState::Paused) {
            return;
        }

        const char* args[] = {"set", "pause", "no", nullptr};
        const int result = mpv_command(mHandle, args);

        if (result < 0) {
            logMessage("WARN", "Failed to resume: " + std::string(mpv_error_string(result)));
            if (mOnError) {
                mOnError(ErrorCode::PlaybackFailed, "Failed to resume playback");
            }
            return;
        }

        mState = PlaybackState::Playing;
        resetPlaybackProgressTracking();
        logMessage("INFO", "Playback resumed");
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in resume(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in resume()");
    }
}

void MpvAudioEngine::stop() {
    if (!mHandle) {
        return;
    }

    const char* args[] = {"stop", nullptr};
    mpv_command(mHandle, args);

    mState = PlaybackState::Stopped;
    resetPlaybackProgressTracking();
    logMessage("INFO", "Playback stopped");
}

void MpvAudioEngine::next() {
    if (!mHandle) {
        return;
    }

    logMessage("INFO", "Skipping to next track");
    if (mOnTrackFinished) {
        mOnTrackFinished();
    }
}

void MpvAudioEngine::previous() {
    if (!mHandle) {
        return;
    }

    logMessage("INFO", "Skipping to previous track");

    const char* args[] = {"seek", "0", "absolute", nullptr};
    mpv_command(mHandle, args);
}

void MpvAudioEngine::seek(double positionSeconds) {
    try {
        if (!mHandle) {
            return;
        }

        const std::string posStr = std::to_string(positionSeconds);
        const char* args[] = {"seek", posStr.c_str(), "absolute", nullptr};
        const int result = mpv_command(mHandle, args);

        if (result < 0) {
            logMessage("WARN", "Seek failed: " + std::string(mpv_error_string(result)));
        } else {
            resetPlaybackProgressTracking();
            logMessage("INFO", "Seeked to: " + posStr + "s");
        }
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in seek(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in seek()");
    }
}

void MpvAudioEngine::setVolume(int level) {
    if (!mHandle) {
        return;
    }

    const int volume = level < 0 ? 0 : (level > 100 ? 100 : level);
    int64_t volInt = volume;
    const int result = mpv_set_property(mHandle, "volume", MPV_FORMAT_INT64, &volInt);

    if (result < 0) {
        logMessage("WARN", "Failed to set volume: " + std::string(mpv_error_string(result)));
    } else {
        logMessage("INFO", "Volume set to: " + std::to_string(volume));
    }
}

int MpvAudioEngine::getVolume() const {
    if (!mHandle) {
        return 0;
    }

    int64_t volume = 0;
    const int result = mpv_get_property(mHandle, "volume", MPV_FORMAT_INT64, &volume);

    if (result < 0) {
        logMessage("WARN", "Failed to get volume: " + std::string(mpv_error_string(result)));
        return 0;
    }

    return static_cast<int>(volume);
}

double MpvAudioEngine::getCurrentPosition() const {
    if (!mHandle) {
        return 0.0;
    }

    double pos = 0.0;
    const int result = mpv_get_property(mHandle, "time-pos", MPV_FORMAT_DOUBLE, &pos);
    if (result < 0) {
        return 0.0;
    }

    return pos;
}

double MpvAudioEngine::getDuration() const {
    if (!mHandle) {
        return 0.0;
    }

    double duration = 0.0;
    const int result = mpv_get_property(mHandle, "duration", MPV_FORMAT_DOUBLE, &duration);
    if (result < 0) {
        return 0.0;
    }

    return duration;
}

bool MpvAudioEngine::isPlaying() const {
    return mState == PlaybackState::Playing;
}

bool MpvAudioEngine::isInitialized() const {
    return mHandle != nullptr;
}

AudioEngine::PlaybackState MpvAudioEngine::getPlaybackState() const {
    return mState;
}

void MpvAudioEngine::setOnTrackFinished(TrackFinishedCallback callback) {
    mOnTrackFinished = std::move(callback);
}

void MpvAudioEngine::setOnFileLoaded(FileLoadedCallback callback) {
    mOnFileLoaded = std::move(callback);
}

void MpvAudioEngine::setOnError(ErrorCallback callback) {
    mOnError = std::move(callback);
}

void MpvAudioEngine::processEvents() {
    if (!mHandle) {
        return;
    }

    try {
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

        samplePlaybackDiagnostics();
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in processEvents(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in processEvents()");
    }
}

void MpvAudioEngine::handleEvent(const mpv_event* event) {
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
                if (mOnFileLoaded) {
                    mOnFileLoaded();
                }
                resetPlaybackProgressTracking();
                logPlaybackSnapshot("file-loaded");
                break;

            case MPV_EVENT_END_FILE: {
                auto* eof = static_cast<mpv_event_end_file*>(event->data);
                if (!eof) {
                    logMessage("WARN", "END_FILE event had no payload");
                    break;
                }

                if (eof->reason == MPV_END_FILE_REASON_ERROR) {
                    logMessage("ERROR", "Track ended with error: " + std::string(mpv_error_string(eof->error)));
                    mState = PlaybackState::Stopped;
                    resetPlaybackProgressTracking();
                    if (mOnError) {
                        mOnError(mapMpvError(eof->error), "Playback error: track ended with error");
                    }
                } else if (eof->reason == MPV_END_FILE_REASON_EOF) {
                    logMessage("INFO", "Track finished");
                    mState = PlaybackState::Stopped;
                    resetPlaybackProgressTracking();
                    if (mOnTrackFinished) {
                        mOnTrackFinished();
                    }
                } else {
                    logMessage("DEBUG", "Track ended for non-EOF reason; skipping auto-next");
                }
                break;
            }

            case MPV_EVENT_PLAYBACK_RESTART:
                logMessage("INFO", "Playback restarted");
                mState = PlaybackState::Playing;
                resetPlaybackProgressTracking();
                logPlaybackSnapshot("playback-restart");
                break;

            case MPV_EVENT_SEEK:
                resetPlaybackProgressTracking();
                logMessage("INFO", "Seek event");
                break;

            case MPV_EVENT_AUDIO_RECONFIG:
                logMessage("INFO", "Audio reconfigured");
                logPlaybackSnapshot("audio-reconfig");
                break;

            case MPV_EVENT_PROPERTY_CHANGE: {
                auto* prop = static_cast<mpv_event_property*>(event->data);
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
                    resetPlaybackProgressTracking();
                    logPlaybackSnapshot("pause-event");
                    break;
                }

                if (eventId == kMpvEventUnpause) {
                    logMessage("INFO", "Unpaused by event");
                    mState = PlaybackState::Playing;
                    resetPlaybackProgressTracking();
                    logPlaybackSnapshot("unpause-event");
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
                    resetPlaybackProgressTracking();
                    logPlaybackSnapshot("error-event");
                    if (mOnError) {
                        mOnError(mapMpvError(error->error),
                                 "MPV Error: " + std::string(mpv_error_string(error->error)));
                    }
                    break;
                }

                break;
        }
    } catch (const std::exception& e) {
        logMessage("ERROR", "Exception in handleEvent(): " + std::string(e.what()));
    } catch (...) {
        logMessage("ERROR", "Unknown exception in handleEvent()");
    }
}

void MpvAudioEngine::samplePlaybackDiagnostics() {
    if (!mHandle) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (mLastDiagnosticSampleAt != std::chrono::steady_clock::time_point{} &&
        now - mLastDiagnosticSampleAt < kDiagnosticSampleInterval) {
        return;
    }
    mLastDiagnosticSampleAt = now;

    bool pause = false;
    if (readFlagProperty(mHandle, "pause", pause) &&
        (!mHasLastObservedPause || pause != mLastObservedPause)) {
        logMessage("INFO", "Diagnostic state change: pause=" + boolToText(pause));
        mHasLastObservedPause = true;
        mLastObservedPause = pause;
    }

    bool coreIdle = false;
    if (readFlagProperty(mHandle, "core-idle", coreIdle) &&
        (!mHasLastObservedCoreIdle || coreIdle != mLastObservedCoreIdle)) {
        logMessage("INFO", "Diagnostic state change: core-idle=" + boolToText(coreIdle));
        mHasLastObservedCoreIdle = true;
        mLastObservedCoreIdle = coreIdle;
    }

    bool pausedForCache = false;
    if (readFlagProperty(mHandle, "paused-for-cache", pausedForCache) &&
        (!mHasLastObservedPausedForCache || pausedForCache != mLastObservedPausedForCache)) {
        logMessage("INFO", "Diagnostic state change: paused-for-cache=" + boolToText(pausedForCache));
        mHasLastObservedPausedForCache = true;
        mLastObservedPausedForCache = pausedForCache;
        if (pausedForCache) {
            logPlaybackSnapshot("paused-for-cache");
        }
    }

    double cacheBufferingState = 0.0;
    if (readDoubleProperty(mHandle, "cache-buffering-state", cacheBufferingState) &&
        (mLastObservedCacheBufferingState < 0.0 ||
         std::fabs(cacheBufferingState - mLastObservedCacheBufferingState) >= 10.0)) {
        logMessage("INFO", "Diagnostic state change: cache-buffering-state=" +
                               std::to_string(cacheBufferingState));
        mLastObservedCacheBufferingState = cacheBufferingState;
    }

    double demuxerCacheDuration = 0.0;
    if (readDoubleProperty(mHandle, "demuxer-cache-duration", demuxerCacheDuration) &&
        (mLastObservedDemuxerCacheDuration < 0.0 ||
         std::fabs(demuxerCacheDuration - mLastObservedDemuxerCacheDuration) >= 2.0)) {
        logMessage("INFO", "Diagnostic state change: demuxer-cache-duration=" +
                               std::to_string(demuxerCacheDuration));
        mLastObservedDemuxerCacheDuration = demuxerCacheDuration;
    }

    std::string audioDevice;
    if (readStringProperty(mHandle, "audio-device", audioDevice) &&
        audioDevice != mLastObservedAudioDevice) {
        logMessage("INFO", "Diagnostic state change: audio-device=" + audioDevice);
        mLastObservedAudioDevice = audioDevice;
    }

    if (mState != PlaybackState::Playing) {
        mHasLastPlaybackPosition = false;
        mPlaybackStallLogged = false;
        return;
    }

    double currentPosition = 0.0;
    if (!readDoubleProperty(mHandle, "time-pos", currentPosition)) {
        return;
    }

    if (!mHasLastPlaybackPosition || currentPosition > mLastPlaybackPosition + 0.05) {
        mLastPlaybackPosition = currentPosition;
        mLastPlaybackAdvanceAt = now;
        mHasLastPlaybackPosition = true;
        mPlaybackStallLogged = false;
        return;
    }

    if (!mHasLastPlaybackPosition) {
        mLastPlaybackPosition = currentPosition;
        mLastPlaybackAdvanceAt = now;
        mHasLastPlaybackPosition = true;
        return;
    }

    if (!mPlaybackStallLogged && now - mLastPlaybackAdvanceAt >= kPlaybackStallThreshold) {
        std::ostringstream message;
        message << "Playback stall suspected: time-pos stuck at " << currentPosition
                << "s for at least "
                << std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastPlaybackAdvanceAt).count()
                << "ms";
        logMessage("WARN", message.str());
        logPlaybackSnapshot("time-pos-stalled");
        mPlaybackStallLogged = true;
    }
}

void MpvAudioEngine::logPlaybackSnapshot(const char* reason) {
    if (!mHandle) {
        return;
    }

    std::ostringstream message;
    message << "Playback diagnostic snapshot [" << (reason ? reason : "unknown") << "]";

    double timePos = 0.0;
    if (readDoubleProperty(mHandle, "time-pos", timePos)) {
        message << " time-pos=" << timePos;
    }

    bool pause = false;
    if (readFlagProperty(mHandle, "pause", pause)) {
        message << " pause=" << boolToText(pause);
    }

    bool coreIdle = false;
    if (readFlagProperty(mHandle, "core-idle", coreIdle)) {
        message << " core-idle=" << boolToText(coreIdle);
    }

    bool pausedForCache = false;
    if (readFlagProperty(mHandle, "paused-for-cache", pausedForCache)) {
        message << " paused-for-cache=" << boolToText(pausedForCache);
    }

    double cacheBufferingState = 0.0;
    if (readDoubleProperty(mHandle, "cache-buffering-state", cacheBufferingState)) {
        message << " cache-buffering-state=" << cacheBufferingState;
    }

    double demuxerCacheDuration = 0.0;
    if (readDoubleProperty(mHandle, "demuxer-cache-duration", demuxerCacheDuration)) {
        message << " demuxer-cache-duration=" << demuxerCacheDuration;
    }

    std::string audioDevice;
    if (readStringProperty(mHandle, "audio-device", audioDevice)) {
        message << " audio-device=" << audioDevice;
    }

    logMessage("INFO", message.str());
}

void MpvAudioEngine::resetPlaybackProgressTracking() {
    mHasLastPlaybackPosition = false;
    mLastPlaybackPosition = 0.0;
    mPlaybackStallLogged = false;
    mLastPlaybackAdvanceAt = std::chrono::steady_clock::now();
}

AudioEngine::ErrorCode MpvAudioEngine::mapMpvError(int mpvError) {
    switch (mpvError) {
        case 0:
            return ErrorCode::NoError;
        case -10:
            return ErrorCode::FileNotFound;
        case -14:
            return ErrorCode::UnsupportedCodec;
        case -11:
        case -12:
            return ErrorCode::DeviceError;
        default:
            return ErrorCode::PlaybackFailed;
    }
}

int MpvAudioEngine::sendCommand(const char** args) {
    if (!mHandle || !args) {
        return -1;
    }

    return mpv_command(mHandle, args);
}

void* MpvAudioEngine::getProperty(const char* property, const char* /* type */) {
    if (!mHandle || !property) {
        return nullptr;
    }

    void* data = nullptr;
    mpv_get_property(mHandle, property, MPV_FORMAT_DOUBLE, &data);
    return data;
}

void MpvAudioEngine::setProperty(const char* property, const char* /* type */, void* value) {
    if (!mHandle || !property || !value) {
        return;
    }

    mpv_set_property(mHandle, property, MPV_FORMAT_DOUBLE, value);
}
