//
// Created by cc on 2020/9/10.
//

#include "DropWnd.h"
#include <QPainter>

void DropWnd::paintEvent(QPaintEvent *e) {
    //
    QPainter painter(this);

    QPen pen;
    pen.setColor(QColor(176,176,176));
    pen.setStyle((Qt::DotLine));
    pen.setWidth(3);
    painter.setPen(pen);
    auto rect = contentsRect();
    painter.drawRoundedRect(rect.x() + 50, rect.y() + 50, rect.width() - 100, rect.height() - 100,
            30, 30);
    //
    painter.setOpacity(0.3);
    painter.drawPixmap(contentsRect(), _mask);
    //
    painter.setPen(QColor(0, 0, 0));
    QFont qFont;
    qFont.setPixelSize(20);
    painter.setFont(qFont);
    painter.drawText(rect, Qt::AlignCenter, tr("拖动到这里发送给 %1").arg(_name));

    QFrame::paintEvent(e);
}

//
void DropWnd::setDrop(const QPixmap& pixmap, const QString& name) {
    _name = name;
    _mask = pixmap;
    update();
}