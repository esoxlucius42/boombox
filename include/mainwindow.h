#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <memory>
#include "filemanager.h"
#include "widgets/trackinfo.h"
#include "widgets/seekbar.h"
#include "widgets/controls.h"
#include "widgets/albumart.h"

class PlaybackController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(PlaybackController *controller, bool fullscreen = false, QWidget *parent = nullptr);

private slots:
    void onBrowseClicked();
    void onTrackChanged(const QString& filePath);
    void onTrackMetadataLoaded(const AudioMetadata& meta);
    void onPlaybackError(const QString& error);
    void onPlayPauseClicked();
    void onNextClicked();
    void onFullscreenClicked();
    void onSeekRequested(int positionSeconds);
    void onPlaybackUiTick();

private:
    void setupUI();
    void loadStylesheet();
    void connectSignals();

    TrackInfoWidget *trackInfoWidget;
    SeekBarWidget *seekBarWidget;
    ControlsWidget *controlsWidget;
    AlbumArtWidget *albumArtWidget;
    PlaybackController *playbackController;
    QString currentTrackPath;
    QTimer *playbackUiTimer = nullptr;
};

#endif // MAINWINDOW_H
