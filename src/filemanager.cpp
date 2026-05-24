#include "filemanager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <algorithm>

namespace {

QString trimTagText(const QString& value) {
    return value.trimmed().remove(QChar('\0'));
}

QString decodeId3Text(const QByteArray& raw, quint8 encoding) {
    if (raw.isEmpty()) {
        return QString();
    }

    QByteArray payload = raw;
    while (!payload.isEmpty() && payload.endsWith('\0')) {
        payload.chop(1);
    }

    switch (encoding) {
        case 0:  // ISO-8859-1
            return trimTagText(QString::fromLatin1(payload));
        case 3:  // UTF-8
            return trimTagText(QString::fromUtf8(payload));
        default:
            return trimTagText(QString::fromUtf8(payload));
    }
}

quint32 readSynchsafeInt(const QByteArray& bytes) {
    if (bytes.size() < 4) {
        return 0;
    }
    return (static_cast<quint32>(static_cast<unsigned char>(bytes[0])) << 21) |
           (static_cast<quint32>(static_cast<unsigned char>(bytes[1])) << 14) |
           (static_cast<quint32>(static_cast<unsigned char>(bytes[2])) << 7) |
           static_cast<quint32>(static_cast<unsigned char>(bytes[3]));
}

quint32 readBigEndianInt(const QByteArray& bytes) {
    if (bytes.size() < 4) {
        return 0;
    }
    return (static_cast<quint32>(static_cast<unsigned char>(bytes[0])) << 24) |
           (static_cast<quint32>(static_cast<unsigned char>(bytes[1])) << 16) |
           (static_cast<quint32>(static_cast<unsigned char>(bytes[2])) << 8) |
           static_cast<quint32>(static_cast<unsigned char>(bytes[3]));
}

}

FileManager::FileManager() : currentTrackPosition(-1) {}

bool FileManager::loadFolder(const QString& folderPath) {
    QDir dir(folderPath);
    
    if (!dir.exists()) {
        qWarning() << "Folder does not exist:" << folderPath;
        return false;
    }

    tracks.clear();
    currentTrackPosition = -1;
    metadataCache.clear();

    // Set up filters to get only files
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    
    // Recursively scan the folder for audio files
    QFileInfoList files = dir.entryInfoList();
    for (const QFileInfo& fileInfo : files) {
        if (isSupportedAudioFormat(fileInfo.absoluteFilePath())) {
            tracks.append(fileInfo.absoluteFilePath());
        }
    }

    // For subdirectories, use recursive scan
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList subdirs = dir.entryList();
    for (const QString& subdir : subdirs) {
        QDir subDirectory(dir.absoluteFilePath(subdir));
        loadFolderRecursive(subDirectory);
    }

    // Sort the track list for consistent ordering
    std::sort(tracks.begin(), tracks.end());

    if (!tracks.isEmpty()) {
        currentTrackPosition = 0;
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
            tracks.append(fileInfo.absoluteFilePath());
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

int FileManager::getTrackCount() const {
    return tracks.size();
}

QString FileManager::getCurrentTrack() const {
    if (currentTrackPosition >= 0 && currentTrackPosition < tracks.size()) {
        return tracks.at(currentTrackPosition);
    }
    return QString();
}

int FileManager::getCurrentTrackPosition() const {
    return currentTrackPosition;
}

bool FileManager::setCurrentTrackPosition(int index) {
    if (index < 0 || index >= tracks.size()) {
        return false;
    }

    currentTrackPosition = index;
    return true;
}

int FileManager::indexOfTrack(const QString& filePath) const {
    return tracks.indexOf(filePath);
}

QString FileManager::getTrackByPosition(int index) const {
    if (index >= 0 && index < tracks.size()) {
        return tracks.at(index);
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

    try {
        QFileInfo fileInfo(filePath);

        // Defaults derived from path structure.
        metadata.title = fileInfo.baseName();
        const QDir albumDir = fileInfo.dir();
        const QString albumFromPath = albumDir.dirName();
        const QString artistFromPath = QDir(albumDir.absolutePath() + "/..").dirName();

        if (!albumFromPath.isEmpty() && albumFromPath != ".") {
            metadata.album = albumFromPath;
        }
        if (!artistFromPath.isEmpty() && artistFromPath != "." && artistFromPath != "..") {
            metadata.artist = artistFromPath;
        }

        // Try to read embedded ID3 tags for MP3 files.
        if (fileInfo.suffix().toLower() == "mp3") {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Failed to open file for metadata extraction:" << filePath;
                return metadata;
            }

            const QByteArray header = file.read(10);
            if (header.size() == 10 && header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
                const int version = static_cast<unsigned char>(header[3]);
                const quint32 tagSize = readSynchsafeInt(header.mid(6, 4));
                const QByteArray tagData = file.read(static_cast<qint64>(tagSize));
                int offset = 0;

                while (offset + 10 <= tagData.size()) {
                    const QByteArray frameId = tagData.mid(offset, 4);
                    if (frameId == QByteArray("\0\0\0\0", 4)) {
                        break;
                    }

                    const QByteArray sizeBytes = tagData.mid(offset + 4, 4);
                    const quint32 frameSize =
                        (version == 4) ? readSynchsafeInt(sizeBytes) : readBigEndianInt(sizeBytes);

                    if (frameSize == 0 || offset + 10 + static_cast<int>(frameSize) > tagData.size()) {
                        break;
                    }

                    const QByteArray frameData = tagData.mid(offset + 10, static_cast<int>(frameSize));
                    if (!frameData.isEmpty()) {
                        const quint8 encoding = static_cast<quint8>(frameData[0]);
                        const QString value = decodeId3Text(frameData.mid(1), encoding);
                        if (frameId == "TIT2" && !value.isEmpty()) {
                            metadata.title = value;
                        } else if (frameId == "TPE1" && !value.isEmpty()) {
                            metadata.artist = value;
                        } else if (frameId == "TALB" && !value.isEmpty()) {
                            metadata.album = value;
                        }
                    }

                    offset += 10 + static_cast<int>(frameSize);
                }
            }

            // Fallback to ID3v1 for older files/missing ID3v2 values.
            if (file.size() >= 128) {
                file.seek(file.size() - 128);
                const QByteArray id3v1 = file.read(128);
                if (id3v1.size() == 128 && id3v1.left(3) == "TAG") {
                    auto parseField = [](const QByteArray& field) {
                        return trimTagText(QString::fromLatin1(field));
                    };

                    const QString title = parseField(id3v1.mid(3, 30));
                    const QString artist = parseField(id3v1.mid(33, 30));
                    const QString album = parseField(id3v1.mid(63, 30));

                    if (!title.isEmpty()) {
                        metadata.title = title;
                    }
                    if (!artist.isEmpty()) {
                        metadata.artist = artist;
                    }
                    if (!album.isEmpty()) {
                        metadata.album = album;
                    }
                }
            }
            file.close();
        }
    } catch (...) {
        qWarning() << "Exception in extractMetadata for:" << filePath;
    }

    return metadata;
}

void FileManager::markFileAsProblematic(const QString& filePath) {
    problematicFiles.insert(filePath);
    qWarning() << "Marked file as problematic:" << filePath;
}

QString FileManager::getNextPlayableTrack(int startIndex) {
    // Try to find next playable track starting from startIndex
    int trackCount = tracks.size();
    if (trackCount == 0) {
        return QString();
    }
    
    // Iterate through tracks, skipping problematic files
    for (int i = 0; i < trackCount; ++i) {
        int idx = (startIndex + i) % trackCount;
        const QString& track = tracks.at(idx);
        
        // Skip problematic files
        if (problematicFiles.contains(track)) {
            continue;
        }
        
        // Check if file still exists and is readable
        if (!isFileReadable(track)) {
            markFileAsProblematic(track);
            continue;
        }
        
        return track;
    }
    
    // No playable tracks found
    qWarning() << "No playable tracks found in track list";
    return QString();
}

bool FileManager::isFileReadable(const QString& filePath) const {
    try {
        QFileInfo fileInfo(filePath);
        return fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable();
    } catch (...) {
        return false;
    }
}
