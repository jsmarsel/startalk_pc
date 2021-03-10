//
// Created by cc on 2020/11/20.
//

#ifndef QTALK_V2_PKGDOWNLOAD_H
#define QTALK_V2_PKGDOWNLOAD_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QFile>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QIODevice>

class PkgDownload : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QString fileName READ setPkgPath WRITE pkgPath)
    Q_PROPERTY(bool downloading READ isDownloading)
    using QObject::QObject;

public:
    QString url();
    void setUrl(const QString& url);

    void pkgPath(const QString &path);
    QString setPkgPath();

    bool isDownloading();

private:
    QString   _url;
    QString   _pkgPath;
    QFile     *_pFile{};
    QNetworkAccessManager *_pAccessManager{};
    bool      _isDownloading{false};

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(bool success);
    void error(const QString& error);

protected slots:
    void start();
    void replyFinished(QNetworkReply*reply);
    void openPkg();
    void clearPkg();
    void onHttpError(QNetworkReply::NetworkError error);
    QString getPkgMd5();
};


#endif //QTALK_V2_PKGDOWNLOAD_H
