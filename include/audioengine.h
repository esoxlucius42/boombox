#pragma once

#include <string>
#include <functional>
#include <memory>
#include <mpv/client.h>

/**
 * @brief AudioEngine - Wraps libmpv C API for audio playback
 * 
 * Provides a simple interface for audio playback control with event callbacks.
 * Manages the lifetime of the mpv context and handles cleanup.
 */
class AudioEngine {
public:
    /**
     * @brief Error codes for playback errors
     */
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

    /**
     * @brief Playback state
     */
    enum class PlaybackState {
        Stopped = 0,
        Playing = 1,
        Paused = 2
    };

    // Callback types
    using TrackFinishedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(ErrorCode, const std::string&)>;

    /**
     * @brief Construct a new AudioEngine object
     */
    AudioEngine();

    /**
     * @brief Destroy the AudioEngine object
     * Cleans up mpv context and releases all resources
     */
    ~AudioEngine();

    // Prevent copying
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Allow moving
    AudioEngine(AudioEngine&&) noexcept;
    AudioEngine& operator=(AudioEngine&&) noexcept;

    /**
     * @brief Start playing a file
     * @param filePath Path to audio file to play
     */
    void play(const std::string& filePath);

    /**
     * @brief Pause playback
     */
    void pause();

    /**
     * @brief Resume from pause
     */
    void resume();

    /**
     * @brief Stop playback and cleanup
     */
    void stop();

    /**
     * @brief Skip to next track
     * Triggers onTrackFinished callback
     */
    void next();

    /**
     * @brief Go to previous track
     */
    void previous();

    /**
     * @brief Seek to position in playback
     * @param positionSeconds Position in seconds
     */
    void seek(double positionSeconds);

    /**
     * @brief Set playback volume
     * @param level Volume level 0-100
     */
    void setVolume(int level);

    /**
     * @brief Get current volume level
     * @return Volume level 0-100
     */
    int getVolume() const;

    /**
     * @brief Get current playback position
     * @return Position in seconds
     */
    double getCurrentPosition() const;

    /**
     * @brief Get duration of current track
     * @return Duration in seconds
     */
    double getDuration() const;

    /**
     * @brief Get current reactive level derived from backend audio analysis
     * @return Normalized level in [0.0, 1.0]
     */
    double getReactiveLevel() const;

    /**
     * @brief Check if currently playing
     * @return true if playing, false otherwise
     */
    bool isPlaying() const;

    /**
     * @brief Check if audio backend is initialized and usable
     * @return true if mpv handle is available
     */
    bool isInitialized() const;

    /**
     * @brief Get current playback state
     * @return Current PlaybackState
     */
    PlaybackState getPlaybackState() const;

    /**
     * @brief Set callback for track finished event
     * @param callback Function to call when track finishes
     */
    void setOnTrackFinished(TrackFinishedCallback callback);

    /**
     * @brief Set callback for error event
     * @param callback Function to call on playback error
     */
    void setOnError(ErrorCallback callback);

    /**
     * @brief Process pending events from mpv
     * Should be called periodically (e.g., from Qt event loop)
     */
    void processEvents();

private:
    mpv_handle* mHandle;
    PlaybackState mState;
    TrackFinishedCallback mOnTrackFinished;
    ErrorCallback mOnError;

    /**
     * @brief Initialize the mpv context
     */
    void initializeMpv();

    /**
     * @brief Cleanup the mpv context
     */
    void cleanupMpv();

    /**
     * @brief Send a command to mpv
     * @param args Command arguments (null-terminated array of strings)
     */
    int sendCommand(const char** args);

    /**
     * @brief Get a property from mpv
     * @param property Property name
     * @param type Property type format
     * @return Property value (caller must cast appropriately)
     */
    void* getProperty(const char* property, const char* type);

    /**
     * @brief Set a property in mpv
     * @param property Property name
     * @param type Property type format
     * @param value Property value
     */
    void setProperty(const char* property, const char* type, void* value);

    /**
     * @brief Handle mpv events
     * @param event Event from mpv
     */
    void handleEvent(const mpv_event* event);

    /**
     * @brief Convert mpv error code to our ErrorCode enum
     * @param mpvError MPV error code
     * @return Corresponding ErrorCode
     */
    static ErrorCode mapMpvError(int mpvError);
};
