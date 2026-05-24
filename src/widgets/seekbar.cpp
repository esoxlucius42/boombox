#include "widgets/seekbar.h"
#include <QHBoxLayout>
#include <QFont>

SeekBarWidget::SeekBarWidget(QWidget *parent)
    : QWidget(parent)
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
    seekSlider->setObjectName("seekSlider");
    layout->addWidget(seekSlider);
    
    // Total time label
    totalTimeLabel = new QLabel("0:00", this);
    totalTimeLabel->setFont(timeFont);
    totalTimeLabel->setObjectName("totalTime");
    totalTimeLabel->setMinimumWidth(40);
    layout->addWidget(totalTimeLabel);
    
    connect(seekSlider, &QSlider::sliderMoved, this, &SeekBarWidget::positionChanged);
    
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

QString SeekBarWidget::formatTime(int milliseconds) const
{
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
