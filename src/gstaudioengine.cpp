#include "gstaudioengine.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
constexpr auto kDiagnosticSampleInterval = std::chrono::milliseconds(1000);
constexpr auto kPlaybackStallThreshold = std::chrono::milliseconds(2000);

void logMessage(const std::string& level, const std::string& message) {
    std::cerr << "[AudioEngine " << level << "] " << message << std::endl;
}

bool fileExists(const std::string& filePath) {
    std::ifstream f(filePath);
    return f.good();
}

std::string makeUriFromPath(const std::string& filePath) {
    GError* error = nullptr;
    gchar* uri = gst_filename_to_uri(filePath.c_str(), &error);
    if (!uri) {
        const std::string message = error ? error->message : "unknown uri conversion failure";
        if (error) {
            g_error_free(error);
        }
        logMessage("ERROR", "Failed to convert file path to URI: " + message);
        return {};
    }

    std::string result(uri);
    g_free(uri);
    return result;
}
} // namespace

GstAudioEngine::GstAudioEngine() {
    initializeGStreamer();
}

GstAudioEngine::~GstAudioEngine() {
    cleanupGStreamer();
}

void GstAudioEngine::initializeGStreamer() {
    if (mPlaybin) {
        logMessage("WARN", "AudioEngine already initialized");
        return;
    }

    mPlaybin = gst_element_factory_make("playbin", "boombox-playbin");
    if (!mPlaybin) {
        logMessage("ERROR", "Failed to create GStreamer playbin");
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "Failed to create GStreamer playbin");
        }
        return;
    }

    GstElement* videoSink = gst_element_factory_make("fakesink", "boombox-video-sink");
    if (videoSink) {
        g_object_set(mPlaybin, "video-sink", videoSink, nullptr);
        gst_object_unref(videoSink);
    }

    GstElement* textSink = gst_element_factory_make("fakesink", "boombox-text-sink");
    if (textSink) {
        g_object_set(mPlaybin, "text-sink", textSink, nullptr);
        gst_object_unref(textSink);
    }

    mBus = gst_element_get_bus(mPlaybin);
    if (!mBus) {
        logMessage("ERROR", "Failed to acquire GStreamer bus");
        cleanupGStreamer();
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "Failed to acquire GStreamer bus");
        }
        return;
    }

    setVolume(100);
    resetPlaybackProgressTracking();
    logMessage("INFO", "GStreamer backend initialized successfully");
}

void GstAudioEngine::cleanupGStreamer() {
    if (!mPlaybin) {
        return;
    }

    gst_element_set_state(mPlaybin, GST_STATE_NULL);

    if (mBus) {
        gst_bus_set_flushing(mBus, TRUE);
        gst_object_unref(mBus);
        mBus = nullptr;
    }

    gst_object_unref(mPlaybin);
    mPlaybin = nullptr;
    mState = PlaybackState::Stopped;
    mFileLoadedSignaled = false;
    mBufferingActive = false;
    mLastBufferingPercent = -1;
    resetPlaybackProgressTracking();
    logMessage("INFO", "GStreamer backend cleaned up");
}

void GstAudioEngine::resetForNewTrack() {
    mFileLoadedSignaled = false;
    mBufferingActive = false;
    mLastBufferingPercent = -1;
    resetPlaybackProgressTracking();
}

void GstAudioEngine::flushPendingMessages() {
    if (!mBus) {
        return;
    }

    gst_bus_set_flushing(mBus, TRUE);
    gst_bus_set_flushing(mBus, FALSE);

    while (GstMessage* message = gst_bus_pop(mBus)) {
        gst_message_unref(message);
    }
}

void GstAudioEngine::play(const std::string& filePath) {
    if (!mPlaybin || !mBus) {
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "GStreamer backend not initialized");
        }
        return;
    }

    if (!fileExists(filePath)) {
        if (mOnError) {
            mOnError(ErrorCode::FileNotFound, "File not found: " + filePath);
        }
        return;
    }

    const std::string uri = makeUriFromPath(filePath);
    if (uri.empty()) {
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "Failed to convert file path to URI");
        }
        return;
    }

    gst_element_set_state(mPlaybin, GST_STATE_NULL);
    flushPendingMessages();
    resetForNewTrack();

    g_object_set(mPlaybin, "uri", uri.c_str(), nullptr);

    const GstStateChangeReturn result = gst_element_set_state(mPlaybin, GST_STATE_PLAYING);
    if (result == GST_STATE_CHANGE_FAILURE) {
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "Failed to start GStreamer playback");
        }
        return;
    }

    mState = PlaybackState::Playing;
    logMessage("INFO", "Playing: " + filePath);
}

void GstAudioEngine::pause() {
    if (!mPlaybin || mState != PlaybackState::Playing) {
        return;
    }

    const GstStateChangeReturn result = gst_element_set_state(mPlaybin, GST_STATE_PAUSED);
    if (result == GST_STATE_CHANGE_FAILURE) {
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "Failed to pause playback");
        }
        return;
    }

    mState = PlaybackState::Paused;
    logMessage("INFO", "Playback paused");
}

void GstAudioEngine::resume() {
    if (!mPlaybin || mState != PlaybackState::Paused) {
        return;
    }

    const GstStateChangeReturn result = gst_element_set_state(mPlaybin, GST_STATE_PLAYING);
    if (result == GST_STATE_CHANGE_FAILURE) {
        if (mOnError) {
            mOnError(ErrorCode::PlaybackFailed, "Failed to resume playback");
        }
        return;
    }

    mState = PlaybackState::Playing;
    resetPlaybackProgressTracking();
    logMessage("INFO", "Playback resumed");
}

void GstAudioEngine::stop() {
    if (!mPlaybin) {
        return;
    }

    gst_element_set_state(mPlaybin, GST_STATE_NULL);
    flushPendingMessages();
    mState = PlaybackState::Stopped;
    mFileLoadedSignaled = false;
    resetPlaybackProgressTracking();
    logMessage("INFO", "Playback stopped");
}

void GstAudioEngine::next() {
    if (mOnTrackFinished) {
        mOnTrackFinished();
    }
}

void GstAudioEngine::previous() {
    seek(0.0);
}

void GstAudioEngine::seek(double positionSeconds) {
    if (!mPlaybin) {
        return;
    }

    const gint64 position = static_cast<gint64>(positionSeconds * GST_SECOND);
    if (!gst_element_seek_simple(mPlaybin,
                                 GST_FORMAT_TIME,
                                 static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                 position)) {
        logMessage("WARN", "Seek failed");
        return;
    }

    resetPlaybackProgressTracking();
    logMessage("INFO", "Seeked to: " + std::to_string(positionSeconds) + "s");
}

void GstAudioEngine::setVolume(int level) {
    if (!mPlaybin) {
        return;
    }

    const int volume = std::clamp(level, 0, 100);
    const gdouble normalized = static_cast<gdouble>(volume) / 100.0;
    g_object_set(mPlaybin, "volume", normalized, nullptr);
}

int GstAudioEngine::getVolume() const {
    if (!mPlaybin) {
        return 0;
    }

    gdouble volume = 0.0;
    g_object_get(mPlaybin, "volume", &volume, nullptr);
    return static_cast<int>(std::round(volume * 100.0));
}

double GstAudioEngine::getCurrentPosition() const {
    if (!mPlaybin) {
        return 0.0;
    }

    gint64 position = 0;
    if (!gst_element_query_position(mPlaybin, GST_FORMAT_TIME, &position)) {
        return 0.0;
    }

    return static_cast<double>(position) / static_cast<double>(GST_SECOND);
}

double GstAudioEngine::getDuration() const {
    if (!mPlaybin) {
        return 0.0;
    }

    gint64 duration = 0;
    if (!gst_element_query_duration(mPlaybin, GST_FORMAT_TIME, &duration)) {
        return 0.0;
    }

    return static_cast<double>(duration) / static_cast<double>(GST_SECOND);
}

bool GstAudioEngine::isPlaying() const {
    return mState == PlaybackState::Playing;
}

bool GstAudioEngine::isInitialized() const {
    return mPlaybin != nullptr && mBus != nullptr;
}

AudioEngine::PlaybackState GstAudioEngine::getPlaybackState() const {
    return mState;
}

void GstAudioEngine::setOnTrackFinished(TrackFinishedCallback callback) {
    mOnTrackFinished = std::move(callback);
}

void GstAudioEngine::setOnFileLoaded(FileLoadedCallback callback) {
    mOnFileLoaded = std::move(callback);
}

void GstAudioEngine::setOnError(ErrorCallback callback) {
    mOnError = std::move(callback);
}

void GstAudioEngine::processEvents() {
    if (!mBus) {
        return;
    }

    while (GstMessage* message = gst_bus_timed_pop(mBus, 0)) {
        handleMessage(message);
        gst_message_unref(message);
    }

    samplePlaybackDiagnostics();
}

void GstAudioEngine::handleMessage(GstMessage* message) {
    if (!message) {
        return;
    }

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* error = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &error, &debug);

            std::ostringstream text;
            text << "GStreamer error";
            if (error && error->message) {
                text << ": " << error->message;
            }
            if (debug) {
                text << " (" << debug << ")";
            }
            logMessage("ERROR", text.str());

            mState = PlaybackState::Stopped;
            resetPlaybackProgressTracking();
            if (mOnError) {
                mOnError(mapGstError(error), text.str());
            }

            if (debug) {
                g_free(debug);
            }
            if (error) {
                g_error_free(error);
            }
            break;
        }

        case GST_MESSAGE_EOS:
            logMessage("INFO", "Track finished");
            mState = PlaybackState::Stopped;
            mFileLoadedSignaled = false;
            resetPlaybackProgressTracking();
            if (mOnTrackFinished) {
                mOnTrackFinished();
            }
            break;

        case GST_MESSAGE_BUFFERING: {
            gint percent = 0;
            gst_message_parse_buffering(message, &percent);
            if (mLastBufferingPercent < 0 || std::abs(percent - mLastBufferingPercent) >= 10 || percent == 0 ||
                percent == 100) {
                logMessage("INFO", "Diagnostic state change: buffering=" + std::to_string(percent) + "%");
                mLastBufferingPercent = percent;
            }

            const bool buffering = percent < 100;
            if (buffering != mBufferingActive) {
                mBufferingActive = buffering;
                if (buffering) {
                    logPlaybackSnapshot("buffering");
                }
            }

            if (buffering && mState == PlaybackState::Playing) {
                gst_element_set_state(mPlaybin, GST_STATE_PAUSED);
            } else if (!buffering && mState == PlaybackState::Playing) {
                gst_element_set_state(mPlaybin, GST_STATE_PLAYING);
            }
            break;
        }

        case GST_MESSAGE_CLOCK_LOST:
            logMessage("WARN", "Audio clock lost; restarting pipeline clock");
            if (mPlaybin && mState == PlaybackState::Playing) {
                gst_element_set_state(mPlaybin, GST_STATE_PAUSED);
                gst_element_set_state(mPlaybin, GST_STATE_PLAYING);
            }
            break;

        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(message) == GST_OBJECT(mPlaybin)) {
                GstState oldState = GST_STATE_NULL;
                GstState newState = GST_STATE_NULL;
                GstState pendingState = GST_STATE_VOID_PENDING;
                gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);

                logMessage("INFO",
                           "Pipeline state changed: " + std::string(gst_element_state_get_name(oldState)) + " -> " +
                               gst_element_state_get_name(newState) + ", pending=" +
                               gst_element_state_get_name(pendingState));

                signalFileLoadedIfReady(newState);
                switch (newState) {
                    case GST_STATE_PLAYING:
                        mState = PlaybackState::Playing;
                        resetPlaybackProgressTracking();
                        break;
                    case GST_STATE_PAUSED:
                        if (mState != PlaybackState::Stopped) {
                            mState = PlaybackState::Paused;
                        }
                        break;
                    default:
                        if (pendingState == GST_STATE_VOID_PENDING) {
                            mState = PlaybackState::Stopped;
                        }
                        break;
                }
            }
            break;

        case GST_MESSAGE_ASYNC_DONE:
            signalFileLoadedIfReady(GST_STATE_PLAYING);
            break;

        default:
            break;
    }
}

void GstAudioEngine::samplePlaybackDiagnostics() {
    if (!mPlaybin) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (mLastDiagnosticSampleAt != std::chrono::steady_clock::time_point{} &&
        now - mLastDiagnosticSampleAt < kDiagnosticSampleInterval) {
        return;
    }
    mLastDiagnosticSampleAt = now;

    if (mState != PlaybackState::Playing) {
        mHasLastPlaybackPosition = false;
        mPlaybackStallLogged = false;
        return;
    }

    const double currentPosition = getCurrentPosition();
    if (!mHasLastPlaybackPosition || currentPosition > mLastPlaybackPosition + 0.05) {
        mLastPlaybackPosition = currentPosition;
        mLastPlaybackAdvanceAt = now;
        mHasLastPlaybackPosition = true;
        mPlaybackStallLogged = false;
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

void GstAudioEngine::logPlaybackSnapshot(const char* reason) const {
    std::ostringstream message;
    message << "Playback diagnostic snapshot [" << (reason ? reason : "unknown") << "]"
            << " time-pos=" << getCurrentPosition() << " duration=" << getDuration()
            << " buffering=" << (mBufferingActive ? "yes" : "no");
    logMessage("INFO", message.str());
}

void GstAudioEngine::signalFileLoadedIfReady(GstState newState) {
    if (mFileLoadedSignaled) {
        return;
    }

    if (newState != GST_STATE_PAUSED && newState != GST_STATE_PLAYING) {
        return;
    }

    mFileLoadedSignaled = true;
    if (mOnFileLoaded) {
        mOnFileLoaded();
    }
    logPlaybackSnapshot("file-loaded");
}

void GstAudioEngine::resetPlaybackProgressTracking() {
    mHasLastPlaybackPosition = false;
    mLastPlaybackPosition = 0.0;
    mPlaybackStallLogged = false;
    mLastPlaybackAdvanceAt = std::chrono::steady_clock::now();
}

AudioEngine::ErrorCode GstAudioEngine::mapGstError(const GError* error) {
    if (!error) {
        return ErrorCode::PlaybackFailed;
    }

    if (error->domain == gst_resource_error_quark()) {
        switch (static_cast<GstResourceError>(error->code)) {
            case GST_RESOURCE_ERROR_NOT_FOUND:
                return ErrorCode::FileNotFound;
            case GST_RESOURCE_ERROR_OPEN_READ:
            case GST_RESOURCE_ERROR_OPEN_WRITE:
            case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
            case GST_RESOURCE_ERROR_READ:
            case GST_RESOURCE_ERROR_WRITE:
            case GST_RESOURCE_ERROR_SEEK:
                return ErrorCode::PlaybackFailed;
            case GST_RESOURCE_ERROR_BUSY:
            case GST_RESOURCE_ERROR_SETTINGS:
            default:
                return ErrorCode::DeviceError;
        }
    }

    if (error->domain == gst_stream_error_quark()) {
        switch (static_cast<GstStreamError>(error->code)) {
            case GST_STREAM_ERROR_CODEC_NOT_FOUND:
            case GST_STREAM_ERROR_TYPE_NOT_FOUND:
            case GST_STREAM_ERROR_WRONG_TYPE:
            case GST_STREAM_ERROR_FORMAT:
                return ErrorCode::UnsupportedCodec;
            case GST_STREAM_ERROR_DECODE:
            case GST_STREAM_ERROR_DECRYPT:
                return ErrorCode::CorruptedFile;
            default:
                return ErrorCode::PlaybackFailed;
        }
    }

    if (error->domain == gst_core_error_quark() &&
        error->code == GST_CORE_ERROR_MISSING_PLUGIN) {
        return ErrorCode::UnsupportedCodec;
    }

    return ErrorCode::PlaybackFailed;
}
