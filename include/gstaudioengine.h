#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <memory>
#include <optional>
#include <string>

#include <gst/gst.h>

#include "audioengine.h"
#include "spectrumlevels.h"

class GstAudioEngine final : public AudioEngine {
public:
    GstAudioEngine();
    ~GstAudioEngine() override;

    GstAudioEngine(const GstAudioEngine&) = delete;
    GstAudioEngine& operator=(const GstAudioEngine&) = delete;
    GstAudioEngine(GstAudioEngine&&) = delete;
    GstAudioEngine& operator=(GstAudioEngine&&) = delete;

    void play(const std::string& filePath) override;
    void pause() override;
    void resume() override;
    void stop() override;
    void next() override;
    void previous() override;
    void seek(double positionSeconds) override;
    void setVolume(int level) override;
    int getVolume() const override;
    double getCurrentPosition() const override;
    double getDuration() const override;
    bool isPlaying() const override;
    bool isInitialized() const override;
    PlaybackState getPlaybackState() const override;
    void setOnTrackFinished(TrackFinishedCallback callback) override;
    void setOnFileLoaded(FileLoadedCallback callback) override;
    void setOnError(ErrorCallback callback) override;
    std::optional<SpectrumLevels> takeLatestSpectrumLevels();
    void clearSpectrumLevels();
    void processEvents() override;

private:
    struct AnalyzerState;

    void initializeGStreamer();
    void cleanupGStreamer();
    void resetForNewTrack();
    void flushPendingMessages();
    void handleMessage(GstMessage* message);
    void samplePlaybackDiagnostics();
    void logPlaybackSnapshot(const char* reason) const;
    void signalFileLoadedIfReady(GstState newState);
    void resetPlaybackProgressTracking();
    void storeSpectrumLevels(const SpectrumLevels& levels);
    bool setupAudioSinkBin();
    static GstFlowReturn handleAnalyzerNewSample(GstElement* sink, gpointer userData);
    GstFlowReturn onAnalyzerNewSample(GstElement* sink);
    static ErrorCode mapGstError(const GError* error);

    GstElement* mPlaybin = nullptr;
    GstBus* mBus = nullptr;
    GstElement* mAudioSinkBin = nullptr;
    GstElement* mAnalyzerSink = nullptr;
    PlaybackState mState = PlaybackState::Stopped;
    TrackFinishedCallback mOnTrackFinished;
    FileLoadedCallback mOnFileLoaded;
    ErrorCallback mOnError;
    std::unique_ptr<AnalyzerState> mAnalyzerState;
    std::mutex mSpectrumMutex;
    SpectrumLevels mLatestSpectrumLevels{};
    bool mHasPendingSpectrumLevels = false;
    bool mFileLoadedSignaled = false;
    bool mPlaybackStallLogged = false;
    bool mHasLastPlaybackPosition = false;
    bool mBufferingActive = false;
    int mLastBufferingPercent = -1;
    double mLastPlaybackPosition = 0.0;
    std::chrono::steady_clock::time_point mLastPlaybackAdvanceAt{};
    std::chrono::steady_clock::time_point mLastDiagnosticSampleAt{};
};
