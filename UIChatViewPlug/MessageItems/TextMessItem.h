﻿#ifndef RICHTEXTMESSITEM_H
#define RICHTEXTMESSITEM_H

#include "MessageItemBase.h"

#include <QLabel>
#include <QTextCursor>
#define MAX_CONTENT_WIDTH 1024
#define DEM_AT_HTML "<span style=\"color:#FF4E3F;\">%1</span>"

class TextBrowser;
class TextMessItem : public MessageItemBase
{
    Q_OBJECT
public:
    explicit TextMessItem(const StNetMessageResult &msgInfo, QWidget *parent = Q_NULLPTR);

public:
    QSize textWdtSize();
    qreal getRealString(const QString& src, qreal& lastLineWidth);

public:
    QSize itemWdtSize() override;
    void setMessageContent(bool delMov = true);
    void copyText();
    // 右键是否在图片上
    bool isImageContext();
    QString getImageLink();
    //
    void onImageDownloaded(const QString& link);
    void setStTextMessages(const std::vector<StTextMessage>& msgs) { _msgs = QVector<StTextMessage>::fromStdVector(msgs); };

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
    bool event(QEvent* e) override ;

private:
    void init();
    void initLayout();
    void initSendLayout();
    void initReceiveLayout();


private slots:
    void onAnchorClicked(const QUrl& url);
    void onImageClicked(int index);
    void onMovieChanged(int );

private:
    TextBrowser * _textBrowser;

    qreal m_textWidth;
    qreal m_lineHeight;

    QSize _headPixSize;
    QMargins _mainMargin;
    QMargins _leftMargin;
    QMargins _rightMargin;
    QMargins _contentMargin;
    int _mainSpacing;
    int _leftSpacing;
    int _rightSpacing;
    int _contentSpacing;
    int _nameLabHeight;

private:
    QVector<StTextMessage> _msgs;

    QString _strImagePath;

// gif
private:
    std::map<QString, QMovie*> _mapMovies;
    qint64 _paintTime = 0;
};

#endif // RICHTEXTMESSITEM_H
