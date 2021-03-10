#include <iostream>
#include <QSettings>
#include <QTextCodec>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include "../Platform/SystemDefine.h"

#ifdef _WINDOWS
#include <client/windows/handler/exception_handler.h>
#elif _LINUX
#include <client/linux/handler/exception_handler.h>
#else
#include <client/mac/handler/exception_handler.h>
#include <QProcess>
#endif
#include "QTalkApp.h"

#if defined(_WINDOWS)
bool minidumpCB(const wchar_t *dump_path, const wchar_t *id, void *context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded) {
	qWarning() << "id" << QString::fromStdWString(id) << "dump Path: " << QString::fromStdWString(dump_path);
#elif defined(_LINUX)
bool minidumpCB(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
    const char* dump_path =  descriptor.path();
#else
bool minidumpCB(const char* dump_path, const char* id, void* context, bool succeeded) {
	qWarning() << "id" << id << "dump Path: " << dump_path;
#endif
    return succeeded;
}

int main(int argc, char *argv[]) {

    qint64 buildTime = build_time() * 1000;
    QString id = QString("%1_%2").arg(GLOBAL_INTERNAL_VERSION)
            .arg(buildTime);

    if(argc == 2 && "--build-time" == std::string(argv[1]))
    {
        std::cout << id.toStdString() << std::endl;
        exit(0);
    }
    unsigned short pid = QCoreApplication::applicationPid();
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", std::to_string(pid).data());
#ifndef QT_DEBUG
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    // high dpi
    //QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    // 缩放因子
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qunar.com", "");
    if(settings.contains("QT_SCALE_ENABLE_V2"))
        settings.remove("QT_SCALE_ENABLE_V2");

    bool scale_enable = false;
    if(settings.contains("QT_SCALE_ENABLE"))
        scale_enable = settings.value("QT_SCALE_ENABLE").toBool();
    else
        settings.setValue("QT_SCALE_ENABLE", scale_enable);
    if (scale_enable) {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }
    else
	    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toLocal8Bit();
#ifndef _DEBUG
    QString dumpLocation = QString("%1/StarTalk/logs/%2").arg(appData, id);

    // mkdir
    if (!QFile::exists(dumpLocation))
    {
        QDir dir(appData);
        dir.mkpath(dumpLocation);
    }

#if defined(_WINDOWS)
    google_breakpad::ExceptionHandler eh(dumpLocation.toStdWString(), nullptr, minidumpCB, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
#elif defined(_LINUX)
    google_breakpad::MinidumpDescriptor descriptor(dumpLocation.toStdString());
    google_breakpad::ExceptionHandler eh(descriptor, nullptr, minidumpCB, nullptr, true, -1);
#else
    google_breakpad::ExceptionHandler eh(
            dumpLocation.toStdString(),
            nullptr,
            minidumpCB,
            nullptr,
            true,
            nullptr);
#endif
#endif
    QTalkApp a(argc, argv);

    return 0;
}
