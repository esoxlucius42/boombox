#include "widgets/seekbar.h"
#include <QHBoxLayout>
#include <QFont>

SeekBarWidget::SeekBarWidget(QWidget *parent)
    : QWidget(parent), isUserDragging(false)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    
    // Current time label
    currentTimeLabel = new QLabel("0:00", this);
    QFont timeFont = currentTimeLabel->font();
    timeFont.setPointSize(8);
    currentTimeLabel->setFont(timeFont);
    currentTimeLabel->setObjectName("currentTime");
    currentTimeLabel->setMinimumWidth(40);
    layout->addWidget(currentTimeLabel);
    
    // Seek slider
    seekSlider = new QSlider(Qt::Horizontal, this);
    seekSlider->setMinimum(0);
    seekSlider->setMaximum(0);
    seekSlider->setMinimumHeight(10);
    seekSlider->setObjectName("seekSlider");
    layout->addWidget(seekSlider);
    
    // Total time label
    totalTimeLabel = new QLabel("0:00", this);
    totalTimeLabel->setFont(timeFont);
    totalTimeLabel->setObjectName("totalTime");
    totalTimeLabel->setMinimumWidth(40);
    layout->addWidget(totalTimeLabel);
    
    // Connect slider signals
    connect(seekSlider, &QSlider::sliderMoved, this, &SeekBarWidget::onSliderMoved);
    connect(seekSlider, &QSlider::sliderPressed, this, &SeekBarWidget::onSliderPressed);
    connect(seekSlider, &QSlider::sliderReleased, this, &SeekBarWidget::onSliderReleased);
    // Also handle direct clicks
    connect(seekSlider, QOverload<int>::of(&QSlider::valueChanged), this, [this](int value) {
        if (!isUserDragging) {
            emit positionChanged(value);
        }
    });
    
    setObjectName("seekBarWidget");
}

void SeekBarWidget::setCurrentTime(int milliseconds)
{
    currentTimeLabel->setText(formatTime(milliseconds));
}

void SeekBarWidget::setTotalTime(int milliseconds)
{
    totalTimeLabel->setText(formatTime(milliseconds));
}

void SeekBarWidget::setRange(int min, int max)
{
    seekSlider->setRange(min, max);
}

int SeekBarWidget::getCurrentPosition() const
{
    return seekSlider->value();
}

void SeekBarWidget::setDuration(int seconds)
{
    seekSlider->setMaximum(seconds);
    totalTimeLabel->setText(formatTimeSeconds(seconds));
}

void SeekBarWidget::updatePosition(int seconds)
{
    if (!isUserDragging) {
        seekSlider->blockSignals(true);
        seekSlider->setValue(seconds);
        seekSlider->blockSignals(false);
        currentTimeLabel->setText(formatTimeSeconds(seconds));
    }
}

void SeekBarWidget::enableSeeking(bool enabled)
{
    seekSlider->setEnabled(enabled);
}

int SeekBarWidget::getRequestedPosition() const
{
    return seekSlider->value();
}

void SeekBarWidget::onSliderMoved(int position)
{
    if (isUserDragging) {
        currentTimeLabel->setText(formatTimeSeconds(position));
        emit userSeeked(position);
        emit positionChanged(position);
    }
}

void SeekBarWidget::onSliderPressed()
{
    isUserDragging = true;
}

void SeekBarWidget::onSliderReleased()
{
    isUserDragging = false;
    emit userSeeked(seekSlider->value());
}

QString SeekBarWidget::formatTime(int milliseconds) const
{
    int seconds = milliseconds / 1000;
    return formatTimeSeconds(seconds);
}

QString SeekBarWidget::formatTimeSeconds(int seconds) const
{
    int minutes = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(secs, 2, 10, QChar('0'));
}
