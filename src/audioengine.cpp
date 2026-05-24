#include "audioengine.h"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <fstream>

// libmpv C API headers
#include <mpv/client.h>

// Helper function to log messages
static void logMessage(const std::string& level, const std::string& message) {
    std::cerr << "[AudioEngine " << level << "] " << message << std::endl;
}

// Helper function to check if file exists
static bool fileExists(const std::string& filePath) {
    std::ifstream f(filePath);
    return f.good();
}

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
        switch (event->event_id) {
            case MPV_EVENT_FILE_LOADED:
                logMessage("INFO", "File loaded successfully");
                if (mState == PlaybackState::Stopped) {
                    mState = PlaybackState::Playing;
                }
                break;

            case MPV_EVENT_END_FILE: {
                mpv_event_end_file* eof = static_cast<mpv_event_end_file*>(event->data);
                if (eof && eof->reason == MPV_END_FILE_REASON_ERROR) {
                    // Handle playback error on end file
                    logMessage("ERROR", "Track ended with error: " + std::string(mpv_error_string(eof->error)));
                    mState = PlaybackState::Stopped;
                    if (mOnError) {
                        ErrorCode errorCode = mapMpvError(eof->error);
                        mOnError(errorCode, "Playback error: track ended with error");
                    }
                } else {
                    logMessage("INFO", "Track finished");
                    mState = PlaybackState::Stopped;
                    if (mOnTrackFinished) {
                        mOnTrackFinished();
                    }
                }
                break;
            }

            case MPV_EVENT_PLAYBACK_RESTART:
                logMessage("INFO", "Playback restarted");
                mState = PlaybackState::Playing;
                break;

            case MPV_EVENT_PAUSE:
                logMessage("INFO", "Paused by event");
                mState = PlaybackState::Paused;
                break;

            case MPV_EVENT_UNPAUSE:
                logMessage("INFO", "Unpaused by event");
                mState = PlaybackState::Playing;
                break;

            case MPV_EVENT_SEEK: {
                logMessage("INFO", "Seek event");
                break;
            }

            case MPV_EVENT_PROPERTY_CHANGE: {
                mpv_event_property* prop = static_cast<mpv_event_property*>(event->data);
                logMessage("DEBUG", "Property changed: " + std::string(prop->name));
                break;
            }

            case MPV_EVENT_ERROR: {
                mpv_event_error* error = static_cast<mpv_event_error*>(event->data);
                logMessage("ERROR", "MPV Error: " + std::string(mpv_error_string(error->error)));
                mState = PlaybackState::Stopped;
                if (mOnError) {
                    mOnError(mapMpvError(error->error),
                            "MPV Error: " + std::string(mpv_error_string(error->error)));
                }
                break;
            }

            case MPV_EVENT_SHUTDOWN:
                logMessage("INFO", "MPV shutting down");
                mState = PlaybackState::Stopped;
                break;

            default:
                // Ignore other events
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
