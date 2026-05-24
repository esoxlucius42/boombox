#include "mainwindow.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QFile>
#include <QApplication>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Boombox");
    
    // Set window size to 800x480 (landscape, touchscreen display)
    resize(800, 480);
    
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Setup UI
    setupUI();
    
    // Load stylesheet
    loadStylesheet();
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = this->centralWidget();
    
    // Main horizontal layout: Left pane | Right pane
    auto mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // ===== LEFT PANE (320x480) =====
    QWidget *leftPane = new QWidget(this);
    leftPane->setObjectName("leftPane");
    leftPane->setMinimumWidth(320);
    leftPane->setMaximumWidth(320);
    
    auto leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    // Pane 0: Track Info (80px)
    trackInfoWidget = new TrackInfoWidget(this);
    trackInfoWidget->setObjectName("pane0");
    trackInfoWidget->setMaximumHeight(80);
    leftLayout->addWidget(trackInfoWidget);
    
    // Pane 1: Seek Bar (100px)
    seekBarWidget = new SeekBarWidget(this);
    seekBarWidget->setObjectName("pane1");
    seekBarWidget->setMaximumHeight(100);
    leftLayout->addWidget(seekBarWidget);
    
    // Pane 2: Control Buttons (300px)
    controlsWidget = new ControlsWidget(this);
    controlsWidget->setObjectName("pane2");
    controlsWidget->setMaximumHeight(300);
    leftLayout->addWidget(controlsWidget);
    
    // Add stretch at the end to ensure proper alignment
    leftLayout->addStretch();
    
    mainLayout->addWidget(leftPane);
    
    // ===== RIGHT PANE (480x480) =====
    QWidget *rightPane = new QWidget(this);
    rightPane->setObjectName("rightPane");
    rightPane->setMinimumSize(480, 480);
    rightPane->setMaximumSize(480, 480);
    
    auto rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    
    // Album art label
    albumArtLabel = new QLabel(this);
    albumArtLabel->setObjectName("albumArt");
    albumArtLabel->setAlignment(Qt::AlignCenter);
    albumArtLabel->setMinimumSize(460, 460);
    albumArtLabel->setMaximumSize(460, 460);
    
    // Create a placeholder image
    QPixmap placeholder(460, 460);
    placeholder.fill(QColor(80, 80, 80));
    albumArtLabel->setPixmap(placeholder);
    
    rightLayout->addWidget(albumArtLabel);
    
    mainLayout->addWidget(rightPane);
    
    // Set test data
    trackInfoWidget->setTrackName("Track Name");
    trackInfoWidget->setArtistName("Artist Name");
    trackInfoWidget->setAlbumName("Album Name");
    trackInfoWidget->setTrackNumber(3, 15);
    
    seekBarWidget->setCurrentTime(90000); // 1:30
    seekBarWidget->setTotalTime(300000);  // 5:00
}

void MainWindow::loadStylesheet()
{
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(style);
        styleFile.close();
    }
}
