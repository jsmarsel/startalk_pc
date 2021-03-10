//
// Created by cc on 2020/7/10.
//

#include "QmlView.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QtConcurrent>
#include "../MessageManager.h"
#include "../../Platform/Platform.h"
#include "../../Platform/dbPlatForm.h"
#include "../../Platform/NavigationManager.h"
#include "PkgDownload.h"
#include "../CustomUi/QtMessageBox.h"

static const char* mainQml = "qrc:/qml/main.qml";

QmlView::QmlView() {
    setFixedSize(484, 336);
//#ifdef _WINDOWS
    Qt::WindowFlags flags = Qt::Dialog 
                            | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint
                            | Qt::WindowFullscreenButtonHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint;
    setWindowFlags(flags);
//#endif
    setAttribute(Qt::WA_QuitOnClose, false);
    qmlRegisterType<PkgDownload>("PkgDownload",1,0,"PkgDownload");
    rootContext()->setContextProperty("viewObj", this);

    connect(this, &QmlView::statusChanged, this, &QmlView::onLoadStatusChanged);
}

void QmlView::init() {
    auto *eng = this->engine();
    eng->rootContext()->setContextProperty("$QmlEngine", this);
}

void QmlView::clearCache() {
    this->engine()->clearComponentCache();
}

void QmlView::showWnd() {
//    QtConcurrent::run([this](){
//        auto info = QTalkMsgManager::getPostList(0);
//        emit postList(info.data());
//    });

    switch (this->status()) {
        case QQuickWidget::Error:
            clearCache();
        case QQuickWidget::Null: {
            QUrl url(mainQml);
            this->setSource(url);
            break;
        }
        case QQuickWidget::Ready: {
            this->show();
            this->raise();
            emit checkUpdate();
        }
        case QQuickWidget::Loading: {
            break;
        }
    }
}

void QmlView::onLoadStatusChanged(QQuickWidget::Status status) {

    switch (status) {
        case QQuickWidget::Error: {
            QtMessageBox::warning(nullptr, tr("警告"), tr("窗口加载失败"));
            this->setVisible(false);
            return;
        }
        case QQuickWidget::Ready: {
            this->show();
            this->raise();
            break;
        }
        case QQuickWidget::Loading:
        case QQuickWidget::Null: {
            break;
        }
    }
}

void QmlView::setUpdateTipVisible(bool visible) {
    emit sgShowUpdateClientLabel(visible);
}

QString QmlView::getCkey() {
    return PLAT.getClientAuthKey().data();
}

QString QmlView::getUserName(const QString &id) {
    return QTalk::getUserName(id.toStdString()).data();
}


QString QmlView::getUserHead(const QString &id) {
    auto info = DB_PLAT.getUserInfo(id.toStdString());
    if(info) {
        return info->HeaderSrc.data();
    }
    return "";
}

QString QmlView::getHttpHost() {
    return NavigationManager::instance().getHttpHost().data();
}

QString QmlView::getSelfUserId() {
    return PLAT.getSelfUserId().data();
}

QString QmlView::getSelfXmppId() {
    return PLAT.getSelfXmppId().data();
}

QString QmlView::getClientVersion() {
    return PLAT.getClientVersion().data();
}

QString QmlView::getGlobalVersion() {
    return PLAT.getGlobalVersion().data();
}

QString QmlView::getPlatformStr() {
    return PLAT.getPlatformStr().data();
}

QString QmlView::getExecuteName() {
    return QString(PLAT.getExecuteName().data()).toLower();
}

int QmlView::getTestChannel() {
    return AppSetting::instance().getTestchannel();
}

QString QmlView::getVersionId() {
    return QString("%1_%2").arg(GLOBAL_INTERNAL_VERSION).arg( build_time());
}

QString QmlView::getOSVersion() {
    return PLAT.getOSVersion().data();
}

bool QmlView::debuging() {
#ifdef QT_DEBUG
    return true;
#else
    return false;
#endif
}