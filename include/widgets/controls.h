#ifndef CONTROLS_H
#define CONTROLS_H

#include <QWidget>
#include <QPushButton>

class ControlsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ControlsWidget(QWidget *parent = nullptr);
    
    void setPlayButtonText(const QString &text);
    void setRandomButtonText(const QString &text);

signals:
    void previousClicked();
    void playPauseClicked();
    void nextClicked();
    void randomClicked();
    void browseClicked();
    void volumeDownClicked();
    void volumeUpClicked();

private:
    QPushButton *previousButton;
    QPushButton *playPauseButton;
    QPushButton *nextButton;
    QPushButton *randomButton;
    QPushButton *browseButton;
    QPushButton *volumeDownButton;
    QPushButton *volumeUpButton;
};

#endif // CONTROLS_H
