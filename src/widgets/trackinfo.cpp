#include "widgets/trackinfo.h"
#include <QVBoxLayout>
#include <QFont>

TrackInfoWidget::TrackInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    
    // Track name label
    trackNameLabel = new QLabel("Track Name", this);
    QFont trackFont = trackNameLabel->font();
    trackFont.setPointSize(12);
    trackFont.setBold(true);
    trackNameLabel->setFont(trackFont);
    trackNameLabel->setObjectName("trackName");
    layout->addWidget(trackNameLabel);
    
    // Artist name label
    artistNameLabel = new QLabel("Artist Name", this);
    QFont artistFont = artistNameLabel->font();
    artistFont.setPointSize(10);
    artistNameLabel->setFont(artistFont);
    artistNameLabel->setObjectName("artistName");
    layout->addWidget(artistNameLabel);
    
    // Album name label
    albumNameLabel = new QLabel("Album Name", this);
    QFont albumFont = albumNameLabel->font();
    albumFont.setPointSize(9);
    albumNameLabel->setFont(albumFont);
    albumNameLabel->setObjectName("albumName");
    layout->addWidget(albumNameLabel);
    
    // Track number label
    trackNumberLabel = new QLabel("1 / 10", this);
    trackNumberLabel->setAlignment(Qt::AlignCenter);
    QFont numberFont = trackNumberLabel->font();
    numberFont.setPointSize(8);
    trackNumberLabel->setFont(numberFont);
    trackNumberLabel->setObjectName("trackNumber");
    layout->addWidget(trackNumberLabel);
    
    layout->addStretch();
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
