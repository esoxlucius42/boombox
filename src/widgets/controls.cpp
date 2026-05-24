#include "widgets/controls.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

ControlsWidget::ControlsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Playback control buttons (Previous, Play/Pause, Next)
    auto playbackLayout = new QHBoxLayout();
    playbackLayout->setSpacing(10);
    
    previousButton = new QPushButton("Previous", this);
    previousButton->setMinimumHeight(50);
    previousButton->setObjectName("previousButton");
    playbackLayout->addWidget(previousButton);
    
    playPauseButton = new QPushButton("Play", this);
    playPauseButton->setMinimumHeight(60);
    QFont playFont = playPauseButton->font();
    playFont.setPointSize(11);
    playFont.setBold(true);
    playPauseButton->setFont(playFont);
    playPauseButton->setObjectName("playPauseButton");
    playbackLayout->addWidget(playPauseButton);
    
    nextButton = new QPushButton("Next", this);
    nextButton->setMinimumHeight(50);
    nextButton->setObjectName("nextButton");
    playbackLayout->addWidget(nextButton);
    
    mainLayout->addLayout(playbackLayout);
    
    // Control buttons grid (Random, Browse, Volume-, Volume+)
    auto controlLayout = new QGridLayout();
    controlLayout->setSpacing(10);
    
    randomButton = new QPushButton("Random Off", this);
    randomButton->setMinimumHeight(50);
    randomButton->setObjectName("randomButton");
    controlLayout->addWidget(randomButton, 0, 0);
    
    browseButton = new QPushButton("Browse", this);
    browseButton->setMinimumHeight(50);
    browseButton->setObjectName("browseButton");
    controlLayout->addWidget(browseButton, 0, 1);
    
    volumeDownButton = new QPushButton("Vol-", this);
    volumeDownButton->setMinimumHeight(50);
    volumeDownButton->setObjectName("volumeDownButton");
    controlLayout->addWidget(volumeDownButton, 1, 0);
    
    volumeUpButton = new QPushButton("Vol+", this);
    volumeUpButton->setMinimumHeight(50);
    volumeUpButton->setObjectName("volumeUpButton");
    controlLayout->addWidget(volumeUpButton, 1, 1);
    
    mainLayout->addLayout(controlLayout);
    mainLayout->addStretch();
    
    // Connect signals
    connect(previousButton, &QPushButton::clicked, this, &ControlsWidget::previousClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &ControlsWidget::playPauseClicked);
    connect(nextButton, &QPushButton::clicked, this, &ControlsWidget::nextClicked);
    connect(randomButton, &QPushButton::clicked, this, &ControlsWidget::randomClicked);
    connect(browseButton, &QPushButton::clicked, this, &ControlsWidget::browseClicked);
    connect(volumeDownButton, &QPushButton::clicked, this, &ControlsWidget::volumeDownClicked);
    connect(volumeUpButton, &QPushButton::clicked, this, &ControlsWidget::volumeUpClicked);
    
    setObjectName("controlsWidget");
}

void ControlsWidget::setPlayButtonText(const QString &text)
{
    playPauseButton->setText(text);
}

void ControlsWidget::setRandomButtonText(const QString &text)
{
    randomButton->setText(text);
}
