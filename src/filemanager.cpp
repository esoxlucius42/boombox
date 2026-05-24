#include "filemanager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <algorithm>

FileManager::FileManager() : currentIndex(-1) {}

bool FileManager::loadFolder(const QString& folderPath) {
    QDir dir(folderPath);
    
    if (!dir.exists()) {
        qWarning() << "Folder does not exist:" << folderPath;
        return false;
    }

    queue.clear();
    currentIndex = -1;
    metadataCache.clear();

    // Set up filters to get only files
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    
    // Recursively scan the folder for audio files
    QFileInfoList files = dir.entryInfoList();
    for (const QFileInfo& fileInfo : files) {
        if (isSupportedAudioFormat(fileInfo.absoluteFilePath())) {
            queue.append(fileInfo.absoluteFilePath());
        }
    }

    // For subdirectories, use recursive scan
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList subdirs = dir.entryList();
    for (const QString& subdir : subdirs) {
        QDir subDirectory(dir.absoluteFilePath(subdir));
        loadFolderRecursive(subDirectory);
    }

    // Sort the queue for consistent ordering
    std::sort(queue.begin(), queue.end());

    if (!queue.isEmpty()) {
        currentIndex = 0;
        return true;
    }
    
    qWarning() << "No audio files found in folder:" << folderPath;
    return false;
}

void FileManager::loadFolderRecursive(QDir dir) {
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList files = dir.entryInfoList();
    for (const QFileInfo& fileInfo : files) {
        if (isSupportedAudioFormat(fileInfo.absoluteFilePath())) {
            queue.append(fileInfo.absoluteFilePath());
        }
    }

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList subdirs = dir.entryList();
    for (const QString& subdir : subdirs) {
        loadFolderRecursive(dir.absoluteFilePath(subdir));
    }
}

bool FileManager::isSupportedAudioFormat(const QString& filePath) const {
    static const QStringList supportedFormats = {
        "mp3", "flac", "wav", "ogg", "m4a", "aac", "wma", "ape"
    };

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    return supportedFormats.contains(suffix);
}

int FileManager::getQueueSize() const {
    return queue.size();
}

QString FileManager::getCurrentTrack() const {
    if (currentIndex >= 0 && currentIndex < queue.size()) {
        return queue.at(currentIndex);
    }
    return QString();
}

int FileManager::getCurrentTrackIndex() const {
    return currentIndex;
}

QString FileManager::getNextTrack() const {
    if (currentIndex + 1 < queue.size()) {
        return queue.at(currentIndex + 1);
    }
    return QString();
}

void FileManager::advanceQueue() {
    if (currentIndex + 1 < queue.size()) {
        currentIndex++;
    } else if (!queue.isEmpty()) {
        // Loop back to beginning
        currentIndex = 0;
    }
}

void FileManager::regressQueue() {
    if (currentIndex > 0) {
        currentIndex--;
    } else if (!queue.isEmpty()) {
        // Loop to end
        currentIndex = queue.size() - 1;
    }
}

QString FileManager::getTrackAt(int index) const {
    if (index >= 0 && index < queue.size()) {
        return queue.at(index);
    }
    return QString();
}

AudioMetadata FileManager::getMetadata(const QString& filePath) {
    // Check cache first (lazy loading pattern)
    if (metadataCache.contains(filePath)) {
        return metadataCache.value(filePath);
    }

    // Extract metadata from file
    AudioMetadata metadata = extractMetadata(filePath);
    
    // Cache for future access
    metadataCache.insert(filePath, metadata);
    
    return metadata;
}

AudioMetadata FileManager::extractMetadata(const QString& filePath) {
    AudioMetadata metadata;

    QFileInfo fileInfo(filePath);
    
    // Use filename as title if we can't extract tags
    metadata.title = fileInfo.baseName();
    
    // Try to read ID3 tags for MP3 files
    if (fileInfo.suffix().toLower() == "mp3") {
        // Attempt to read basic ID3v2 tags
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray header = file.read(10);
            
            // Check for ID3v2 header (3 bytes: "ID3")
            if (header.size() >= 3 && header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
                // ID3v2 found, but proper parsing is complex
                // For now, use defaults with filename as title
                qDebug() << "ID3 tags found in:" << filePath << "but full parsing not implemented";
            }
            file.close();
        }
    }
    
    // For FLAC, WAV, OGG, and other formats with Vorbis comments
    // Basic support could be added here with a proper tag library
    // For now, return sensible defaults
    
    return metadata;
}
