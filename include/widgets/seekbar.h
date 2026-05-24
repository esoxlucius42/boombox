#ifndef SEEKBAR_H
#define SEEKBAR_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

class SeekBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit SeekBarWidget(QWidget *parent = nullptr);
    
    void setCurrentTime(int milliseconds);
    void setTotalTime(int milliseconds);
    void setRange(int min, int max);
    
    int getCurrentPosition() const;

signals:
    void positionChanged(int position);

private:
    QLabel *currentTimeLabel;
    QSlider *seekSlider;
    QLabel *totalTimeLabel;
    
    QString formatTime(int milliseconds) const;
};

#endif // SEEKBAR_H
