#include "statemanager.h"
#include "logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

StateManager& StateManager::instance()
{
    static StateManager inst;
    return inst;
}

StateManager::StateManager()
{
}

void StateManager::init()
{
    instance().initInternal();
}

void StateManager::initInternal()
{
    if (initialized) {
        return;
    }

    Logger::debug("StateManager", "Initializing StateManager");
    loadInternal();
    initialized = true;
    Logger::debug("StateManager", "StateManager initialized");
}

void StateManager::loadInternal()
{
    try {
        QString configPath = getConfigPath();
        QFile configFile(configPath);

        if (!configFile.exists()) {
            Logger::debug("StateManager", "Config file does not exist, using defaults for first run");
            currentFolder = "";
            currentTrack = "";
            return;
        }

        if (!configFile.open(QIODevice::ReadOnly)) {
            Logger::warn("StateManager", "Failed to open config file: " + configPath);
            currentFolder = "";
            currentTrack = "";
            return;
        }

        QByteArray jsonData = configFile.readAll();
        configFile.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

        if (!doc.isObject()) {
            Logger::warn("StateManager", "Invalid JSON in config file: " + parseError.errorString());
            currentFolder = "";
            currentTrack = "";
            return;
        }

        QJsonObject obj = doc.object();
        currentFolder = obj.value("currentFolder").toString("");
        currentTrack = obj.value("currentTrack").toString("");

        Logger::debug("StateManager", "Loaded state - Folder: " + currentFolder + ", Track: " + currentTrack);
    } catch (const std::exception& e) {
        Logger::error("StateManager", QString("Exception in loadInternal: %1").arg(e.what()));
        currentFolder = "";
        currentTrack = "";
    } catch (...) {
        Logger::error("StateManager", "Unknown exception in loadInternal, using defaults");
        currentFolder = "";
        currentTrack = "";
    }
}

bool StateManager::ensureConfigDirectory() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.boombox";
    QDir dir(configDir);

    if (!dir.exists()) {
        if (!QDir().mkpath(configDir)) {
            Logger::error("StateManager", "Failed to create config directory: " + configDir);
            return false;
        }
        Logger::debug("StateManager", "Created config directory: " + configDir);
    }

    return true;
}

QString StateManager::getConfigPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.boombox/config.json";
}

void StateManager::setCurrentFolder(const QString& folderPath)
{
    instance().currentFolder = folderPath;
    Logger::debug("StateManager", "Set current folder: " + folderPath);
}

QString StateManager::getCurrentFolder()
{
    return instance().currentFolder;
}

void StateManager::setCurrentTrack(const QString& filename)
{
    instance().currentTrack = filename;
    Logger::debug("StateManager", "Set current track: " + filename);
}

QString StateManager::getCurrentTrack()
{
    return instance().currentTrack;
}

void StateManager::save()
{
    instance().saveInternal();
}

void StateManager::saveInternal() const
{
    try {
        if (!ensureConfigDirectory()) {
            Logger::error("StateManager", "Failed to ensure config directory exists");
            return;
        }

        QJsonObject obj;
        obj.insert("currentFolder", currentFolder);
        obj.insert("currentTrack", currentTrack);
        obj.insert("version", "1.0");

        QJsonDocument doc(obj);
        QByteArray jsonData = doc.toJson();

        QString configPath = getConfigPath();
        QFile configFile(configPath);

        if (!configFile.open(QIODevice::WriteOnly)) {
            Logger::error("StateManager", "Failed to open config file for writing: " + configPath);
            return;
        }

        qint64 written = configFile.write(jsonData);
        configFile.close();

        if (written < 0) {
            Logger::error("StateManager", "Failed to write config file: " + configPath);
            return;
        }

        Logger::debug("StateManager", "State saved to: " + configPath);
    } catch (const std::exception& e) {
        Logger::error("StateManager", QString("Exception in saveInternal: %1").arg(e.what()));
    } catch (...) {
        Logger::error("StateManager", "Unknown exception in saveInternal");
    }
}
