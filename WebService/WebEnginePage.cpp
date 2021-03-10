//
// Created by cc on 2019-02-18.
//

#include "WebEnginePage.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QWebEngineProfile>
#include "../QtUtil/Utils/Log.h"

WebEnginePage::WebEnginePage(QObject *parent)
        : QWebEnginePage(parent) {

	profile()->setParent(this);
    connect(this, &WebEnginePage::featurePermissionRequested, this, &WebEnginePage::onFeaturePermissionRequested);

//    connect(this, &WebEnginePage::quotaRequested, this, &WebEnginePage::onQuotaRequested);

    connect(this, &WebEnginePage::fullScreenRequested, this, &WebEnginePage::onFullScreenRequested);

//    connect(this, &WebEnginePage::authenticationRequired, this, &WebEnginePage::onAuthenticationRequired);


//    void fullScreenRequested(QWebEngineFullScreenRequest fullScreenRequest);
//    void quotaRequested(QWebEngineQuotaRequest quotaRequest);
//    void registerProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);
}

WebEnginePage::~WebEnginePage() = default;

inline QString questionForFeature(QWebEnginePage::Feature feature)
{
    switch (feature) {
//        case QWebEnginePage::Geolocation:
//            return (tr("允许 %1 访问您的位置信息？"));
//        case QWebEnginePage::MediaAudioCapture:
//            return (tr("允许 %1 访问您的麦克风？"));
//        case QWebEnginePage::MediaVideoCapture:
//            return (tr("允许 %1 访问您的摄像头？"));
//        case QWebEnginePage::MediaAudioVideoCapture:
//            return (tr("允许 %1 访问您的麦克风和网络摄像头？"));
//        case QWebEnginePage::MouseLock:
//            return (tr("允许 %1 锁定鼠标光标？"));
//        case QWebEnginePage::DesktopVideoCapture:
//            return (tr("允许 %1 捕获桌面的视频？"));
//        case QWebEnginePage::DesktopAudioVideoCapture:
//            return (tr("允许 %1 捕获桌面的音频和视频？"));
        case QWebEnginePage::Notifications:
            return QString();
    }
    return QString();
}

void WebEnginePage::onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature) {
//    QString title = tr("权限申请");
//    QString question = questionForFeature(feature).arg(securityOrigin.host());
//    if (!question.isEmpty() && QMessageBox::question(view()->window(), title, question) == QMessageBox::Yes)
//        setFeaturePermission(securityOrigin, feature, PermissionGrantedByUser);
//    else
//        setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);

    switch (feature) {
        case QWebEnginePage::Geolocation:
//            return (tr("允许 %1 访问您的位置信息？"));
        case QWebEnginePage::MediaAudioCapture:
//            return (tr("允许 %1 访问您的麦克风？"));
        case QWebEnginePage::MediaVideoCapture:
//            return (tr("允许 %1 访问您的摄像头？"));
        case QWebEnginePage::MediaAudioVideoCapture:
//            return (tr("允许 %1 访问您的麦克风和网络摄像头？"));
        case QWebEnginePage::MouseLock:
//            return (tr("允许 %1 锁定鼠标光标？"));
#ifndef _LINUX
        case QWebEnginePage::DesktopVideoCapture:
//            return (tr("允许 %1 捕获桌面的视频？"));
	case QWebEnginePage::DesktopAudioVideoCapture:
//            return (tr("允许 %1 捕获桌面的音频和视频？"));
#endif
            setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionGrantedByUser);
        case QWebEnginePage::Notifications:
        default:
            setFeaturePermission(securityOrigin, feature, QWebEnginePage::PermissionDeniedByUser);
    }
}

//void WebEnginePage::onQuotaRequested(QWebEngineQuotaRequest quotaRequest) {
//
//    debug_log("onQuotaRequested");
//
//}

//void WebEnginePage::registerProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request) {
//
//}

void WebEnginePage::onFullScreenRequested(QWebEngineFullScreenRequest fullScreenRequest) {
    fullScreenRequest.accept();
    emit sgFullScreen();
    debug_log("onFullScreenRequested");
}

bool WebEnginePage::certificateError(const QWebEngineCertificateError &certificateError) {
    return true;
}

QWebEnginePage *WebEnginePage::createWindow(QWebEnginePage::WebWindowType type) {
    // todo 解决 _blank 问题
    return QWebEnginePage::createWindow( type);
}

QStringList WebEnginePage::chooseFiles(QWebEnginePage::FileSelectionMode mode, const QStringList &oldFiles,
                                       const QStringList &acceptedMimeTypes) {

    QString filter;
    for(const auto& it : acceptedMimeTypes)
    {
        if(it.contains("image"))
            filter += "*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.gif *.GIF ";
        else {
            if(it == ("application/pdf"))
                filter += "*.pdf *.PDF ";
            else if(it == ("application/msword"))
                filter += "*.doc *.DOC ";
            else if(it == ("application/vnd.ms-excel"))
                filter += "*.xls *.xlt *.xla *.XLS *.XLT *.XLA ";
            else if(it == ("application/vnd.ms-powerpoint"))
                filter += "*.ppt *.pot *.pps *.ppa *.PPT *.POT *.PPS *.PPA ";
            else if(it == ("application/vnd.openxmlformats-officedocument.wordprocessingml.document"))
                filter += "*.docx *.DOCX ";
            else if(it == ("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"))
                filter += "*.xlsx *.XLSX ";
            else if(it == ("application/vnd.openxmlformats-officedocument.presentationml.slideshow"))
                filter += "*.ppsx *.PPSX ";
        }
    }
    if(!acceptedMimeTypes.isEmpty())
        filter.append(";;");
    filter += "All Files(*.*)";

    auto list = QFileDialog::getOpenFileNames((QWidget*)parent(), tr("打开"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                                              filter);
    return list;
}


//void WebEnginePage::onAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator) {
//    debug_log("onAuthenticationRequired");
//}

//void WebEnginePage::proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator,
//                                                const QString &proxyHost) {
//
//}
//
//void WebEnginePage::renderProcessTerminated(RenderProcessTerminationStatus terminationStatus, int exitCode) {
//
//}
