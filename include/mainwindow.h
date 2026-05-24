#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "widgets/trackinfo.h"
#include "widgets/seekbar.h"
#include "widgets/controls.h"

class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUI();
    void loadStylesheet();

    TrackInfoWidget *trackInfoWidget;
    SeekBarWidget *seekBarWidget;
    ControlsWidget *controlsWidget;
    QLabel *albumArtLabel;
};

#endif // MAINWINDOW_H
