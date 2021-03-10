#ifndef HEADPHOTOBTN_H
#define HEADPHOTOBTN_H

#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QMap>
#include <QPixmap>
#include <QMutexLocker>
#include <QMovie>
#include "customui_global.h"

class CUSTOMUISHARED_EXPORT HeadPhotoLab : public QLabel
{
    using QLabel::QLabel;

public:
    explicit HeadPhotoLab(
            QString  strHeadPath = QString(),
            int radius = 0,
            bool isGrey = false,
            bool startMovie = false,
            bool showRect = false,
            QWidget *parent = nullptr);

public:
    void setHead(const QString& headPath,
            int radius,
            bool isGrey = false,
            bool startMovie = false,
            bool showRect = false);

    void startMovie();
    void stopMovie();
    void setTip(bool tip);

protected:
    void paintEvent(QPaintEvent* e) override;
    void initMovie();

private:
    QString _imagePath;
    int     _radius{0};
    bool    _isGrey{false};
    bool    _startMovie{false};
    bool    _showRect{false};

private:
    QMovie* _mov{nullptr};
    QVector<QPixmap> _pixmap;

    bool    _showDot{false};
};

#endif // HEADPHOTOBTN_H
