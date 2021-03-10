﻿#include "HeadPhotoLab.h"
#include "../UICom/qimage/qimage.h"
#include "../UICom/StyleDefine.h"
#include <QBitmap>
#include <QDebug>
#include <QPainter>
#include <QFileInfo>
#include <QPainterPath>
#include <utility>

HeadPhotoLab::HeadPhotoLab(QString strHeadPath,
        int radius,
        bool isGrey,
        bool startMovie,
        bool showRect,
        QWidget *parent)
    : QLabel(parent), _imagePath(std::move(strHeadPath))
    , _radius(radius), _isGrey(isGrey)
    , _mov(nullptr), _startMovie(startMovie)
    , _showRect(showRect)
{
    if(radius != 0)
    {
        setFixedSize( radius * 2, radius * 2);
    }
    initMovie();
}

/**
 *
 * @param headPath
 */
void HeadPhotoLab::setHead(const QString& headPath, int radius, bool isGrey, bool startMovie, bool showRect)
{
    if(!headPath.isEmpty())
    {
        _imagePath = headPath;
    }
    if(radius != 0)
    {
        setFixedSize( radius * 2, radius * 2);
        _radius = radius;
    }
    _pixmap.clear();
    _isGrey = isGrey;
    _startMovie = startMovie;
    _showRect = showRect;
    initMovie();
    update();
}

/**
 *
 * @param e
 */
void HeadPhotoLab::paintEvent(QPaintEvent *e)
{
    //
    int dpi = QTalk::qimage::dpi();
    QPixmap pixmap;
    if(_mov)
    {
        auto index = _mov->currentFrameNumber();
        if(index > 0)
        {
            if(index >= _pixmap.size())
            {
                pixmap = _mov->currentPixmap();
                pixmap = QTalk::qimage::scaledPixmap(pixmap, _radius * dpi * 2);
                _pixmap.push_back(pixmap);
            }
            else
                pixmap = _pixmap[index];
        }
    }

    if(pixmap.isNull())
    {
        if(_pixmap.empty())
        {
            pixmap = QTalk::qimage::loadImage(_imagePath, true, true, _radius * 2 * dpi);
            _pixmap.push_back(pixmap);
        }
        else
            pixmap = _pixmap[0];
    }

    if(!pixmap.isNull())
    {
        QPainter painter(this);
        painter.save();
        painter.setRenderHints(QPainter::Antialiasing, true);
        painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
        QRect headRect(0, 0, _radius * 2, _radius * 2);

        QPainterPath path;
        if(!_showRect)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(220, 220, 220, 200));
            painter.drawEllipse(headRect);

            path.addEllipse(headRect);
            painter.setClipPath(path);


        }
        int w = pixmap.width() / dpi;
        int h = pixmap.height() / dpi;
        painter.drawPixmap((_radius * 2 - w) / 2, (_radius * 2 - h) / 2, w, h, pixmap);

        if(!_showRect)
            painter.fillPath(path, QTalk::StyleDefine::instance().getHeadPhotoMaskColor());

        painter.restore();
        if(_showDot) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(Qt::red));
            painter.drawEllipse( 0, _radius * 2 - 7, 7, 7);
        }
    }
    //
    QLabel::paintEvent(e);
}

void HeadPhotoLab::initMovie() {

    if(_mov)
    {
        _mov->stop();
        delete _mov;
        _mov = nullptr;
    }
    if(_startMovie && !_imagePath.isEmpty())
    {
        QString suff = QTalk::qimage::getRealImageSuffix(_imagePath);
        if(suff == "GIF")
        {
            _mov = new QMovie(_imagePath);
            int count = _mov->frameCount();
            if(count > 0)
            {
                _mov->setParent(this);
                _mov->start();
                _mov->setSpeed(80);
                connect(_mov, &QMovie::frameChanged, [this](){
                    if(_mov)
                        update();
                });
            }
            else
            {
                delete _mov;
                _mov = nullptr;
            }
        }
    }
}

void HeadPhotoLab::startMovie() {
    _startMovie = true;
    initMovie();
}

void HeadPhotoLab::stopMovie() {
    _startMovie = false;
    initMovie();
    if(_mov)
    {
        _mov->stop();
        delete _mov;
        _mov = nullptr;
    }
}

void HeadPhotoLab::setTip(bool tip) {
    _showDot = tip;
    update();
}
