﻿//
// Created by cc on 2019-02-13.
//

#include "WebView.h"
#include <QDebug>
#include <QWebEngineSettings>
#include "WebEnginePage.h"
#include "WebJsObj.h"
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QWebEngineUrlRequestInterceptor>
#include <QFileDialog>
#include "../QtUtil/Utils/Log.h"
#include "WebEngineUrlRequestInterceptor.h"
#include <QWebEngineHistory>

WebView::WebView(QWidget* parent)
    : QFrame(parent)
{
    _pWebView = new QWebEngineView(this);
	_pWebCannel = new QWebChannel(this);
    _downloadWgt = new DownLoadWgt(this);
//	_pWebView->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    _pWebPage = new WebEnginePage(_pWebView);
    _pWebView->setPage(_pWebPage);
    auto * lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->addWidget(_pWebView, 1);
    lay->addWidget(_downloadWgt, 0);

    auto* profile = _pWebPage->profile();
    QString cachePath = profile->cachePath();
    profile->setHttpCacheType(QWebEngineProfile::NoCache);
    _pWebView->setContextMenuPolicy(Qt::NoContextMenu);
    _downloadWgt->setVisible(false);

    _pWebView->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    _pWebPage->profile()->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
    _pWebPage->profile()->setRequestInterceptor(new WebEngineUrlRequestInterceptor(this));
    _pWebPage->setWebChannel(_pWebCannel);
//    connect(_pWebPage, &WebEnginePage::contentsSizeChanged, [this](const QSizeF &size){
//
//    });

    connect(_pWebPage->profile(), &QWebEngineProfile::downloadRequested, this, &WebView::onDownLoadFile);
    connect(_pWebPage, &WebEnginePage::sgFullScreen, this, &WebView::sgFullScreen);
    connect(_pWebPage, &WebEnginePage::loadFinished, this, &WebView::sgLoadFinished);
    connect(_pWebPage, &WebEnginePage::loadFinished, [this](){
//        qreal zoom = _pWebView->zoomFactor();
//        if(abs(zoom - 0.8) > 0.00001)
//            _pWebView->setZoomFactor(0.8);
    });
    auto cookieStore = _pWebPage->profile()->cookieStore();
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, this, &WebView::sgCookieAdded);
    connect(cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &WebView::sgCookieRemoved);

    QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
}

WebView::~WebView() 
{
    _pWebView->releaseMouse();
    _pWebView->releaseKeyboard();
}

void WebView::setObj(WebJsObj *obj) {
    _pWebCannel->registerObject("client", obj);
    connect(obj, &WebJsObj::runScript, this, &WebView::excuteJs);
    connect(obj, &WebJsObj::sgFullScreen, this, &WebView::sgFullScreen);
    connect(obj, &WebJsObj::closeGroupRoom, this, &WebView::closeVideo);
}

/**
 *
 * @param url
 */
void WebView::loadUrl(const QUrl &url)
{
    if(nullptr != _pWebView)
    {
        _pWebView->setUrl(url);
    }
}

void WebView::startReq(const QWebEngineHttpRequest& req)
{
    if(nullptr != _pWebView)
    {
        auto cookieStore = _pWebPage->profile()->cookieStore();
        cookieStore->loadAllCookies();
        _pWebView->load(req);
    }
}

void WebView::setCookie(const QNetworkCookie& cookie, const QUrl& hostUrl)
{
    if(_pWebView)
    {
        auto cookieStore = _pWebPage->profile()->cookieStore();
        cookieStore->setCookie(cookie, hostUrl);
    }
}

/**
 * 
 */
void WebView::setAgent(const QString & userAgent)
{
    _pWebPage->profile()->setHttpUserAgent(userAgent);
}

/**
 *
 */
void WebView::excuteJs(const QString &js)
{
    _pWebPage->runJavaScript(js);
    qInfo() << js;
    debug_log(js.toStdString());
}

void WebView::onDownLoadFile(QWebEngineDownloadItem *download)
{
    QString path = QFileDialog::getSaveFileName(this, tr("另存为"), download->path());
    if (path.isEmpty())
        return;
    download->setPath(path);
    download->accept();

    _downloadWgt->addDownLoaded(download);
}

void WebView::clearHistory()
{
    _pWebView->history()->clear();
}

void WebView::clearCookieAndCache()
{
    if(_pWebView)
    {
        _pWebPage->profile()->clearHttpCache();
        auto cookieStore = _pWebPage->profile()->cookieStore();
        cookieStore->deleteAllCookies();
    }
}