#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <QString>
#include <QJsonObject>
#include <memory>

class StateManager {
public:
    // Initialize state manager and load existing config if available
    static void init();

    // Set and get current folder path
    static void setCurrentFolder(const QString& folderPath);
    static QString getCurrentFolder();

    // Set and get current track filename
    static void setCurrentTrack(const QString& filename);
    static QString getCurrentTrack();

    // Save state to disk
    static void save();

private:
    StateManager();
    ~StateManager() = default;

    static StateManager& instance();

    void initInternal();
    void loadInternal();
    void saveInternal() const;
    bool ensureConfigDirectory() const;
    QString getConfigPath() const;

    QString currentFolder;
    QString currentTrack;
    bool initialized = false;

    // Prevent copying
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
};

#endif // STATEMANAGER_H
