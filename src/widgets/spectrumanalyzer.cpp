#include "widgets/spectrumanalyzer.h"

#include <QPainter>
#include <algorithm>
#include <cmath>

SpectrumAnalyzerWidget::SpectrumAnalyzerWidget(QWidget *parent)
    : QWidget(parent),
      currentLevels(BAR_COUNT, 0.0f),
      peakLevels(BAR_COUNT, 0.0f)
{
    setObjectName("spectrumAnalyzerWidget");
    setMinimumHeight(62);
    setMaximumHeight(78);
}

void SpectrumAnalyzerWidget::setSpectrumLevels(const QVector<float>& levels)
{
    const QVector<float> normalized = normalizeLevels(levels);

    for (int i = 0; i < BAR_COUNT; ++i) {
        const float level = normalized[i];
        currentLevels[i] = level;

        if (level >= peakLevels[i]) {
            peakLevels[i] = level;
        } else {
            peakLevels[i] = std::max(0.0f, peakLevels[i] - 0.035f);
        }
    }

    update();
}

void SpectrumAnalyzerWidget::clear()
{
    std::fill(currentLevels.begin(), currentLevels.end(), 0.0f);
    std::fill(peakLevels.begin(), peakLevels.end(), 0.0f);
    update();
}

QSize SpectrumAnalyzerWidget::sizeHint() const
{
    return {320, 70};
}

void SpectrumAnalyzerWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    // Subtle dotted backdrop for minimal vintage-style analyzer texture.
    painter.setBrush(QColor(92, 100, 104, 45));
    const int dotStep = 12;
    for (int y = 6; y < height(); y += dotStep) {
        for (int x = 6; x < width(); x += dotStep) {
            painter.drawEllipse(QRectF(x, y, 2.0, 2.0));
        }
    }

    const int leftPadding = 10;
    const int rightPadding = 10;
    const int topPadding = 8;
    const int bottomPadding = 8;
    const int segmentSpacing = 2;
    const int barSpacing = 4;

    const int usableWidth = std::max(1, width() - leftPadding - rightPadding);
    const int usableHeight = std::max(1, height() - topPadding - bottomPadding);
    const int barWidth = std::max(2, (usableWidth - ((BAR_COUNT - 1) * barSpacing)) / BAR_COUNT);
    const int segmentHeight = std::max(2, (usableHeight - ((SEGMENT_COUNT - 1) * segmentSpacing)) / SEGMENT_COUNT);
    const int roundedRadius = 2;

    const QColor onColor(235, 239, 233);
    const QColor offColor(128, 135, 139, 55);
    const QColor peakColor(248, 202, 74);

    for (int i = 0; i < BAR_COUNT; ++i) {
        const int x = leftPadding + i * (barWidth + barSpacing);
        const int litSegments = static_cast<int>(std::round(clampLevel(currentLevels[i]) * SEGMENT_COUNT));
        const int peakSegment = std::clamp(
            static_cast<int>(std::round(clampLevel(peakLevels[i]) * SEGMENT_COUNT)) - 1,
            0,
            SEGMENT_COUNT - 1
        );

        for (int segment = 0; segment < SEGMENT_COUNT; ++segment) {
            const int y = height() - bottomPadding - (segment + 1) * segmentHeight - segment * segmentSpacing;
            const QRectF segmentRect(x, y, barWidth, segmentHeight);
            const bool isLit = segment < litSegments;
            const bool isPeak = segment == peakSegment;

            painter.setBrush(isPeak ? peakColor : (isLit ? onColor : offColor));
            painter.drawRoundedRect(segmentRect, roundedRadius, roundedRadius);
        }
    }
}

QVector<float> SpectrumAnalyzerWidget::normalizeLevels(const QVector<float>& levels) const
{
    QVector<float> normalized(BAR_COUNT, 0.0f);
    if (levels.isEmpty()) {
        return normalized;
    }

    if (levels.size() == BAR_COUNT) {
        for (int i = 0; i < BAR_COUNT; ++i) {
            normalized[i] = clampLevel(levels[i]);
        }
        return normalized;
    }

    for (int i = 0; i < BAR_COUNT; ++i) {
        const int src = static_cast<int>(std::floor((static_cast<double>(i) * levels.size()) / BAR_COUNT));
        const int maxIndex = static_cast<int>(levels.size()) - 1;
        normalized[i] = clampLevel(levels[std::clamp(src, 0, maxIndex)]);
    }
    return normalized;
}

float SpectrumAnalyzerWidget::clampLevel(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}
