#include "widgets/spectrumwidget.h"

#include <QPainter>

#include <algorithm>
#include <array>

namespace {

constexpr int kOuterMargin = 6;
constexpr int kChannelGap = 8;
constexpr int kLabelWidth = 12;
constexpr int kBarGap = 4;
constexpr int kBlockGap = 2;

QColor channelColor(int channel, int blockIndex, int totalBlocks)
{
    const QColor lowColor = channel == 0 ? QColor(102, 206, 255) : QColor(149, 255, 176);
    const QColor highColor = channel == 0 ? QColor(227, 141, 255) : QColor(255, 216, 114);
    const qreal t = totalBlocks <= 1 ? 0.0 : static_cast<qreal>(blockIndex) / static_cast<qreal>(totalBlocks - 1);

    return QColor::fromRgbF(lowColor.redF() + (highColor.redF() - lowColor.redF()) * t,
                            lowColor.greenF() + (highColor.greenF() - lowColor.greenF()) * t,
                            lowColor.blueF() + (highColor.blueF() - lowColor.blueF()) * t,
                            1.0);
}

} // namespace

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

void SpectrumWidget::setLevels(const SpectrumLevels &levels)
{
    mLevels = levels;
    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outerRect = rect().adjusted(kOuterMargin, kOuterMargin, -kOuterMargin, -kOuterMargin);
    if (outerRect.isEmpty()) {
        return;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(18, 18, 18, 110));
    painter.drawRoundedRect(outerRect, 8.0, 8.0);

    painter.setPen(QColor(255, 255, 255, 40));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(outerRect, 8.0, 8.0);

    const QRectF contentRect = outerRect.adjusted(8.0, 6.0, -8.0, -6.0);
    const qreal channelHeight = (contentRect.height() - kChannelGap) / 2.0;
    const qreal barsLeft = contentRect.left() + kLabelWidth;
    const qreal barsWidth = std::max<qreal>(0.0, contentRect.width() - kLabelWidth);

    painter.setPen(QColor(255, 255, 255, 150));
    const QFontMetrics metrics(font());
    painter.drawText(QRectF(contentRect.left(), contentRect.top(), kLabelWidth, channelHeight),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     metrics.elidedText(QStringLiteral("L"), Qt::ElideRight, kLabelWidth));
    painter.drawText(QRectF(contentRect.left(), contentRect.top() + channelHeight + kChannelGap, kLabelWidth, channelHeight),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     metrics.elidedText(QStringLiteral("R"), Qt::ElideRight, kLabelWidth));

    const qreal barWidth =
        (barsWidth - static_cast<qreal>(SpectrumLevels::kBandCount - 1) * kBarGap) / static_cast<qreal>(SpectrumLevels::kBandCount);

    for (int channel = 0; channel < SpectrumLevels::kChannelCount; ++channel) {
        const qreal top = contentRect.top() + channel * (channelHeight + kChannelGap);
        const QRectF channelRect(barsLeft, top, barsWidth, channelHeight);
        const qreal blockHeight =
            (channelRect.height() - static_cast<qreal>(SpectrumLevels::kBlockCount - 1) * kBlockGap) / static_cast<qreal>(SpectrumLevels::kBlockCount);

        for (int band = 0; band < SpectrumLevels::kBandCount; ++band) {
            const qreal left = channelRect.left() + band * (barWidth + kBarGap);
            const int activeBlocks = std::clamp(mLevels.channels[channel][band], 0, SpectrumLevels::kBlockCount);

            for (int block = 0; block < SpectrumLevels::kBlockCount; ++block) {
                const int reverseIndex = SpectrumLevels::kBlockCount - 1 - block;
                const qreal blockTop = channelRect.top() + reverseIndex * (blockHeight + kBlockGap);
                const QRectF blockRect(left, blockTop, std::max<qreal>(2.0, barWidth), std::max<qreal>(2.0, blockHeight));
                const bool isActive = mLevels.active && block < activeBlocks;

                painter.setPen(Qt::NoPen);
                painter.setBrush(isActive ? channelColor(channel, block, SpectrumLevels::kBlockCount) : QColor(255, 255, 255, 24));
                painter.drawRoundedRect(blockRect, 1.6, 1.6);
            }
        }
    }
}
