//
// Created by cc on 2019/11/13.
//

#include "NetImageLabel.h"
#include <utility>
#include <QFile>
#include <QPainter>
#include "../Platform/Platform.h"
#include "ChatViewMainPanel.h"
#include "../UICom/qimage/qimage.h"
#include "../UICom/StyleDefine.h"
#include <QPainterPath>

extern ChatViewMainPanel *g_pMainPanel;
NetImageLabel::NetImageLabel(QString link, QWidget* parent)
    :QFrame(parent), _image_link(std::move(link))
{
    setFixedSize(30, 30);
    std::string imgPath = QTalk::GetImagePathByUrl(_image_link.toStdString());

    connect(this, &NetImageLabel::sgDownloadSuccess, [this](){this->update();});

    QFileInfo info(QString::fromStdString(imgPath));
    if(!_image_link.isEmpty() && (!info.exists() || info.isDir()))
    {
        QT_CONCURRENT_FUNC([this](){
            std::string downloadFile = ChatMsgManager::getLocalFilePath(_image_link.toStdString());
            if(!downloadFile.empty())
                emit sgDownloadSuccess();
        });
    }
}

void NetImageLabel::paintEvent(QPaintEvent *event) {

    QPainter painter(this);
    QRect rect = this->contentsRect();

    auto load_default_image = [rect, &painter](){
        auto image = QTalk::qimage::loadImage(":/chatview/image1/default.png", false);
        painter.drawPixmap(rect, image);
    };

    QString imgPath = _local_path;
    if(imgPath.isEmpty())
        imgPath = QTalk::GetImagePathByUrl(_image_link.toStdString()).data();

    if(!imgPath.isEmpty())
    {
        qreal dpi = QTalk::qimage::dpi();
        QPixmap image = QTalk::qimage::loadImage(imgPath,
                                                            false, true, rect.width() * dpi, rect.height() * dpi);
        if(image.isNull())
            load_default_image();
        else
        {
            qreal imgW = image.width() / dpi;
            qreal imgH = image.height() / dpi;
            painter.drawPixmap((rect.width() - imgW) / 2 + rect.x(),
                               (rect.height() - imgH) / 2 + rect.y(),
                               imgW, imgH, image);
        }
    }
    else
        load_default_image();

    if(_showVideoMask)
    {
        auto w = contentsRect().width();
        auto h = contentsRect().height();
        const int sw = 15;
        const int ew = 30;
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 100));
        painter.drawEllipse((w - ew) / 2, (h - ew) / 2, ew, ew);

        auto path = QPainterPath();
        path.moveTo((w - sw) / 2 + 1, (h - sw) / 2);
        path.lineTo((w - sw) / 2 + sw + 1, (h - sw) / 2 + sw / 2);
        path.lineTo((w - sw) / 2 + 1, (h - sw) / 2 + sw);
        painter.setBrush(QColor(255, 255, 255, 100));
        painter.drawPath(path);
    }

    QFrame::paintEvent(event);
}

// show local image
void NetImageLabel::setLocalImage(const QString &local) {
    _local_path = local;
    update();
}

//
void NetImageLabel::showVideoMask()
{
    _showVideoMask = true;
    update();
}

//
bool NetImageLabel::event(QEvent *e) {

    if(e->type() == QEvent::MouseButtonPress)
        emit clicked();
    return QFrame::event(e);
}