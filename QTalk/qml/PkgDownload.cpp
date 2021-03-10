//
// Created by cc on 2020/11/20.
//

#include "PkgDownload.h"
#include "../../Platform/Platform.h"
#include "../../QtUtil/Utils/utils.h"
#include <QDebug>
#include <QtConcurrentRun>
#include <QApplication>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QMetaEnum>

QString PkgDownload::url() {
    return _url;
}

void PkgDownload::setUrl(const QString &url) {
    _url = url;
}

QString PkgDownload::setPkgPath() {
    return _pkgPath;
}

void PkgDownload::pkgPath(const QString &path) {
    _pkgPath = path;
}

void PkgDownload::start() {
    if(_url.isEmpty()) {
        emit error("invalid url");
        return;
    }
    if(_pkgPath.isEmpty()) {
        _pkgPath = QString("%1/updater/SetUp.%2")
                .arg(PLAT.getAppdataRoamingPath().data()).arg(QTalk::utils::getFileSuffix(_url.toStdString()).data());
    }

    QFileInfo setupFileInfo(_pkgPath);
    if(!QFile::exists(setupFileInfo.absolutePath())) {
    	QDir(setupFileInfo.absolutePath()).mkpath(setupFileInfo.absolutePath());
    }

    _pFile = new QFile(_pkgPath);
    if(!_pFile->open(QIODevice::WriteOnly)) {
        emit error("write file error. ");
        return;
    }
    _pAccessManager = new QNetworkAccessManager(this);
    QNetworkRequest req;
    req.setUrl(_url);
    auto *reply = _pAccessManager->get(req);
    _isDownloading = true;
    connect(_pAccessManager, &QNetworkAccessManager::finished, this, &PkgDownload::replyFinished);
    connect(reply, &QNetworkReply::downloadProgress, this, &PkgDownload::downloadProgress);
    connect(reply, &QNetworkReply::readyRead, [this, reply](){
        _pFile->write(reply->readAll());
    });
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onHttpError(QNetworkReply::NetworkError)));
}

void PkgDownload::replyFinished(QNetworkReply *reply) {
    auto code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool success = reply->error() == QNetworkReply::NoError;
    if (success) {
        _pFile->flush();
        _pFile->close();
    } else {
        _pFile->remove();
    }
    delete _pFile;
    _pFile = nullptr;

    emit finished(success);
}

void PkgDownload::openPkg() {
    QtConcurrent::run([this](){
#if defined(Q_OS_WIN)
        QProcess::startDetached(_pkgPath, QStringList());
#elif defined(Q_OS_MAC)
        QStringList params;
        params << _pkgPath;
        QProcess::execute("open", params);
#elif defined(Q_OS_LINUX)
        QFileInfo info(_pkgPath);
        QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
#endif
        QApplication::exit(0);
    });
}

bool PkgDownload::isDownloading() {
    return _isDownloading;
}

void PkgDownload::clearPkg() {
    if(_pkgPath.isEmpty()) {
        auto dirPath = QString("%1/updater")
                .arg(PLAT.getAppdataRoamingPath().data());

        QDir dir(dirPath);
        for(const auto& it : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files)) {
            it.dir().remove(it.absoluteFilePath());
        }
    }
    else {
        QFile::remove(_pkgPath);
    }
}

void PkgDownload::onHttpError(QNetworkReply::NetworkError err) {
    auto metaEnum = QMetaEnum::fromType<QNetworkReply::NetworkError>();
    emit error(QString("http error %1").arg(metaEnum.valueToKey(err)));
}

//
QString PkgDownload::getPkgMd5() {

    if(_pkgPath.isEmpty()) {
        _pkgPath = QString("%1/updater/SetUp.%2")
                .arg(PLAT.getAppdataRoamingPath().data()).arg(QTalk::utils::getFileSuffix(_url.toStdString()).data());
    }

    QFile localFile(_pkgPath);
    if(!QFile::exists(_pkgPath)) {
//        emit error("could not find the file");
        return "";
    }

    if(!localFile.open(QIODevice::ReadOnly)) {
//        emit error("could not open the file");
        return "";
    }
    QCryptographicHash ch(QCryptographicHash::Md5);

    auto fileSize = localFile.size();
    qint64 readSize = 0;
    QByteArray buf;
    while (readSize < fileSize) {
        buf = localFile.read(4096);
        ch.addData(buf);
        readSize += buf.size();
        buf.resize(0);
    }
    localFile.close();
    return ch.result().toHex().constData();
}