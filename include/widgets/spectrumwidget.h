#pragma once

#include <QImage>
#include <QWidget>

class SpectrumWidget : public QWidget {
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setFrame(const QImage& frame);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage mFrame;
};
