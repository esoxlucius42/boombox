#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>
#include "filemanager.h"

class QThread;
class PlaybackWorker;

/**
 * @brief PlaybackController - Main-thread bridge for playback state and commands
 *
 * The actual playback engine runs in a dedicated worker thread. This controller
 * stays on the UI thread, forwards commands to the worker via queued signals,
 * caches worker-pushed state, and re-emits UI-facing signals.
 */
class PlaybackController : public QObject {
    Q_OBJECT

public:
    explicit PlaybackController(QObject *parent = nullptr);
    ~PlaybackController();

    PlaybackController(const PlaybackController&) = delete;
    PlaybackController& operator=(const PlaybackController&) = delete;

    void loadFolder(const QString& folderPath);
    void playNext();
    void seek(int position);
    bool isPlaying() const;
    void play();
    void pause();
    double getCurrentPosition() const;
    double getDuration() const;
    int getCurrentTrackPosition() const;
    int getTrackCount() const;
    void shutdown();

signals:
    void trackChanged(const QString& filePath);
    void trackMetadataLoaded(const AudioMetadata& meta);
    void playbackError(const QString& error);
    void spectrumLevelsUpdated(const QVector<float>& levels);

    void requestLoadFolder(const QString& folderPath);
    void requestPlayNext();
    void requestSeek(int position);
    void requestPlay();
    void requestPause();
    void requestShutdown();

private slots:
    void onWorkerTrackChanged(const QString& filePath, int position, int trackCount);
    void onWorkerTrackMetadataLoaded(const AudioMetadata& meta);
    void onWorkerPlaybackError(const QString& error);
    void onWorkerSpectrumLevelsUpdated(const QVector<float>& levels);
    void onWorkerPlaybackSnapshot(bool playing, double position, double duration);
    void onWorkerThreadFinished();

private:
    QThread* workerThread = nullptr;
    PlaybackWorker* worker = nullptr;
    bool shutdownRequested = false;
    bool playing = false;
    double currentPosition = 0.0;
    double duration = 0.0;
    int currentTrackPosition = -1;
    int trackCount = 0;
};

#endif // PLAYBACKCONTROLLER_H
