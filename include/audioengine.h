#pragma once

#include <functional>
#include <memory>
#include <string>

/**
 * @brief AudioEngine - backend-agnostic audio playback contract
 *
 * The concrete implementation is GStreamer-based.
 */
class AudioEngine {
public:
    enum class ErrorCode {
        NoError = 0,
        InitializationFailed = 1,
        FileNotFound = 2,
        CorruptedFile = 3,
        UnsupportedCodec = 4,
        PlaybackFailed = 5,
        DeviceError = 6,
        UnknownError = 7
    };

    enum class PlaybackState {
        Stopped = 0,
        Playing = 1,
        Paused = 2
    };

    using TrackFinishedCallback = std::function<void()>;
    using FileLoadedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;

    virtual ~AudioEngine() = default;

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    virtual void play(const std::string& filePath) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
    virtual void next() = 0;
    virtual void previous() = 0;
    virtual void seek(double positionSeconds) = 0;
    virtual void setVolume(int level) = 0;
    virtual int getVolume() const = 0;
    virtual double getCurrentPosition() const = 0;
    virtual double getDuration() const = 0;
    virtual bool isPlaying() const = 0;
    virtual bool isInitialized() const = 0;
    virtual PlaybackState getPlaybackState() const = 0;
    virtual void setOnTrackFinished(TrackFinishedCallback callback) = 0;
    virtual void setOnFileLoaded(FileLoadedCallback callback) = 0;
    virtual void setOnError(ErrorCallback callback) = 0;
    virtual void processEvents() = 0;

protected:
    AudioEngine() = default;
};

std::unique_ptr<AudioEngine> createAudioEngine();
void initializeAudioBackendRuntime();
const char* selectedAudioBackendName();
