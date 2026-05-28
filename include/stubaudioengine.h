#pragma once

#include "audioengine.h"

class StubAudioEngine final : public AudioEngine {
public:
    StubAudioEngine() = default;
    ~StubAudioEngine() override = default;

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
    PlaybackState mState = PlaybackState::Stopped;
    TrackFinishedCallback mOnTrackFinished;
    FileLoadedCallback mOnFileLoaded;
    ErrorCallback mOnError;
};
