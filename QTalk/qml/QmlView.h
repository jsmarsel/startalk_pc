//
// Created by cc on 2020/7/10.
//

#ifndef QTALK_V2_QMLVIEW_H
#define QTALK_V2_QMLVIEW_H

#include <QQuickWidget>

class QmlView : public QQuickWidget {
    Q_OBJECT
public:
    QmlView();

public:
    void init();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE QString getCkey();
    Q_INVOKABLE QString getUserName(const QString& id);
    Q_INVOKABLE QString getUserHead(const QString& id);
    Q_INVOKABLE QString getHttpHost();
    Q_INVOKABLE QString getSelfUserId();
    Q_INVOKABLE QString getSelfXmppId();
    Q_INVOKABLE QString getClientVersion();
    Q_INVOKABLE QString getGlobalVersion();
    Q_INVOKABLE QString getPlatformStr();
    Q_INVOKABLE QString getExecuteName();
    Q_INVOKABLE int getTestChannel();
    Q_INVOKABLE QString getVersionId();
    Q_INVOKABLE QString getOSVersion();
    Q_INVOKABLE bool debuging();
    Q_INVOKABLE void setUpdateTipVisible(bool);

public slots:
    void showWnd();

private slots:
    void onLoadStatusChanged(QQuickWidget::Status);

signals:
    void checkUpdate();
    void sgShowUpdateClientLabel(bool);
};


#endif //QTALK_V2_QMLVIEW_H
