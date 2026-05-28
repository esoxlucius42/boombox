#pragma once

#include <chrono>
#include <string>

#include <gst/gst.h>

#include "audioengine.h"

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
    void processEvents() override;

private:
    void initializeGStreamer();
    void cleanupGStreamer();
    void resetForNewTrack();
    void flushPendingMessages();
    void handleMessage(GstMessage* message);
    void samplePlaybackDiagnostics();
    void logPlaybackSnapshot(const char* reason) const;
    void signalFileLoadedIfReady(GstState newState);
    void resetPlaybackProgressTracking();
    static ErrorCode mapGstError(const GError* error);

    GstElement* mPlaybin = nullptr;
    GstBus* mBus = nullptr;
    PlaybackState mState = PlaybackState::Stopped;
    TrackFinishedCallback mOnTrackFinished;
    FileLoadedCallback mOnFileLoaded;
    ErrorCallback mOnError;
    bool mFileLoadedSignaled = false;
    bool mPlaybackStallLogged = false;
    bool mHasLastPlaybackPosition = false;
    bool mBufferingActive = false;
    int mLastBufferingPercent = -1;
    double mLastPlaybackPosition = 0.0;
    std::chrono::steady_clock::time_point mLastPlaybackAdvanceAt{};
    std::chrono::steady_clock::time_point mLastDiagnosticSampleAt{};
};
