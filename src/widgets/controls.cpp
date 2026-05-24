#include "widgets/controls.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>

ControlsWidget::ControlsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    mainLayout->addStretch();
    
    // Playback control buttons (Play/Pause, Next)
    auto playbackLayout = new QHBoxLayout();
    playbackLayout->setSpacing(10);
    
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
    
    // Secondary controls (Browse)
    auto controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(10);

    browseButton = new QPushButton("Browse", this);
    browseButton->setMinimumHeight(50);
    browseButton->setObjectName("browseButton");
    controlLayout->addWidget(browseButton);
    
    mainLayout->addLayout(controlLayout);
    
    // Connect signals
    connect(playPauseButton, &QPushButton::clicked, this, &ControlsWidget::playPauseClicked);
    connect(nextButton, &QPushButton::clicked, this, &ControlsWidget::nextClicked);
    connect(browseButton, &QPushButton::clicked, this, &ControlsWidget::browseClicked);
    
    setObjectName("controlsWidget");
}

void ControlsWidget::setPlayButtonText(const QString &text)
{
    playPauseButton->setText(text);
}

void ControlsWidget::setPlayButtonState(bool playing)
{
    isPlaying = playing;
    setPlayButtonText(playing ? "Pause" : "Play");
}

void ControlsWidget::enableControls(bool enabled)
{
    playPauseButton->setEnabled(enabled);
    nextButton->setEnabled(enabled);
    browseButton->setEnabled(enabled);
}
