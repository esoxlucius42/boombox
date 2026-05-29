#include "widgets/spectrumwidget.h"

#include <QPainter>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("spectrumWidget");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(84);
}

QSize SpectrumWidget::minimumSizeHint() const
{
    return {220, 84};
}

QSize SpectrumWidget::sizeHint() const
{
    return {320, 88};
}

void SpectrumWidget::setFrame(const QImage& frame)
{
    mFrame = frame;
    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    if (!mFrame.isNull()) {
        painter.drawImage(rect(), mFrame);
    } else {
        painter.fillRect(rect(), Qt::transparent);
    }
}
