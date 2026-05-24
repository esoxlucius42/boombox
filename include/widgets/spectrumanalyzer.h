#ifndef SPECTRUMANALYZER_H
#define SPECTRUMANALYZER_H

#include <QVector>
#include <QWidget>

class SpectrumAnalyzerWidget : public QWidget {
    Q_OBJECT

public:
    explicit SpectrumAnalyzerWidget(QWidget *parent = nullptr);

    void setSpectrumLevels(const QVector<float>& levels);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;

private:
    static constexpr int BAR_COUNT = 24;
    static constexpr int SEGMENT_COUNT = 10;

    QVector<float> currentLevels;
    QVector<float> peakLevels;

    QVector<float> normalizeLevels(const QVector<float>& levels) const;
    static float clampLevel(float value);
};

#endif // SPECTRUMANALYZER_H
