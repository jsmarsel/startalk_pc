//
// Created by cc on 2020/1/13.
//

#include "ImageLabel.h"
#include "../UICom/qimage/qimage.h"
#include <QPainter>

ImageLabel::ImageLabel(const QString &imagePath, QWidget *parent)
    : QFrame(parent)
{
    _path = imagePath;
}

void ImageLabel::paintEvent(QPaintEvent *e)
{
    const QRect &rect = this->rect();
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    QPixmap pixmap(_path);
    double width = pixmap.width(), height = pixmap.height();
    double maxWidth = rect.width(), macHeight = rect.height();
    double factor = 1.0;
    double factor1 = maxWidth / width;
    double factor2 = macHeight / height;

    if (factor1 < 1.0 || factor2 < 1.0)
    {
        factor = qMin(factor1, factor2);
    }

    width = factor * width;
    height = factor * height;
    auto dpi = QTalk::qimage::dpi();
    pixmap = QTalk::qimage::loadImage(_path, false, true, width * dpi, height * dpi);
    painter.drawPixmap({rect.x(), rect.y(), (int)width, (int)height}, pixmap);
    QFrame::paintEvent(e);
}
