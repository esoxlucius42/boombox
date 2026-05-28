#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <locale.h>
#include <memory>
#include "audioengine.h"
#include "logger.h"
#include "filemanager.h"
#include "statemanager.h"
#include "playbackcontroller.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Audio backends expect C numeric locale for parsing floats/options.
    setlocale(LC_NUMERIC, "C");

    initializeAudioBackendRuntime();

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.jpg"));

    // Initialize logger after creating QApplication
    Logger::init();
    Logger::info("Main", "Logger initialized");
    Logger::debug("Main", "Debug message");
    Logger::warn("Main", "Warning message");

    // Initialize state manager to load saved state
    StateManager::init();
    Logger::info("Main", "StateManager initialized");

    // Some frameworks can reset locale during startup; enforce again before backend init.
    if (!setlocale(LC_NUMERIC, "C")) {
        Logger::warn("Main", "Failed to enforce LC_NUMERIC=C before audio engine initialization");
    }

    Logger::info("Main", QString("Selected audio backend: %1").arg(selectedAudioBackendName()));

    // Create PlaybackController using the required GStreamer playback backend.
    auto playbackController = std::make_unique<PlaybackController>();
    Logger::info("Main", "PlaybackController initialized");

    // Check for --fullscreen flag
    bool fullscreenMode = app.arguments().contains("--fullscreen");
    if (fullscreenMode) {
        Logger::info("Main", "Fullscreen mode enabled");
    }

    // ===== FileManager Test =====
    Logger::info("Main", "=== Testing FileManager ===");
    
    FileManager fm;
    
    // Create a temporary directory with test audio files
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        Logger::error("Main", "Failed to create temp directory");
        return 1;
    }
    
    // Create dummy audio files (just empty files with audio extensions)
    QStringList testFiles = { "song1.mp3", "song2.flac", "song3.wav", "track4.ogg" };
    for (const QString& fileName : testFiles) {
        QFile file(tempDir.path() + "/" + fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write("DUMMY AUDIO DATA");
            file.close();
            Logger::debug("Main", "Created test file: " + fileName);
        }
    }
    
    // Test loadFolder
    if (fm.loadFolder(tempDir.path())) {
        Logger::info("Main", "✓ Folder loaded successfully");
    } else {
        Logger::error("Main", "✗ Failed to load folder");
        return 1;
    }
    
    // Test track list operations
    Logger::info("Main", QString("Track count: %1").arg(fm.getTrackCount()));
    if (fm.getTrackCount() == 4) {
        Logger::info("Main", "✓ Correct track count");
    } else {
        Logger::error("Main", QString("✗ Incorrect track count (expected 4, got %1)").arg(fm.getTrackCount()));
        return 1;
    }
    
    // Test current track
    QString currentTrack = fm.getCurrentTrack();
    Logger::info("Main", "Current track: " + currentTrack);
    if (!currentTrack.isEmpty()) {
        Logger::info("Main", "✓ Current track retrieved");
    } else {
        Logger::error("Main", "✗ No current track");
        return 1;
    }
    
    // Test track position
    Logger::info("Main", QString("Current position: %1").arg(fm.getCurrentTrackPosition()));
    
    // Test metadata loading (lazy loading)
    AudioMetadata meta = fm.getMetadata(currentTrack);
    Logger::info("Main", "Metadata for current track:");
    Logger::info("Main", "  Title: " + meta.title);
    Logger::info("Main", "  Artist: " + meta.artist);
    Logger::info("Main", "  Album: " + meta.album);
    Logger::info("Main", QString("  Duration: %1 seconds").arg(meta.duration));
    
    // Test getTrackByPosition
    QString trackAt1 = fm.getTrackByPosition(1);
    Logger::info("Main", QString("Track at index 1: %1").arg(!trackAt1.isEmpty() ? "found" : "not found"));
    
    Logger::info("Main", "=== All FileManager tests completed ===");

    // Create and show the main window
    MainWindow window(playbackController.get(), fullscreenMode);
    window.show();

    // Auto-load the last selected folder if it still exists.
    const QString savedFolder = StateManager::getCurrentFolder();
    if (!savedFolder.isEmpty()) {
        const QFileInfo savedFolderInfo(savedFolder);
        if (savedFolderInfo.exists() && savedFolderInfo.isDir() && savedFolderInfo.isReadable()) {
            playbackController->loadFolder(savedFolder);
            Logger::info("Main", "Auto-loaded saved folder: " + savedFolder);
        } else {
            Logger::warn("Main", "Saved folder is unavailable and was not auto-loaded: " + savedFolder);
        }
    }

    // Save state when application exits
    QObject::connect(&app, &QApplication::aboutToQuit, [&playbackController]() {
        playbackController->shutdown();
        StateManager::save();
        Logger::debug("Main", "State saved on application exit");
    });

    return app.exec();
}
