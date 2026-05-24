#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>

struct AudioMetadata {
    QString artist = "Unknown Artist";
    QString album = "Unknown Album";
    QString title = "Unknown Title";
    int duration = 0;  // in seconds
};

class FileManager {
public:
    FileManager();
    ~FileManager() = default;

    bool loadFolder(const QString& folderPath);
    int getQueueSize() const;
    QString getCurrentTrack() const;
    int getCurrentTrackIndex() const;
    QString getNextTrack() const;
    void advanceQueue();
    void regressQueue();
    QString getTrackAt(int index) const;
    AudioMetadata getMetadata(const QString& filePath);

private:
    QStringList queue;
    int currentIndex = -1;
    QMap<QString, AudioMetadata> metadataCache;

    bool isSupportedAudioFormat(const QString& filePath) const;
    AudioMetadata extractMetadata(const QString& filePath);
    void loadFolderRecursive(class QDir dir);
};

#endif // FILEMANAGER_H
