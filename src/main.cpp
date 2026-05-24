#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include "logger.h"
#include "filemanager.h"
#include "statemanager.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Initialize logger after creating QApplication
    Logger::init();
    Logger::info("Main", "Logger initialized");
    Logger::debug("Main", "Debug message");
    Logger::warn("Main", "Warning message");

    // Initialize state manager to load saved state
    StateManager::init();
    Logger::info("Main", "StateManager initialized");

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
    
    // Test queue operations
    Logger::info("Main", QString("Queue size: %1").arg(fm.getQueueSize()));
    if (fm.getQueueSize() == 4) {
        Logger::info("Main", "✓ Correct queue size");
    } else {
        Logger::error("Main", QString("✗ Incorrect queue size (expected 4, got %1)").arg(fm.getQueueSize()));
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
    
    // Test track indexing
    Logger::info("Main", QString("Current index: %1").arg(fm.getCurrentTrackIndex()));
    
    // Test next track
    QString nextTrack = fm.getNextTrack();
    Logger::info("Main", QString("Next track exists: %1").arg(!nextTrack.isEmpty() ? "yes" : "no"));
    
    // Test advance
    fm.advanceQueue();
    Logger::info("Main", QString("Advanced to index: %1").arg(fm.getCurrentTrackIndex()));
    
    // Test metadata loading (lazy loading)
    AudioMetadata meta = fm.getMetadata(currentTrack);
    Logger::info("Main", "Metadata for current track:");
    Logger::info("Main", "  Title: " + meta.title);
    Logger::info("Main", "  Artist: " + meta.artist);
    Logger::info("Main", "  Album: " + meta.album);
    Logger::info("Main", QString("  Duration: %1 seconds").arg(meta.duration));
    
    // Test getTrackAt
    QString trackAt1 = fm.getTrackAt(1);
    Logger::info("Main", QString("Track at index 1: %1").arg(!trackAt1.isEmpty() ? "found" : "not found"));
    
    // Test regression
    fm.regressQueue();
    Logger::info("Main", QString("Regressed to index: %1").arg(fm.getCurrentTrackIndex()));
    
    Logger::info("Main", "=== All FileManager tests completed ===");

    // Create and show the main window
    MainWindow window;
    window.show();

    // Save state when application exits
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        StateManager::save();
        Logger::debug("Main", "State saved on application exit");
    });

    return app.exec();
}
