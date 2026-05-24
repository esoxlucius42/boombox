#include "widgets/albumart.h"
#include <QVBoxLayout>
#include <QFile>
#include <QByteArray>
#include <QBuffer>
#include <QImage>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QPainter>
#include <algorithm>

AlbumArtWidget::AlbumArtWidget(QWidget *parent)
    : QWidget(parent) {
    
    // Create label for displaying art
    artLabel = new QLabel(this);
    artLabel->setAlignment(Qt::AlignCenter);
    artLabel->setMinimumSize(DISPLAY_SIZE, DISPLAY_SIZE);
    artLabel->setMaximumSize(DISPLAY_SIZE, DISPLAY_SIZE);
    
    // Set up layout
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(artLabel);
    
    // Show placeholder initially
    setPlaceholder();
}

void AlbumArtWidget::setAlbumArt(const QPixmap& pixmap) {
    if (pixmap.isNull()) {
        setPlaceholder();
        return;
    }
    
    currentPixmap = scalePixmapToFit(pixmap);
    artLabel->setPixmap(currentPixmap);
}

void AlbumArtWidget::setAlbumArtFromFile(const QString& filePath) {
    if (filePath.isEmpty()) {
        setPlaceholder();
        return;
    }
    
    QPixmap pixmap;
    
    // Try to load from cache first
    if (imageCache.contains(filePath)) {
        pixmap = imageCache.value(filePath);
    } else {
        // Load from file
        pixmap.load(filePath);
        if (!pixmap.isNull()) {
            updateCache(filePath, pixmap);
        }
    }
    
    if (pixmap.isNull()) {
        setPlaceholder();
    } else {
        setAlbumArt(pixmap);
    }
}

void AlbumArtWidget::extractAndDisplayAlbumArt(const QString& filePath) {
    if (filePath.isEmpty()) {
        setPlaceholder();
        return;
    }
    
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    QPixmap pixmap;
    
    // Check cache first
    if (imageCache.contains(filePath)) {
        pixmap = imageCache.value(filePath);
        setAlbumArt(pixmap);
        return;
    }
    
    // Extract based on file type
    try {
        if (suffix == "mp3") {
            pixmap = extractMP3Art(filePath);
        } else if (suffix == "flac") {
            pixmap = extractFLACArt(filePath);
        } else if (suffix == "ogg" || suffix == "oga") {
            pixmap = extractOGGArt(filePath);
        } else {
            // For other formats, try basic image file loading
            setPlaceholder();
            return;
        }
    } catch (const std::exception& e) {
        qWarning() << "Exception extracting album art:" << e.what();
        pixmap = QPixmap();
    } catch (...) {
        qWarning() << "Unknown exception extracting album art";
        pixmap = QPixmap();
    }
    
    if (!pixmap.isNull()) {
        updateCache(filePath, pixmap);
        setAlbumArt(pixmap);
    } else {
        setPlaceholder();
    }
}

void AlbumArtWidget::clearAlbumArt() {
    setPlaceholder();
}

void AlbumArtWidget::setPlaceholder() {
    currentPixmap = createPlaceholder();
    artLabel->setPixmap(currentPixmap);
}

QPixmap AlbumArtWidget::createPlaceholder() {
    QPixmap placeholder(DISPLAY_SIZE, DISPLAY_SIZE);
    placeholder.fill(QColor(80, 80, 80));  // Grey background
    
    // Draw "No Album Art" text
    QPainter painter(&placeholder);
    painter.setPen(QColor(150, 150, 150));
    QFont font = painter.font();
    font.setPointSize(14);
    painter.setFont(font);
    painter.drawText(placeholder.rect(), Qt::AlignCenter, "No Album Art");
    
    return placeholder;
}

QPixmap AlbumArtWidget::scalePixmapToFit(const QPixmap& original) {
    if (original.isNull()) {
        return createPlaceholder();
    }
    
    // Scale to fit 460x460 while maintaining aspect ratio
    QPixmap scaled = original.scaledToWidth(DISPLAY_SIZE, Qt::SmoothTransformation);
    
    if (scaled.height() > DISPLAY_SIZE) {
        scaled = original.scaledToHeight(DISPLAY_SIZE, Qt::SmoothTransformation);
    }
    
    // Create canvas and center the image
    QPixmap canvas(DISPLAY_SIZE, DISPLAY_SIZE);
    canvas.fill(QColor(80, 80, 80));
    
    QPainter painter(&canvas);
    int x = (DISPLAY_SIZE - scaled.width()) / 2;
    int y = (DISPLAY_SIZE - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
    
    return canvas;
}

QPixmap AlbumArtWidget::extractMP3Art(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    
    // Read file header to check for ID3v2
    QByteArray header = file.read(10);
    
    // Check for ID3v2 header (starts with "ID3")
    if (header.size() < 10 || header[0] != 'I' || header[1] != 'D' || header[2] != '3') {
        file.close();
        return QPixmap();
    }
    
    // Parse ID3v2 size (bytes 6-9, synchsafe)
    int size = ((header[6] & 0x7f) << 21) |
               ((header[7] & 0x7f) << 14) |
               ((header[8] & 0x7f) << 7) |
               (header[9] & 0x7f);
    
    // Read the entire ID3v2 tag
    QByteArray id3Data = file.read(size);
    file.close();
    
    // Look for APIC frame (Album Art)
    // APIC frame starts with "APIC" followed by frame size and flags
    int apicPos = id3Data.indexOf("APIC");
    if (apicPos == -1) {
        return QPixmap();
    }
    
    // Parse APIC frame
    // Skip frame header: "APIC" (4) + size (4) + flags (2) = 10 bytes
    if (apicPos + 10 > id3Data.size()) {
        return QPixmap();
    }
    
    int frameDataStart = apicPos + 10;
    
    // Skip: encoding (1 byte) + MIME type (null-terminated) + description (null-terminated)
    int pos = frameDataStart + 1;  // Skip encoding
    
    // Skip MIME type
    while (pos < id3Data.size() && id3Data[pos] != 0) {
        pos++;
    }
    pos++;  // Skip null terminator
    
    // Skip description
    while (pos < id3Data.size() && id3Data[pos] != 0) {
        pos++;
    }
    pos++;  // Skip null terminator
    
    // Remaining data is the image
    if (pos >= id3Data.size()) {
        return QPixmap();
    }
    
    QByteArray imageData = id3Data.mid(pos);
    QPixmap pixmap;
    pixmap.loadFromData(imageData);
    
    return pixmap;
}

QPixmap AlbumArtWidget::extractFLACArt(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    
    // Check for FLAC signature (fLaC)
    QByteArray signature = file.read(4);
    if (signature != "fLaC") {
        file.close();
        return QPixmap();
    }
    
    // Parse FLAC metadata blocks
    bool isLastMetadataBlock = false;
    
    while (!isLastMetadataBlock && !file.atEnd()) {
        QByteArray blockHeader = file.read(4);
        if (blockHeader.size() < 4) break;
        
        // Parse block header
        quint8 headerByte = blockHeader[0];
        isLastMetadataBlock = (headerByte & 0x80) != 0;
        quint8 blockType = headerByte & 0x7f;
        
        // Block size is in bytes 1-3
        quint32 blockSize = ((blockHeader[1] & 0xff) << 16) |
                           ((blockHeader[2] & 0xff) << 8) |
                           (blockHeader[3] & 0xff);
        
        // Block type 6 is PICTURE
        if (blockType == 6) {
            QByteArray pictureData = file.read(blockSize);
            if (pictureData.size() < 32) {
                continue;
            }
            
            // Parse PICTURE block
            // Skip: picture type (4), MIME length (4), MIME (variable), description length (4), description (variable)
            QBuffer buffer(&pictureData);
            buffer.open(QIODevice::ReadOnly);
            
            // Skip picture type
            buffer.seek(4);
            
            // Read MIME type length
            QByteArray mimeLen = buffer.read(4);
            if (mimeLen.size() < 4) continue;
            quint32 mimeLength = (mimeLen[0] << 24) | (mimeLen[1] << 16) | (mimeLen[2] << 8) | mimeLen[3];
            
            // Skip MIME
            buffer.seek(buffer.pos() + mimeLength);
            
            // Skip description
            if (buffer.pos() + 4 > pictureData.size()) continue;
            QByteArray descLen = buffer.read(4);
            quint32 descLength = (descLen[0] << 24) | (descLen[1] << 16) | (descLen[2] << 8) | descLen[3];
            buffer.seek(buffer.pos() + descLength);
            
            // Skip width, height, color depth, colors used (4*4 = 16 bytes)
            buffer.seek(buffer.pos() + 16);
            
            // Read picture data length
            if (buffer.pos() + 4 > pictureData.size()) continue;
            QByteArray picLen = buffer.read(4);
            quint32 picDataLength = (picLen[0] << 24) | (picLen[1] << 16) | (picLen[2] << 8) | picLen[3];
            
            // Read and load image
            QByteArray imageData = buffer.read(picDataLength);
            QPixmap pixmap;
            pixmap.loadFromData(imageData);
            
            file.close();
            return pixmap;
        } else {
            // Skip this metadata block
            file.seek(file.pos() + blockSize);
        }
    }
    
    file.close();
    return QPixmap();
}

QPixmap AlbumArtWidget::extractOGGArt(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    
    // OGG Vorbis format parsing is complex
    // For now, return empty pixmap
    // In a real implementation, would parse Vorbis comments for METADATA_BLOCK_PICTURE
    file.close();
    return QPixmap();
}

void AlbumArtWidget::maintainCacheSize() {
    if (imageCache.size() > MAX_CACHE_SIZE) {
        // Remove oldest entries (FIFO)
        auto it = imageCache.begin();
        int toRemove = imageCache.size() - MAX_CACHE_SIZE;
        while (toRemove > 0 && it != imageCache.end()) {
            it = imageCache.erase(it);
            toRemove--;
        }
    }
}

void AlbumArtWidget::updateCache(const QString& filePath, const QPixmap& pixmap) {
    imageCache.insert(filePath, pixmap);
    maintainCacheSize();
}
