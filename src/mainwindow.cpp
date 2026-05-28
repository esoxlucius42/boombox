#include "mainwindow.h"
#include "playbackcontroller.h"
#include "statemanager.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFile>
#include <QApplication>
#include <QScreen>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include "logger.h"

MainWindow::MainWindow(PlaybackController *controller, bool fullscreen, QWidget *parent)
    : QMainWindow(parent), playbackController(controller)
{
    setWindowTitle("Boombox");
    
    // Set window size to 800x480 (landscape, touchscreen display)
    resize(800, 480);
    
    // Enable fullscreen mode if requested (for RPi deployment)
    if (fullscreen) {
        setWindowState(Qt::WindowFullScreen);
    }
    
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Setup UI
    setupUI();
    
    // Connect signals between UI and controller
    connectSignals();
    
    // Load stylesheet
    loadStylesheet();
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = this->centralWidget();
    
    // Main horizontal layout: Left pane | Right pane
    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // ===== LEFT PANE (320x480) =====
    QWidget *leftPane = new QWidget(this);
    leftPane->setObjectName("leftPane");
    leftPane->setMinimumWidth(320);
    leftPane->setMaximumWidth(320);
    
    auto leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    // Analyzer strip at top of left pane
    spectrumAnalyzerWidget = new SpectrumAnalyzerWidget(this);
    spectrumAnalyzerWidget->setObjectName("spectrumAnalyzer");
    leftLayout->addWidget(spectrumAnalyzerWidget);
    leftLayout->addSpacing(2);
     
    // Pane 0: Track Info (80px)
    trackInfoWidget = new TrackInfoWidget(this);
    trackInfoWidget->setObjectName("pane0");
    trackInfoWidget->setMinimumHeight(150);
    trackInfoWidget->setMaximumHeight(170);
    leftLayout->addWidget(trackInfoWidget);
     
    // Pane 1: Seek Bar (100px)
    seekBarWidget = new SeekBarWidget(this);
    seekBarWidget->setObjectName("pane1");
    seekBarWidget->setMinimumHeight(56);
    seekBarWidget->setMaximumHeight(72);
    leftLayout->addWidget(seekBarWidget);
    
    // Pane 2: Control Buttons (300px)
    controlsWidget = new ControlsWidget(this);
    controlsWidget->setObjectName("pane2");
    leftLayout->addWidget(controlsWidget, 1);
    
    mainLayout->addWidget(leftPane);
    
    // ===== RIGHT PANE (480x480) =====
    QWidget *rightPane = new QWidget(this);
    rightPane->setObjectName("rightPane");
    rightPane->setMinimumSize(480, 480);
    rightPane->setMaximumSize(480, 480);
    
    auto rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    
    // Album art widget
    albumArtWidget = new AlbumArtWidget(this);
    albumArtWidget->setObjectName("albumArt");
    
    rightLayout->addWidget(albumArtWidget);
    
    mainLayout->addWidget(rightPane);
    
    trackInfoWidget->clearDisplay();
    spectrumAnalyzerWidget->clear();
    seekBarWidget->setCurrentTime(0);
    seekBarWidget->setTotalTime(0);
    seekBarWidget->setDuration(0);
    seekBarWidget->enableSeeking(false);
}

void MainWindow::connectSignals()
{
    connect(controlsWidget, &ControlsWidget::browseClicked,
            this, &MainWindow::onBrowseClicked);
    connect(controlsWidget, &ControlsWidget::fullscreenClicked,
            this, &MainWindow::onFullscreenClicked);

    if (!playbackController) {
        Logger::warn("MainWindow", "PlaybackController is null, cannot connect signals");
        return;
    }

    // Connect track changed to extract album art
    connect(playbackController, &PlaybackController::trackChanged,
            this, &MainWindow::onTrackChanged);
    
    // Connect track metadata loaded
    connect(playbackController, &PlaybackController::trackMetadataLoaded,
            this, &MainWindow::onTrackMetadataLoaded);
    
    // Connect playback error
    connect(playbackController, &PlaybackController::playbackError,
            this, &MainWindow::onPlaybackError);

    connect(playbackController, &PlaybackController::spectrumLevelsUpdated,
            spectrumAnalyzerWidget, &SpectrumAnalyzerWidget::setSpectrumLevels);
    
    // Connect play/pause button
    connect(controlsWidget, &ControlsWidget::playPauseClicked,
            this, &MainWindow::onPlayPauseClicked);
    
    // Connect transport controls
    connect(controlsWidget, &ControlsWidget::nextClicked,
            this, &MainWindow::onNextClicked);

    // Connect seek widget
    connect(seekBarWidget, &SeekBarWidget::userSeeked,
            this, &MainWindow::onSeekRequested);

    // UI timer for live playback position updates
    playbackUiTimer = new QTimer(this);
    playbackUiTimer->setInterval(250);
    connect(playbackUiTimer, &QTimer::timeout, this, &MainWindow::onPlaybackUiTick);
    playbackUiTimer->start();
    
    // Initialize UI state from controller
    controlsWidget->setPlayButtonState(playbackController->isPlaying());
}

void MainWindow::onBrowseClicked()
{
    Logger::debug("MainWindow", "Browse button clicked");
    
    // Get the last folder location from StateManager
    QString lastFolder = StateManager::getCurrentFolder();
    if (lastFolder.isEmpty()) {
        lastFolder = QDir::homePath();
    }
    
    // Open folder selection dialog
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this,
        "Select Audio Folder",
        lastFolder,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    // If user cancelled the dialog, return
    if (selectedFolder.isEmpty()) {
        Logger::debug("MainWindow", "Browse dialog cancelled");
        return;
    }
    
    Logger::info("MainWindow", QString("Selected folder: %1").arg(selectedFolder));
    
    // Save the folder path to state manager
    StateManager::setCurrentFolder(selectedFolder);
    StateManager::save();
    
    // Load the folder through the playback controller
    // Errors will be handled through onPlaybackError signal
    playbackController->loadFolder(selectedFolder);
}

void MainWindow::onTrackChanged(const QString& filePath)
{
    Logger::debug("MainWindow", QString("Track changed: %1").arg(filePath));
    
    // Store current track path for album art extraction
    currentTrackPath = filePath;
    albumArtWidget->setPlaceholder();
    QTimer::singleShot(0, this, [this, filePath]() {
        if (currentTrackPath != filePath) {
            return;
        }
        albumArtWidget->extractAndDisplayAlbumArt(filePath);
    });

    const int trackPosition = playbackController ? playbackController->getCurrentTrackPosition() : -1;
    const int trackCount = playbackController ? playbackController->getTrackCount() : 0;
    if (trackPosition >= 0 && trackCount > 0) {
        trackInfoWidget->setTrackNumber(trackPosition + 1, trackCount);
    }
}

void MainWindow::onTrackMetadataLoaded(const AudioMetadata& meta)
{
    Logger::debug("MainWindow", QString("Track metadata loaded: %1 by %2").arg(meta.title).arg(meta.artist));
    
    // Update track info widget
    trackInfoWidget->setTrackName(meta.title);
    trackInfoWidget->setArtistName(meta.artist);
    trackInfoWidget->setAlbumName(meta.album);
    
    // Update seek bar with duration
    if (meta.duration > 0) {
        seekBarWidget->setDuration(meta.duration);
        seekBarWidget->enableSeeking(true);
    }

    const int trackPosition = playbackController ? playbackController->getCurrentTrackPosition() : -1;
    const int trackCount = playbackController ? playbackController->getTrackCount() : 0;
    if (trackPosition >= 0 && trackCount > 0) {
        trackInfoWidget->setTrackNumber(trackPosition + 1, trackCount);
    }
}

void MainWindow::onPlaybackError(const QString& error)
{
    Logger::error("MainWindow", QString("Playback error: %1").arg(error));
    
    // Show error message to user
    QMessageBox::warning(
        this,
        "Playback Error",
        error,
        QMessageBox::Ok
    );
}

void MainWindow::onPlayPauseClicked()
{
    Logger::debug("MainWindow", "Play/Pause button clicked");
    
    if (!playbackController) {
        return;
    }
    
    if (playbackController->isPlaying()) {
        playbackController->pause();
        controlsWidget->setPlayButtonState(false);
    } else {
        // Check if any folder is loaded before playing
        if (playbackController->isPlaying() || !StateManager::getCurrentFolder().isEmpty()) {
            playbackController->play();
            controlsWidget->setPlayButtonState(true);
        } else {
            QMessageBox::information(this, "No Folder Selected", 
                                   "Please select a folder first using the Browse button.");
        }
    }
}

void MainWindow::onNextClicked()
{
    if (!playbackController) {
        return;
    }

    playbackController->playNext();
    controlsWidget->setPlayButtonState(playbackController->isPlaying());
}

void MainWindow::onFullscreenClicked()
{
    const bool enableFullscreen = !isFullScreen();
    if (enableFullscreen) {
        showFullScreen();
    } else {
        showNormal();
        resize(800, 480);
    }

    Logger::info("MainWindow",
                 enableFullscreen ? "Fullscreen enabled from controls"
                                  : "Fullscreen disabled from controls");
}

void MainWindow::onSeekRequested(int positionSeconds)
{
    if (!playbackController) {
        return;
    }

    playbackController->seek(positionSeconds);
}

void MainWindow::onPlaybackUiTick()
{
    if (!playbackController) {
        return;
    }

    const int duration = static_cast<int>(playbackController->getDuration());
    const int position = static_cast<int>(playbackController->getCurrentPosition());

    if (duration > 0) {
        seekBarWidget->setDuration(duration);
        seekBarWidget->enableSeeking(true);
    }

    seekBarWidget->updatePosition(position);
    controlsWidget->setPlayButtonState(playbackController->isPlaying());
}

void MainWindow::loadStylesheet()
{
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(style);
        styleFile.close();
    }
}
