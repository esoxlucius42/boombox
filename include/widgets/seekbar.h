#ifndef SEEKBAR_H
#define SEEKBAR_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

class SeekBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SeekBarWidget(QWidget *parent = nullptr);
    
    // Legacy methods for backwards compatibility
    void setCurrentTime(int milliseconds);
    void setTotalTime(int milliseconds);
    void setRange(int min, int max);
    int getCurrentPosition() const;
    
    // New methods as per requirements
    void setDuration(int seconds);
    void updatePosition(int seconds);
    void enableSeeking(bool enabled);
    int getRequestedPosition() const;

signals:
    void positionChanged(int position);
    void userSeeked(int positionSeconds);

private slots:
    void onSliderMoved(int position);
    void onSliderPressed();
    void onSliderReleased();

private:
    QLabel *currentTimeLabel;
    QSlider *seekSlider;
    QLabel *totalTimeLabel;
    bool isUserDragging;
    
    QString formatTime(int milliseconds) const;
    QString formatTimeSeconds(int seconds) const;
};

#endif // SEEKBAR_H
