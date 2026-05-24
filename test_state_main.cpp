
#include <QCoreApplication>
#include "statemanager.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    Logger::init();
    
    // Test 1: Initialize and check first run
    StateManager::init();
    
    if (StateManager::getCurrentFolder().isEmpty()) {
        qInfo() << "✓ Test 1 passed: Current folder is empty on first run";
    } else {
        qWarning() << "✗ Test 1 failed: Current folder should be empty";
        return 1;
    }
    
    // Test 2: Set state and save
    StateManager::setCurrentFolder("/home/user/Music");
    StateManager::setCurrentTrack("song.mp3");
    StateManager::save();
    
    qInfo() << "✓ State saved";
    
    return 0;
}
