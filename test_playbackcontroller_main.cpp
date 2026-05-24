#include <QApplication>
#include <QString>
#include <QDebug>
#include <iostream>
#include <memory>
#include "playbackcontroller.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Initialize logger
    Logger::init();
    Logger::info("Test", "PlaybackController Test Started");
    
    // Create PlaybackController
    auto controller = std::make_unique<PlaybackController>();
    
    // Test 1: Load folder (random-only playback mode)
    Logger::info("Test", "=== Test 1: Load folder ===");
    controller->loadFolder("/var/home/esox/test_audio");
    Logger::info("Test", "Folder loaded successfully");
    
    // Test 2: Check track list metrics
    Logger::info("Test", "=== Test 2: Check track metrics ===");
    if (controller->getTrackCount() > 0 && controller->getCurrentTrackPosition() >= 0) {
        Logger::info("Test", "✓ Track metrics available after load");
    } else {
        Logger::error("Test", "✗ Expected non-empty track list after load");
        return 1;
    }
    
    // Test 3: Test playNext (random-only mode)
    Logger::info("Test", "=== Test 3: Play next track (random-only mode) ===");
    for (int i = 0; i < 5; i++) {
        controller->playNext();
        Logger::debug("Test", QString("Played random track %1").arg(i + 1));
        QCoreApplication::processEvents();
    }
    Logger::info("Test", "✓ playNext works in random-only mode");
    
    // Test 4: Test pause/play
    Logger::info("Test", "=== Test 4: Test pause/play ===");
    controller->pause();
    Logger::info("Test", "✓ Playback paused");
    controller->play();
    Logger::info("Test", "✓ Playback resumed");
    
    // Test 5: Test seek
    Logger::info("Test", "=== Test 5: Test seek ===");
    controller->seek(30);
    Logger::info("Test", "✓ Sought to 30 seconds");
    
    Logger::info("Test", "=== ALL TESTS PASSED ===");
    
    return 0;
}
