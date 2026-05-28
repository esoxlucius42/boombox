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
    const QString formatted = formatTime(milliseconds);
    if (currentTimeLabel->text() == formatted) {
        return;
    }
    currentTimeLabel->setText(formatted);
}

void SeekBarWidget::setTotalTime(int milliseconds)
{
    const QString formatted = formatTime(milliseconds);
    if (totalTimeLabel->text() == formatted) {
        return;
    }
    totalTimeLabel->setText(formatted);
}

void SeekBarWidget::setRange(int min, int max)
{
    if (seekSlider->minimum() == min && seekSlider->maximum() == max) {
        return;
    }
    seekSlider->setRange(min, max);
}

int SeekBarWidget::getCurrentPosition() const
{
    return seekSlider->value();
}

void SeekBarWidget::setDuration(int seconds)
{
    if (seekSlider->maximum() != seconds) {
        seekSlider->setMaximum(seconds);
    }

    const QString formatted = formatTimeSeconds(seconds);
    if (totalTimeLabel->text() != formatted) {
        totalTimeLabel->setText(formatted);
    }
}

void SeekBarWidget::updatePosition(int seconds)
{
    if (!isUserDragging) {
        if (seekSlider->value() != seconds) {
            seekSlider->blockSignals(true);
            seekSlider->setValue(seconds);
            seekSlider->blockSignals(false);
        }

        const QString formatted = formatTimeSeconds(seconds);
        if (currentTimeLabel->text() != formatted) {
            currentTimeLabel->setText(formatted);
        }
    }
}

void SeekBarWidget::enableSeeking(bool enabled)
{
    if (seekSlider->isEnabled() == enabled) {
        return;
    }
    seekSlider->setEnabled(enabled);
}

int SeekBarWidget::getRequestedPosition() const
{
    return seekSlider->value();
}

void SeekBarWidget::onSliderMoved(int position)
{
    if (isUserDragging) {
        const QString formatted = formatTimeSeconds(position);
        if (currentTimeLabel->text() != formatted) {
            currentTimeLabel->setText(formatted);
        }
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
