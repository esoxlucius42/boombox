#include "widgets/controls.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>

ControlsWidget::ControlsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 4, 10, 10);
    mainLayout->setSpacing(10);
    mainLayout->addStretch(1);
    
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
    
    // Secondary controls (Fullscreen, Browse)
    auto controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(10);

    fullscreenButton = new QPushButton("FS", this);
    fullscreenButton->setMinimumHeight(50);
    fullscreenButton->setMinimumWidth(60);
    fullscreenButton->setObjectName("fullscreenButton");
    controlLayout->addWidget(fullscreenButton);

    browseButton = new QPushButton("Browse", this);
    browseButton->setMinimumHeight(50);
    browseButton->setObjectName("browseButton");
    controlLayout->addWidget(browseButton);
    
    mainLayout->addLayout(controlLayout);
    
    // Connect signals
    connect(playPauseButton, &QPushButton::clicked, this, &ControlsWidget::playPauseClicked);
    connect(nextButton, &QPushButton::clicked, this, &ControlsWidget::nextClicked);
    connect(fullscreenButton, &QPushButton::clicked, this, &ControlsWidget::fullscreenClicked);
    connect(browseButton, &QPushButton::clicked, this, &ControlsWidget::browseClicked);
    
    setObjectName("controlsWidget");
}

void ControlsWidget::setPlayButtonText(const QString &text)
{
    if (playPauseButton->text() == text) {
        return;
    }
    playPauseButton->setText(text);
}

void ControlsWidget::setPlayButtonState(bool playing)
{
    if (isPlaying == playing) {
        return;
    }
    isPlaying = playing;
    setPlayButtonText(playing ? "Pause" : "Play");
}

void ControlsWidget::enableControls(bool enabled)
{
    if (playPauseButton->isEnabled() != enabled) {
        playPauseButton->setEnabled(enabled);
    }
    if (nextButton->isEnabled() != enabled) {
        nextButton->setEnabled(enabled);
    }
    if (fullscreenButton->isEnabled() != enabled) {
        fullscreenButton->setEnabled(enabled);
    }
    if (browseButton->isEnabled() != enabled) {
        browseButton->setEnabled(enabled);
    }
}
