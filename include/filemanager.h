#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <memory>

struct AudioMetadata {
    QString artist = "Unknown Artist";
    QString album = "Unknown Album";
    QString title = "Unknown Title";
    int duration = 0;  // in seconds
};

Q_DECLARE_METATYPE(AudioMetadata)

class FileManager {
public:
    FileManager();
    ~FileManager() = default;

    bool loadFolder(const QString& folderPath);
    int getTrackCount() const;
    QString getCurrentTrack() const;
    int getCurrentTrackPosition() const;
    bool setCurrentTrackPosition(int index);
    int indexOfTrack(const QString& filePath) const;
    QString getTrackByPosition(int index) const;
    AudioMetadata getMetadata(const QString& filePath);
    
    /**
     * @brief Mark a file as problematic (corrupted, unsupported, etc)
     * @param filePath Path to the problematic file
     */
    void markFileAsProblematic(const QString& filePath);
    
    /**
     * @brief Get the next playable track, skipping problematic files
     * @return Path to next playable track, or empty string if none found
     */
    QString getNextPlayableTrack(int startIndex);

private:
    QStringList tracks;
    int currentTrackPosition = -1;
    QMap<QString, AudioMetadata> metadataCache;
    QSet<QString> problematicFiles;  // Track files that have had errors

    bool isSupportedAudioFormat(const QString& filePath) const;
    AudioMetadata extractMetadata(const QString& filePath);
    void loadFolderRecursive(class QDir dir);
    
    /**
     * @brief Check if file exists and is readable
     */
    bool isFileReadable(const QString& filePath) const;
};

#endif // FILEMANAGER_H
