﻿//
// Created by cc on 19-1-16.
//

#include "qimage.h"
#include <QPainter>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <iostream>
#include <QImageReader>
#include <QBuffer>
#include <QDebug>
#include <QPainterPath>
#include "../../QtUtil/Utils/utils.h"
#include "../../QtUtil/Utils/Log.h"

namespace QTalk {

    static QMap<QString, QPixmap>                _allPixmap;
    static QMap<QString, QMap<QString, QPixmap>> _scanPixmap;
    static QMutex _mutex{};
    static int    _dpi = 0;

    //
    QPixmap qimage::loadImage(const QString &srcPath, bool save, bool scaled, int width, int height) {

        if (srcPath.isEmpty())
            return QPixmap();

        QFileInfo info(srcPath);
        if (!info.exists() || info.isDir())
            return QPixmap();
        QPixmap dest;
        if (_allPixmap.contains(srcPath)) {
            QMutexLocker locker(&_mutex);
            dest = _allPixmap[srcPath];

            if (scaled)
            {
                QString key = QString("%1-%2").arg(width).arg(height);
                if(_scanPixmap.contains(srcPath) && _scanPixmap[srcPath].contains(key))
                {
                    dest = _scanPixmap[srcPath][key];
                }
                else
                {
                    dest = scaledPixmap(dest, width, height);
                    _scanPixmap[srcPath][key] = dest;
                }
            }
        } else {
            dest = QPixmap(srcPath);
            if (dest.isNull()) {
                QString suffix = getRealImageSuffix(srcPath);
                if (suffix.isEmpty())
                    return QPixmap();
                QString newPath = QString("%1/%2.%3").arg(info.absolutePath()).arg(info.baseName()).arg(suffix);
                if (!QFile::exists(newPath))
                    QFile::copy(srcPath, newPath);
                dest = QPixmap(newPath);
                if (dest.isNull())
                    return QPixmap();
            }
            if (save)
            {
                QMutexLocker locker(&_mutex);
                _allPixmap[srcPath] = dest;
            }

            if (scaled)
            {
                QString key = QString("%1-%2").arg(width).arg(height);
                dest = scaledPixmap(dest, width, height);
                if (save)
                    _scanPixmap[srcPath][key] = dest;
            }
        }
        return dest;
    }

    QPixmap qimage::loadCirclePixmap(const QString &srcPath, const int &radius, bool isGrey) {

        QPixmap dest = loadImage(srcPath, true);
        if (dest.isNull())
            return dest;

        if (isGrey)
            dest = generateGreyPixmap(dest);
        dest = generatePixmap(dest, radius);
        return dest;
    }

    QPixmap qimage::scaledPixmap(const QPixmap &src, int width, int height, bool model) {
//	    if(src.width() < width && src.height() < height)
//            return src;
        return src.scaled(width, (height == 0 ? width : height),
                          model ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio,
                          Qt::SmoothTransformation);
    }

    QPixmap qimage::generatePixmap(const QPixmap &src, const int &radius) {

        if (src.isNull()) {
            return src;
        }

        QPixmap pixmap = scaledPixmap(src, radius * 2);

        QPixmap dest(2 * radius, 2 * radius);
        dest.fill(Qt::transparent);
        QPainter painter(&dest);
        painter.setRenderHints(QPainter::Antialiasing, true);
        painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
        QPainterPath path;
        path.addEllipse(0, 0, 2 * radius, 2 * radius);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 2 * radius, 2 * radius, pixmap);

        return dest;
    }

    QPixmap qimage::generateGreyPixmap(const QPixmap &src) {
        QImage origin = src.toImage();
        auto *newImage = new QImage(origin.width(), origin.height(), QImage::Format_ARGB32);
        QColor oldColor;
        for (int x = 0; x < newImage->width(); x++) {
            for (int y = 0; y < newImage->height(); y++) {
                oldColor = QColor(origin.pixel(x, y));
                int average = (oldColor.red() + oldColor.green() + oldColor.blue()) / 3;
                newImage->setPixel(x, y, qRgba(average, average, average, 240));
            }
        }
        QPixmap greypix(QPixmap::fromImage(*newImage));
        delete newImage;
        return greypix;
    }

    QString qimage::getRealImageSuffix(const QString &filePath) {
        QFile file(filePath);
        // get image format
        QFile f(filePath);
        QString format;
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray arr = f.read(256);
            QBuffer buffer;
            buffer.setData(arr);
            buffer.open(QIODevice::ReadOnly);
            format = QImageReader::imageFormat(&buffer);
            buffer.close();
            f.close();
        }
        return format.toUpper();
    }

    int qimage::dpi() {

        if (_dpi != 0)
            return _dpi;

        QScreen *sreen = QGuiApplication::primaryScreen();
        int dpi = (int) sreen->physicalDotsPerInch();

        if (dpi <= 100) {
            _dpi = 1;
        } else if (dpi <= 120) {
            _dpi = 2;
        } else if (dpi <= 144) {
            _dpi = 3;
        } else {
            _dpi = 4;
        }

        return _dpi;
    }

    QString qimage::getRealImagePath(const QString &filePath) {
        QFileInfo imageInfo(filePath);
        if (!imageInfo.exists())
            return QString();

        QString suffix = getRealImageSuffix(filePath);
        if (suffix.isEmpty())
            return QString();

        if (imageInfo.suffix().toUpper() == suffix)
            return filePath;
        else {
            QString newPath = QString("%1/%2.%3").arg(imageInfo.absolutePath()).arg(imageInfo.baseName()).arg(suffix);
            if (!QFile::exists(newPath))
                QFile::copy(filePath, newPath);

            return newPath;
        }
    }

    QString qimage::getGifImagePath(const QString &srcPath, qreal &width, qreal &height) {
	    if(srcPath.isEmpty())
            return srcPath;

        QFileInfo imageInfo(srcPath);
        QString dstPath = QString("%1/%2_gif.png").arg(imageInfo.absolutePath(), imageInfo.baseName());
        if(!QFile::exists(dstPath))
        {
            if(width < 50 || height < 30)
            {
                width = height = 50;
            }


            QPixmap pixmap(srcPath);
            QPixmap dest(width, height);
            dest.fill(Qt::transparent);
            QPainter painter(&dest);
            painter.setRenderHints(QPainter::Antialiasing, true);
            painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
            painter.drawPixmap(0, 0, width, height, pixmap);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(100, 100, 100, 200));
            QRect rect(width - 45, height - 30, 40, 24);
            painter.drawRoundedRect(rect, 12, 12);
            painter.setPen(QColor(255, 255, 255));
            QFont font(painter.font());
//            font.setBold(true);
            font.setPixelSize(13);
            painter.setFont(font);
            painter.drawText(rect, Qt::AlignCenter, "GIF");
            if(!dest.save(dstPath, "PNG"))
                return srcPath;
        }
        return dstPath;
    }

    QString qimage::getGifImagePathNoMark(const QString &srcPath) {
        if(srcPath.isEmpty())
            return srcPath;

        QFileInfo imageInfo(srcPath);
        QString dstPath = QString("%1/%2.png").arg(imageInfo.absolutePath(), imageInfo.baseName());
        if(!QFile::exists(dstPath))
        {
            QPixmap pixmap(srcPath);
            if(!pixmap.isNull() && !pixmap.save(dstPath, "PNG"))
                return srcPath;
        }
        return dstPath;
    }
}