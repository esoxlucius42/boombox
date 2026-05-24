#ifndef ALBUMART_H
#define ALBUMART_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <QPixmapCache>
#include <memory>

/**
 * @brief AlbumArtWidget - Displays album art with lazy loading and caching
 * 
 * Features:
 * - Loads album art from pixmap or file
 * - Shows placeholder for missing art
 * - Caches extracted images (max 10)
 * - Scales images to fit 460x460 while maintaining aspect ratio
 */
class AlbumArtWidget : public QWidget {
    Q_OBJECT

public:
    explicit AlbumArtWidget(QWidget *parent = nullptr);
    ~AlbumArtWidget() = default;

    /**
     * @brief Set album art from pixmap
     * @param pixmap Image pixmap to display
     */
    void setAlbumArt(const QPixmap& pixmap);

    /**
     * @brief Load and display album art from file
     * @param filePath Path to image file
     */
    void setAlbumArtFromFile(const QString& filePath);

    /**
     * @brief Extract and display album art from audio file
     * @param filePath Path to audio file (MP3, FLAC, OGG, etc.)
     */
    void extractAndDisplayAlbumArt(const QString& filePath);

    /**
     * @brief Clear current album art and show placeholder
     */
    void clearAlbumArt();

    /**
     * @brief Show default grey placeholder
     */
    void setPlaceholder();

private:
    QLabel *artLabel;
    QPixmap currentPixmap;
    QMap<QString, QPixmap> imageCache;  // Cache for extracted images
    static constexpr int MAX_CACHE_SIZE = 10;  // Max images to cache
    static constexpr int DISPLAY_SIZE = 460;  // 460x460 display size

    /**
     * @brief Create and return a placeholder pixmap
     */
    QPixmap createPlaceholder();

    /**
     * @brief Scale pixmap to fit display size while maintaining aspect ratio
     */
    QPixmap scalePixmapToFit(const QPixmap& original);

    /**
     * @brief Extract album art from MP3 file (ID3v2)
     */
    QPixmap extractMP3Art(const QString& filePath);

    /**
     * @brief Extract album art from FLAC file
     */
    QPixmap extractFLACArt(const QString& filePath);

    /**
     * @brief Extract album art from OGG Vorbis file
     */
    QPixmap extractOGGArt(const QString& filePath);

    /**
     * @brief Clear old cache entries if size exceeds limit
     */
    void maintainCacheSize();

    /**
     * @brief Update cache if image doesn't exist
     */
    void updateCache(const QString& filePath, const QPixmap& pixmap);
};

#endif // ALBUMART_H
