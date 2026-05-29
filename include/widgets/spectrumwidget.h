#pragma once

#include <QWidget>

#include "spectrumlevels.h"

class SpectrumWidget : public QWidget {
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setLevels(const SpectrumLevels &levels);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SpectrumLevels mLevels;
};
