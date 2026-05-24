#include "widgets/trackinfo.h"
#include <QVBoxLayout>
#include <QFont>
#include <QFontMetrics>

TrackInfoWidget::TrackInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 2);
    layout->setSpacing(3);
    
    // Track name label
    trackNameLabel = new QLabel("No track loaded", this);
    trackNameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont trackFont = trackNameLabel->font();
    trackFont.setPointSize(16);
    trackFont.setBold(true);
    trackNameLabel->setFont(trackFont);
    trackNameLabel->setObjectName("trackName");
    trackNameLabel->setWordWrap(true);
    const int titleAreaHeight = (QFontMetrics(trackFont).lineSpacing() * 2) + 6;
    trackNameLabel->setMinimumHeight(titleAreaHeight);
    trackNameLabel->setMaximumHeight(titleAreaHeight);
    layout->addWidget(trackNameLabel);
    layout->addSpacing(QFontMetrics(trackFont).height() / 4);
    
    // Artist name label
    artistNameLabel = new QLabel("Unknown Artist", this);
    artistNameLabel->setAlignment(Qt::AlignCenter);
    QFont artistFont = artistNameLabel->font();
    artistFont.setPointSize(16);
    artistNameLabel->setFont(artistFont);
    artistNameLabel->setObjectName("artistName");
    artistNameLabel->setWordWrap(false);
    layout->addWidget(artistNameLabel);
    
    // Album name label
    albumNameLabel = new QLabel("Unknown Album", this);
    albumNameLabel->setAlignment(Qt::AlignCenter);
    QFont albumFont = albumNameLabel->font();
    albumFont.setPointSize(14);
    albumNameLabel->setFont(albumFont);
    albumNameLabel->setObjectName("albumName");
    albumNameLabel->setWordWrap(false);
    layout->addWidget(albumNameLabel);
    
    // Track number label
    trackNumberLabel = new QLabel("0 / 0", this);
    trackNumberLabel->setAlignment(Qt::AlignCenter);
    QFont numberFont = trackNumberLabel->font();
    numberFont.setPointSize(14);
    trackNumberLabel->setFont(numberFont);
    trackNumberLabel->setObjectName("trackNumber");
    layout->addWidget(trackNumberLabel);
    
    setObjectName("trackInfoWidget");
}

void TrackInfoWidget::setTrackName(const QString &name)
{
    trackNameLabel->setText(name);
}

void TrackInfoWidget::setArtistName(const QString &artist)
{
    artistNameLabel->setText(artist);
}

void TrackInfoWidget::setAlbumName(const QString &album)
{
    albumNameLabel->setText(album);
}

void TrackInfoWidget::setTrackNumber(int current, int total)
{
    trackNumberLabel->setText(QString("%1 / %2").arg(current).arg(total));
}

void TrackInfoWidget::updateTrackInfo(const QString& trackName, const QString& artist,
                                     const QString& album, int trackNum, int totalTracks)
{
    setTrackName(trackName);
    setArtistName(artist);
    setAlbumName(album);
    setTrackNumber(trackNum, totalTracks);
}

void TrackInfoWidget::clearDisplay()
{
    trackNameLabel->setText("No track loaded");
    artistNameLabel->setText("Unknown Artist");
    albumNameLabel->setText("Unknown Album");
    trackNumberLabel->setText("0 / 0");
}
