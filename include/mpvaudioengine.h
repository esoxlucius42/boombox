#pragma once

#include <chrono>
#include <string>

#include "audioengine.h"
#include "mpv_client_compat.h"

class MpvAudioEngine final : public AudioEngine {
public:
    MpvAudioEngine();
    ~MpvAudioEngine() override;

    MpvAudioEngine(const MpvAudioEngine&) = delete;
    MpvAudioEngine& operator=(const MpvAudioEngine&) = delete;
    MpvAudioEngine(MpvAudioEngine&&) = delete;
    MpvAudioEngine& operator=(MpvAudioEngine&&) = delete;

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
    void initializeMpv();
    void cleanupMpv();
    int sendCommand(const char** args);
    void* getProperty(const char* property, const char* type);
    void setProperty(const char* property, const char* type, void* value);
    void handleEvent(const mpv_event* event);
    void samplePlaybackDiagnostics();
    void logPlaybackSnapshot(const char* reason);
    void resetPlaybackProgressTracking();
    static ErrorCode mapMpvError(int mpvError);

    mpv_handle* mHandle = nullptr;
    PlaybackState mState = PlaybackState::Stopped;
    TrackFinishedCallback mOnTrackFinished;
    FileLoadedCallback mOnFileLoaded;
    ErrorCallback mOnError;
    bool mHasLastObservedPause = false;
    bool mLastObservedPause = false;
    bool mHasLastObservedCoreIdle = false;
    bool mLastObservedCoreIdle = false;
    bool mHasLastObservedPausedForCache = false;
    bool mLastObservedPausedForCache = false;
    double mLastObservedCacheBufferingState = -1.0;
    double mLastObservedDemuxerCacheDuration = -1.0;
    std::string mLastObservedAudioDevice;
    bool mHasLastPlaybackPosition = false;
    double mLastPlaybackPosition = 0.0;
    bool mPlaybackStallLogged = false;
    std::chrono::steady_clock::time_point mLastPlaybackAdvanceAt{};
    std::chrono::steady_clock::time_point mLastDiagnosticSampleAt{};
};
