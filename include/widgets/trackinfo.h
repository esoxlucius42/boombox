#ifndef TRACKINFO_H
#define TRACKINFO_H

#include <QWidget>
#include <QLabel>

class TrackInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrackInfoWidget(QWidget *parent = nullptr);
    
    void setTrackName(const QString &name);
    void setArtistName(const QString &artist);
    void setAlbumName(const QString &album);
    void setTrackNumber(int current, int total);

private:
    QLabel *trackNameLabel;
    QLabel *artistNameLabel;
    QLabel *albumNameLabel;
    QLabel *trackNumberLabel;
};

#endif // TRACKINFO_H
