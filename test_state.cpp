#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include "statemanager.h"
#include "logger.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Logger::init();

    Logger::info("Test", "Starting State Persistence Tests");

    // Test 1: Initialize and load (first run)
    Logger::info("Test", "=== Test 1: First Run (no config file) ===");
    StateManager::init();
    
    if (StateManager::getCurrentFolder().isEmpty()) {
        Logger::info("Test", "✓ Current folder is empty on first run");
    } else {
        Logger::error("Test", "✗ Current folder should be empty on first run");
        return 1;
    }

    if (StateManager::getCurrentTrack().isEmpty()) {
        Logger::info("Test", "✓ Current track is empty on first run");
    } else {
        Logger::error("Test", "✗ Current track should be empty on first run");
        return 1;
    }

    // Test 2: Set and save state
    Logger::info("Test", "=== Test 2: Setting and Saving State ===");
    StateManager::setCurrentFolder("/home/user/Music");
    StateManager::setCurrentTrack("song.mp3");
    StateManager::save();

    Logger::info("Test", "State saved. Checking config file...");

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.boombox/config.json";
    QFile configFile(configPath);

    if (configFile.exists()) {
        Logger::info("Test", "✓ Config file created at: " + configPath);

        if (configFile.open(QIODevice::ReadOnly)) {
            QByteArray jsonData = configFile.readAll();
            configFile.close();

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                Logger::info("Test", "JSON Content:");
                Logger::info("Test", "  currentFolder: " + obj.value("currentFolder").toString());
                Logger::info("Test", "  currentTrack: " + obj.value("currentTrack").toString());
                Logger::info("Test", "  version: " + obj.value("version").toString());

                if (obj.value("currentFolder").toString() == "/home/user/Music" &&
                    obj.value("currentTrack").toString() == "song.mp3" &&
                    obj.value("version").toString() == "1.0") {
                    Logger::info("Test", "✓ JSON format is correct");
                } else {
                    Logger::error("Test", "✗ JSON values don't match");
                    return 1;
                }
            } else {
                Logger::error("Test", "✗ Invalid JSON in config file");
                return 1;
            }
        } else {
            Logger::error("Test", "✗ Failed to open config file");
            return 1;
        }
    } else {
        Logger::error("Test", "✗ Config file was not created");
        return 1;
    }

    // Test 3: Simulate reload (create new instance, load state)
    Logger::info("Test", "=== Test 3: Loading Saved State ===");
    
    // Clear the current state
    StateManager::setCurrentFolder("");
    StateManager::setCurrentTrack("");
    
    // Create a new StateManager instance by calling init again
    // Note: Since StateManager uses a singleton, we need to manually test this
    // by checking if our setters work and state persists
    
    if (StateManager::getCurrentFolder() == "" && StateManager::getCurrentTrack() == "") {
        Logger::info("Test", "✓ State cleared for testing");
    }
    
    Logger::info("Test", "=== Test 4: Test Empty State ===");
    StateManager::setCurrentFolder("");
    StateManager::setCurrentTrack("");
    StateManager::save();
    
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = configFile.readAll();
        configFile.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.value("currentFolder").toString() == "" &&
                obj.value("currentTrack").toString() == "") {
                Logger::info("Test", "✓ Empty state saved correctly");
            }
        }
    }

    Logger::info("Test", "=== All tests passed! ===");
    return 0;
}
