#ifndef CONTROLS_H
#define CONTROLS_H

#include <QWidget>
#include <QPushButton>

class ControlsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ControlsWidget(QWidget *parent = nullptr);
    
    void setPlayButtonText(const QString &text);
    void setPlayButtonState(bool playing);
    void enableControls(bool enabled);

signals:
    void playPauseClicked();
    void nextClicked();
    void fullscreenClicked();
    void browseClicked();

private:
    QPushButton *playPauseButton;
    QPushButton *nextButton;
    QPushButton *fullscreenButton;
    QPushButton *browseButton;
    bool isPlaying = false;
};

#endif // CONTROLS_H
