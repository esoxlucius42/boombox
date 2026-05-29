#include "gstaudioengine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace {
constexpr auto kDiagnosticSampleInterval = std::chrono::milliseconds(1000);
constexpr auto kPlaybackStallThreshold = std::chrono::milliseconds(2000);
constexpr auto kSpectrumAnalysisWindow = std::chrono::milliseconds(50);
constexpr auto kSpectrumRefreshInterval = std::chrono::milliseconds(25);
constexpr auto kPlaybackQueueLeadTime = std::chrono::milliseconds(500);
constexpr int kAnalyzerSampleRate = 44100;
constexpr std::array<double, SpectrumLevels::kBandCount> kSpectrumBandCenters = {31.0, 62.0, 125.0, 200.0, 250.0, 400.0, 500.0};
constexpr double kSpectrumDbFloor = 66.0;
constexpr double kSpectrumDbRange = 38.0;
constexpr double kSpectrumResponseGamma = 0.72;
constexpr float kSpectrumAverageBlend = 0.18F;
constexpr float kSpectrumTargetFill = 0.92F;
constexpr float kSpectrumAdaptiveFloor = 0.24F;
constexpr float kSpectrumAdaptiveAttack = 0.22F;
constexpr float kSpectrumAdaptiveRelease = 0.08F;
constexpr float kSpectrumMinAdaptiveGain = 0.85F;
constexpr float kSpectrumMaxAdaptiveGain = 2.75F;

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

void setElementPropertyIfPresent(GstElement* element, const char* propertyName, const GValue* value)
{
    if (!element || !propertyName || !value) {
        return;
    }

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(element), propertyName)) {
        g_object_set_property(G_OBJECT(element), propertyName, value);
    }
}

void setBooleanPropertyIfPresent(GstElement* element, const char* propertyName, gboolean enabled)
{
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_set_boolean(&value, enabled);
    setElementPropertyIfPresent(element, propertyName, &value);
    g_value_unset(&value);
}

void setUIntPropertyIfPresent(GstElement* element, const char* propertyName, guint value)
{
    GValue gvalue = G_VALUE_INIT;
    g_value_init(&gvalue, G_TYPE_UINT);
    g_value_set_uint(&gvalue, value);
    setElementPropertyIfPresent(element, propertyName, &gvalue);
    g_value_unset(&gvalue);
}
} // namespace

struct GstAudioEngine::AnalyzerState {
    double sampleRate = kAnalyzerSampleRate;
    std::size_t windowFrames =
        static_cast<std::size_t>(kAnalyzerSampleRate * kSpectrumAnalysisWindow.count() / 1000);
    std::size_t hopFrames =
        static_cast<std::size_t>(kAnalyzerSampleRate * kSpectrumRefreshInterval.count() / 1000);
    std::vector<float> monoBuffer{};
    std::array<float, SpectrumLevels::kBandCount> smoothedLevels{};
    float adaptiveReference = kSpectrumTargetFill;

    void reset()
    {
        sampleRate = kAnalyzerSampleRate;
        windowFrames = static_cast<std::size_t>(kAnalyzerSampleRate * kSpectrumAnalysisWindow.count() / 1000);
        hopFrames = static_cast<std::size_t>(kAnalyzerSampleRate * kSpectrumRefreshInterval.count() / 1000);
        adaptiveReference = kSpectrumTargetFill;
        monoBuffer.clear();
        smoothedLevels.fill(0.0F);
    }

    void updateFormat(int rate)
    {
        const int normalizedRate = rate > 0 ? rate : kAnalyzerSampleRate;
        if (std::abs(sampleRate - static_cast<double>(normalizedRate)) < 1.0) {
            return;
        }

        sampleRate = normalizedRate;
        windowFrames = std::max<std::size_t>(
            512,
            static_cast<std::size_t>(sampleRate * kSpectrumAnalysisWindow.count() / 1000.0));
        hopFrames = std::max<std::size_t>(
            256,
            static_cast<std::size_t>(sampleRate * kSpectrumRefreshInterval.count() / 1000.0));
        adaptiveReference = kSpectrumTargetFill;
        monoBuffer.clear();
        smoothedLevels.fill(0.0F);
    }

    std::optional<SpectrumLevels> ingest(const float* samples, std::size_t frames, int channels)
    {
        if (!samples || frames == 0 || channels <= 0) {
            return std::nullopt;
        }

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const std::size_t sampleIndex = frame * static_cast<std::size_t>(channels);
            float mixedSample = 0.0F;
            for (int channel = 0; channel < channels; ++channel) {
                mixedSample += samples[sampleIndex + static_cast<std::size_t>(channel)];
            }
            monoBuffer.push_back(mixedSample / static_cast<float>(channels));
        }

        std::optional<SpectrumLevels> latest;
        while (monoBuffer.size() >= windowFrames) {
            latest = analyzeWindow();
            monoBuffer.erase(monoBuffer.begin(), monoBuffer.begin() + static_cast<std::ptrdiff_t>(hopFrames));
        }

        return latest;
    }

private:
    static double goertzelMagnitude(const float* samples, std::size_t sampleCount, double targetFrequency, double rate)
    {
        if (!samples || sampleCount == 0 || targetFrequency <= 0.0 || rate <= 0.0) {
            return 0.0;
        }

        const double normalizedFrequency = targetFrequency / rate;
        const double omega = 2.0 * M_PI * normalizedFrequency;
        const double coefficient = 2.0 * std::cos(omega);
        double q0 = 0.0;
        double q1 = 0.0;
        double q2 = 0.0;

        for (std::size_t i = 0; i < sampleCount; ++i) {
            const double window =
                0.5 - 0.5 * std::cos((2.0 * M_PI * static_cast<double>(i)) / static_cast<double>(sampleCount - 1));
            q0 = window * static_cast<double>(samples[i]) + coefficient * q1 - q2;
            q2 = q1;
            q1 = q0;
        }

        const double power = q1 * q1 + q2 * q2 - coefficient * q1 * q2;
        return std::sqrt(std::max(power, 0.0)) / static_cast<double>(sampleCount);
    }

    SpectrumLevels analyzeWindow()
    {
        SpectrumLevels levels;
        levels.active = true;
        float strongestBand = 0.0F;
        const float* data = monoBuffer.data();

        for (int band = 0; band < SpectrumLevels::kBandCount; ++band) {
            const double center = kSpectrumBandCenters[band];
            const double lower = goertzelMagnitude(data, windowFrames, center * 0.85, sampleRate);
            const double middle = goertzelMagnitude(data, windowFrames, center, sampleRate);
            const double upper = goertzelMagnitude(data, windowFrames, center * 1.15, sampleRate);
            const double magnitude = (lower + middle + upper) / 3.0;
            const double db = 20.0 * std::log10(magnitude + 1.0e-7);
            const double normalized = std::clamp((db + kSpectrumDbFloor) / kSpectrumDbRange, 0.0, 1.0);
            const float target = static_cast<float>(std::pow(normalized, kSpectrumResponseGamma));
            float& smoothed = smoothedLevels[band];
            smoothed += (target - smoothed) * kSpectrumAverageBlend;
            strongestBand = std::max(strongestBand, smoothed);
        }

        const float targetReference = std::max(strongestBand, kSpectrumAdaptiveFloor);
        const float referenceBlend = targetReference >= adaptiveReference ? kSpectrumAdaptiveAttack : kSpectrumAdaptiveRelease;
        adaptiveReference += (targetReference - adaptiveReference) * referenceBlend;
        const float adaptiveGain = std::clamp(
            kSpectrumTargetFill / std::max(adaptiveReference, 0.001F),
            kSpectrumMinAdaptiveGain,
            kSpectrumMaxAdaptiveGain);

        for (int band = 0; band < SpectrumLevels::kBandCount; ++band) {
            levels.bands[band] = std::clamp(smoothedLevels[band] * adaptiveGain, 0.0F, 1.0F);
        }

        return levels;
    }
};

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

    mAnalyzerState = std::make_unique<AnalyzerState>();
    if (!setupAudioSinkBin()) {
        logMessage("ERROR", "Failed to create appsink analysis path");
        cleanupGStreamer();
        if (mOnError) {
            mOnError(ErrorCode::InitializationFailed, "Failed to create GStreamer appsink analysis path");
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

    if (mAudioSinkBin) {
        g_object_set(mPlaybin, "audio-sink", mAudioSinkBin, nullptr);
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

    if (mAudioSinkBin) {
        gst_object_unref(mAudioSinkBin);
        mAudioSinkBin = nullptr;
    }
    mAnalyzerSink = nullptr;
    gst_object_unref(mPlaybin);
    mPlaybin = nullptr;
    mState = PlaybackState::Stopped;
    mFileLoadedSignaled = false;
    mBufferingActive = false;
    mLastBufferingPercent = -1;
    if (mAnalyzerState) {
        mAnalyzerState->reset();
    }
    clearSpectrumLevels();
    resetPlaybackProgressTracking();
    logMessage("INFO", "GStreamer backend cleaned up");
}

void GstAudioEngine::resetForNewTrack() {
    mFileLoadedSignaled = false;
    mBufferingActive = false;
    mLastBufferingPercent = -1;
    if (mAnalyzerState) {
        mAnalyzerState->reset();
    }
    clearSpectrumLevels();
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
    clearSpectrumLevels();
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
    if (mAnalyzerState) {
        mAnalyzerState->reset();
    }
    clearSpectrumLevels();
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

    if (mAnalyzerState) {
        mAnalyzerState->reset();
    }
    clearSpectrumLevels();
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

void GstAudioEngine::clearSpectrumLevels()
{
    storeSpectrumLevels(SpectrumLevels{});
}

std::optional<SpectrumLevels> GstAudioEngine::takeLatestSpectrumLevels()
{
    std::lock_guard<std::mutex> lock(mSpectrumMutex);
    if (!mHasPendingSpectrumLevels) {
        return std::nullopt;
    }

    mHasPendingSpectrumLevels = false;
    return mLatestSpectrumLevels;
}

void GstAudioEngine::storeSpectrumLevels(const SpectrumLevels& levels)
{
    std::lock_guard<std::mutex> lock(mSpectrumMutex);
    mLatestSpectrumLevels = levels;
    mHasPendingSpectrumLevels = true;
}

bool GstAudioEngine::setupAudioSinkBin()
{
    GstElement* audioConvert = gst_element_factory_make("audioconvert", "boombox-audio-convert");
    GstElement* tee = gst_element_factory_make("tee", "boombox-audio-tee");
    GstElement* playbackQueue = gst_element_factory_make("queue", "boombox-playback-queue");
    GstElement* playbackConvert = gst_element_factory_make("audioconvert", "boombox-playback-convert");
    GstElement* playbackSink = gst_element_factory_make("autoaudiosink", "boombox-output-sink");
    GstElement* analyzerQueue = gst_element_factory_make("queue", "boombox-analyzer-queue");
    GstElement* analyzerConvert = gst_element_factory_make("audioconvert", "boombox-analyzer-convert");
    GstElement* analyzerResample = gst_element_factory_make("audioresample", "boombox-analyzer-resample");
    GstElement* analyzerCaps = gst_element_factory_make("capsfilter", "boombox-analyzer-caps");
    GstElement* analyzerSink = gst_element_factory_make("appsink", "boombox-analyzer-sink");

    if (!audioConvert || !tee || !playbackQueue || !playbackConvert || !playbackSink || !analyzerQueue ||
        !analyzerConvert || !analyzerResample || !analyzerCaps || !analyzerSink) {
        if (audioConvert) {
            gst_object_unref(audioConvert);
        }
        if (tee) {
            gst_object_unref(tee);
        }
        if (playbackQueue) {
            gst_object_unref(playbackQueue);
        }
        if (playbackConvert) {
            gst_object_unref(playbackConvert);
        }
        if (playbackSink) {
            gst_object_unref(playbackSink);
        }
        if (analyzerQueue) {
            gst_object_unref(analyzerQueue);
        }
        if (analyzerConvert) {
            gst_object_unref(analyzerConvert);
        }
        if (analyzerResample) {
            gst_object_unref(analyzerResample);
        }
        if (analyzerCaps) {
            gst_object_unref(analyzerCaps);
        }
        if (analyzerSink) {
            gst_object_unref(analyzerSink);
        }
        return false;
    }

    mAudioSinkBin = gst_bin_new("boombox-audio-bin");
    if (!mAudioSinkBin) {
        gst_object_unref(audioConvert);
        gst_object_unref(tee);
        gst_object_unref(playbackQueue);
        gst_object_unref(playbackConvert);
        gst_object_unref(playbackSink);
        gst_object_unref(analyzerQueue);
        gst_object_unref(analyzerConvert);
        gst_object_unref(analyzerResample);
        gst_object_unref(analyzerCaps);
        gst_object_unref(analyzerSink);
        return false;
    }

    g_object_set(playbackQueue,
                 "max-size-buffers", 0u,
                 "max-size-bytes", 0u,
                 "max-size-time", static_cast<guint64>(kPlaybackQueueLeadTime.count()) * GST_MSECOND,
                 nullptr);
    g_object_set(analyzerQueue,
                 "max-size-buffers", 8u,
                 "max-size-bytes", 0u,
                 "max-size-time", static_cast<guint64>(0),
                 "leaky", 2,
                 "flush-on-eos", TRUE,
                 nullptr);

    GstCaps* caps = gst_caps_from_string("audio/x-raw,format=F32LE,layout=interleaved,rate=44100");
    if (!caps) {
        gst_object_unref(mAudioSinkBin);
        mAudioSinkBin = nullptr;
        return false;
    }
    g_object_set(analyzerCaps, "caps", caps, nullptr);
    gst_caps_unref(caps);

    setBooleanPropertyIfPresent(analyzerSink, "emit-signals", TRUE);
    setBooleanPropertyIfPresent(analyzerSink, "sync", FALSE);
    setBooleanPropertyIfPresent(analyzerSink, "enable-last-sample", FALSE);
    setBooleanPropertyIfPresent(analyzerSink, "drop", TRUE);
    setUIntPropertyIfPresent(analyzerSink, "max-buffers", 1u);

    gst_bin_add_many(GST_BIN(mAudioSinkBin),
                     audioConvert,
                     tee,
                     playbackQueue,
                     playbackConvert,
                     playbackSink,
                     analyzerQueue,
                     analyzerConvert,
                     analyzerResample,
                     analyzerCaps,
                     analyzerSink,
                     nullptr);

    const bool linkedMain = gst_element_link_many(audioConvert, tee, nullptr);
    const bool linkedPlayback = gst_element_link_many(playbackQueue, playbackConvert, playbackSink, nullptr);
    const bool linkedAnalyzer =
        gst_element_link_many(analyzerQueue,
                              analyzerConvert,
                              analyzerResample,
                              analyzerCaps,
                              analyzerSink,
                              nullptr);
    if (!linkedMain || !linkedPlayback || !linkedAnalyzer) {
        gst_object_unref(mAudioSinkBin);
        mAudioSinkBin = nullptr;
        return false;
    }

    GstPad* playbackTeePad = gst_element_request_pad_simple(tee, "src_%u");
    GstPad* playbackSinkPad = gst_element_get_static_pad(playbackQueue, "sink");
    GstPad* analyzerTeePad = gst_element_request_pad_simple(tee, "src_%u");
    GstPad* analyzerSinkPad = gst_element_get_static_pad(analyzerQueue, "sink");
    const bool teePadsReady = playbackTeePad && playbackSinkPad && analyzerTeePad && analyzerSinkPad;
    const GstPadLinkReturn playbackLink =
        teePadsReady ? gst_pad_link(playbackTeePad, playbackSinkPad) : GST_PAD_LINK_REFUSED;
    const GstPadLinkReturn analyzerLink =
        teePadsReady ? gst_pad_link(analyzerTeePad, analyzerSinkPad) : GST_PAD_LINK_REFUSED;
    if (playbackSinkPad) {
        gst_object_unref(playbackSinkPad);
    }
    if (analyzerSinkPad) {
        gst_object_unref(analyzerSinkPad);
    }
    if (playbackTeePad) {
        gst_object_unref(playbackTeePad);
    }
    if (analyzerTeePad) {
        gst_object_unref(analyzerTeePad);
    }
    if (!teePadsReady || playbackLink != GST_PAD_LINK_OK || analyzerLink != GST_PAD_LINK_OK) {
        gst_object_unref(mAudioSinkBin);
        mAudioSinkBin = nullptr;
        return false;
    }

    GstPad* ghostTarget = gst_element_get_static_pad(audioConvert, "sink");
    GstPad* ghostPad = ghostTarget ? gst_ghost_pad_new("sink", ghostTarget) : nullptr;
    if (ghostTarget) {
        gst_object_unref(ghostTarget);
    }
    if (!ghostPad || !gst_pad_set_active(ghostPad, TRUE) || !gst_element_add_pad(mAudioSinkBin, ghostPad)) {
        if (ghostPad) {
            gst_object_unref(ghostPad);
        }
        gst_object_unref(mAudioSinkBin);
        mAudioSinkBin = nullptr;
        return false;
    }

    g_signal_connect(analyzerSink, "new-sample", G_CALLBACK(GstAudioEngine::handleAnalyzerNewSample), this);
    mAnalyzerSink = analyzerSink;
    return true;
}

GstFlowReturn GstAudioEngine::handleAnalyzerNewSample(GstElement* sink, gpointer userData)
{
    auto* self = static_cast<GstAudioEngine*>(userData);
    return self ? self->onAnalyzerNewSample(sink) : GST_FLOW_ERROR;
}

GstFlowReturn GstAudioEngine::onAnalyzerNewSample(GstElement* sink)
{
    if (!mAnalyzerState || !sink) {
        return GST_FLOW_OK;
    }

    GstSample* sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) {
        return GST_FLOW_OK;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!caps || !buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    const GstStructure* structure = gst_caps_get_structure(caps, 0);
    int rate = kAnalyzerSampleRate;
    int channels = 2;
    const gchar* format = structure ? gst_structure_get_string(structure, "format") : nullptr;
    if (structure) {
        gst_structure_get_int(structure, "rate", &rate);
        gst_structure_get_int(structure, "channels", &channels);
    }

    if (!format || std::string_view(format) != "F32LE") {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstMapInfo mapInfo{};
    if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    mAnalyzerState->updateFormat(rate);
    const std::size_t frameSize = sizeof(float) * static_cast<std::size_t>(std::max(channels, 1));
    if (frameSize == 0 || mapInfo.size < frameSize) {
        gst_buffer_unmap(buffer, &mapInfo);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    const auto* samples = reinterpret_cast<const float*>(mapInfo.data);
    const std::size_t frames = mapInfo.size / frameSize;
    const auto levels = mAnalyzerState->ingest(samples, frames, channels);

    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);

    if (levels) {
        storeSpectrumLevels(*levels);
    }
    return GST_FLOW_OK;
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
            if (mAnalyzerState) {
                mAnalyzerState->reset();
            }
            clearSpectrumLevels();
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
            if (mAnalyzerState) {
                mAnalyzerState->reset();
            }
            clearSpectrumLevels();
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
                        clearSpectrumLevels();
                        break;
                    default:
                        if (pendingState == GST_STATE_VOID_PENDING) {
                            mState = PlaybackState::Stopped;
                            clearSpectrumLevels();
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
