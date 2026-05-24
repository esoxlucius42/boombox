#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <memory>
#include <random>
#include "audioengine.h"
#include "filemanager.h"

/**
 * @brief PlaybackController - Coordinates AudioEngine and FileManager with randomness
 * 
 * Manages playback state and implements true random playback.
 * Emits signals for UI updates when track changes or errors occur.
 */
class PlaybackController : public QObject {
    Q_OBJECT

public:
    explicit PlaybackController(QObject *parent = nullptr);
    ~PlaybackController();

    // Prevent copying
    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;

    /**
     * @brief Load a folder and start playback with first random track
     * @param folderPath Path to folder containing audio files
     */
    void loadFolder(const QString& folderPath);

    /**
     * @brief Play next track (random-only mode)
     */
    void playNext();

    /**
     * @brief Seek to position in current track
     * @param position Position in seconds
     */
    void seek(int position);

    /**
     * @brief Check if currently playing
     * @return true if playing
     */
    bool isPlaying() const;

    /**
     * @brief Start playing a loaded folder (used after pause)
     */
    void play();

    /**
     * @brief Pause playback
     */
    void pause();

    /**
     * @brief Get current playback position in seconds
     */
    double getCurrentPosition() const;

    /**
     * @brief Get current track duration in seconds
     */
    double getDuration() const;

    int getCurrentTrackPosition() const;
    int getTrackCount() const;

signals:
    /**
     * @brief Emitted when track changes
     * @param filePath Path to the new track
     */
    void trackChanged(const QString& filePath);

    /**
     * @brief Emitted when track metadata is loaded
     * @param meta Metadata for the track
     */
    void trackMetadataLoaded(const AudioMetadata& meta);

    /**
     * @brief Emitted on playback error
     * @param error Error message
     */
    void playbackError(const QString& error);
    void spectrumLevelsUpdated(const QVector<float>& levels);

private slots:
    /**
     * @brief Called periodically to process audio backend events
     */
    void onAudioEventTick();

    /**
     * @brief Called when current track finishes
     */
    void onTrackFinished();

    /**
     * @brief Called on playback error
     * @param errorCode Error code from AudioEngine
     * @param errorMsg Error message
     */
    void onPlaybackError(AudioEngine::ErrorCode errorCode, const std::string& errorMsg);

private:
    static constexpr const char* BACKEND_UNAVAILABLE_MESSAGE =
        "Audio backend unavailable. Folder loaded, but playback cannot start.";

    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<FileManager> fileManager;
    
    std::mt19937 randomGenerator;  // Mersenne Twister for true random
    bool backendUnavailableErrorShown = false;
    QTimer* audioEventTimer = nullptr;
    static constexpr int FIXED_VOLUME_LEVEL = 100;
    static constexpr int SPECTRUM_BIN_COUNT = 24;
    QVector<float> smoothedSpectrumBins;

    /**
     * @brief Pick a random track from queue
     * @return Index of random track, or -1 if queue is empty
     */
    int pickRandomTrack();

    /**
     * @brief Start playing track at given index
     * @param index Index in queue
     */
    void playTrackAt(int index);
    QVector<float> createSpectrumBins(float baseLevel, bool playing);

    /**
     * @brief Check if backend is currently available for playback
     */
    bool isBackendAvailable() const;

    /**
     * @brief Emit backend unavailable error at most once
     */
    void emitBackendUnavailableErrorOnce();
};

#endif // PLAYBACKCONTROLLER_H
