#include "QTalkApp.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>
#include <thread>
#include <sstream>
#include <QEvent>
#include <iostream>
#include <QMouseEvent>
#include <iostream>
#include <QSettings>
#include <QTranslator>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QFontDatabase>
#include <curl/curl.h>
#include <QtConcurrent>
#include <QSslSocket>
#include <QThreadPool>
#include "GlobalManager.h"
#include "../LogicManager/LogicManager.h"
#include "../Platform/Platform.h"
#include "../QtUtil/Utils/Log.h"
#include "MainWindow.h"
#include "../CustomUi/QtMessageBox.h"
#include "MessageManager.h"
#include "../EventBus/EventBus.h"

#ifdef _WINDOWS
    #define UNICODE
#else
    #ifdef _MACOS
        #include "MacApp.h"

    #endif
#endif

extern bool _sys_run {false};
QTalkApp::QTalkApp(int argc, char *argv[])
    : QApplication(argc, argv)
{
    _sys_run = true;
    QString strExcutePath = argv[0];
    //
    {
//#ifdef Q_OS_WIN
//        auto name = qApp->applicationName().toLower();
//        QSettings qSettings("HKEY_CLASSES_ROOT\\" + name, QSettings::NativeFormat);
//
//        if (!qSettings.value(".").isValid())
//        {
//            QProcess qProcess;
//            QStringList params;
//            params << "ExecName=" + name;
//            qProcess.startDetached(QFileInfo(strExcutePath).absolutePath() + "/WinHelper.exe", params);
//        }
//
//#endif
    }
    //
    auto maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    QThreadPool::globalInstance()->setMaxThreadCount(qMin(maxThreadCount * 2, 16));
    strExcutePath = QFileInfo(strExcutePath).absolutePath();
    // 启动日志
    initLogSys();
    //
#ifdef _MACOS
    MacApp::initApp();
#endif
    //设置当前路径
#if defined(_WINDOWS) || defined(_MACOS)
    QString curPath = QApplication::applicationDirPath();
    QDir::setCurrent(curPath);
#endif
    curl_global_init(CURL_GLOBAL_ALL);
    // 加载全局单利platform
    PLAT.setMainThreadId();
    PLAT.setExecutePath(strExcutePath.toStdString());
    PLAT.setExecuteName(QApplication::applicationName().toStdString());
    PLAT.setProcessId(QApplication::applicationPid());
    QString systemStr = QSysInfo::prettyProductName();
    QString productType = QSysInfo::productType();
    QString productVersion = QSysInfo::productVersion();
    PLAT.setOSInfo(systemStr.toStdString());
    PLAT.setOSProductType(productType.toStdString());
    PLAT.setOSVersion(productVersion.toStdString());
    // Ui管理单例
    _pUiManager = GlobalManager::instance();
    int language = AppSetting::instance().getLanguage();

    if(QLocale::AnyLanguage == language)
    {
        language = QLocale::system().language();
        AppSetting::instance().setLanguage(language);
    }

    switch (language)
    {
        case QLocale::English:
            {
                // 加载翻译文件
                static QTranslator wgtQm;
                wgtQm.load(":/QTalk/config/qtalk_en.qm");
                QApplication::installTranslator(&wgtQm);
                break;
            }

        case QLocale::Korean:
            {
                //
                static QTranslator qtGloble;
                qtGloble.load(":/QTalk/config/qt_ko.qm");
                QApplication::installTranslator(&qtGloble);
                static QTranslator wgtQm;
                wgtQm.load(":/QTalk/config/qtalk_ko.qm");
                QApplication::installTranslator(&wgtQm);
                break;
            }

        case QLocale::Chinese:
        default:
            {
                // 加载翻译文件
                static QTranslator qtGloble;
                qtGloble.load(":/QTalk/config/qt_zh_CN.qm");
                QApplication::installTranslator(&qtGloble);
                //
                static QTranslator wgtQm;
                wgtQm.load(":/QTalk/config/widgets_zh_CN.qm");
                QApplication::installTranslator(&wgtQm);
                break;
            }
    }

    //
    initTTF();
    // 加载主Qss文件
    _pUiManager->setStylesForApp();
    // 业务管理单例
    _pLogicManager = LogicManager::instance();
    //
    setWindowIcon(QIcon(":/QTalk/image1/StarTalk.png"));
    QApplication::setStyle("fusion");
    _pMainWnd = new MainWindow;
#ifdef _MACOS
    // 窗口调整
    MacApp::AllowMinimizeForFramelessWindow((QWidget *)_pMainWnd);
    // 多开
    connect(_pMainWnd, &MainWindow::sgRunNewInstance, []()
    {
        QStringList params;
        const QString &cmd = QApplication::applicationFilePath();
        QStringList arguments;
#ifdef _STARTALK
        arguments << "START_BY_STARTER=YES" << "AUTO_LOGIN=OFF";
#else
        arguments << "AUTO_LOGIN=OFF";
#endif
        QProcess::startDetached(cmd, arguments);
    });
#endif
    bool enableAutoLogin = true;
    QString loginMsg;
    //
    {
        int index = 0;
        QMap<QString, QString> params;

        while (index < argc)
        {
            QString tmpParam(argv[index++]);

            if(tmpParam.contains("=") && tmpParam.count("=") == 1)
            {
                params.insert(tmpParam.section("=", 0, 0),
                              tmpParam.section("=", 1, 1));
            }
        }

        if(params.contains("AUTO_LOGIN"))
            enableAutoLogin = params["AUTO_LOGIN"] == "ON";

        if(params.contains("MSG"))
            loginMsg = params["MSG"];
    }
    _pMainWnd->InitLogin(enableAutoLogin, loginMsg);
    _pMainWnd->initSystemTray();
    //
    connect(this, &QApplication::applicationStateChanged, this, &QTalkApp::onApplicationStateChange);
    // exec
    exec();
}

QTalkApp::~QTalkApp()
{
    EventBus::clearHandle();

    if (nullptr != _pMainWnd)
    {
        delete _pMainWnd;
        _pMainWnd = nullptr;
    }

    QTalk::logger::exit();
}


QString strlogPath;
QString strqLogPath;

void LogMsgOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if(!_sys_run)
        return;

    QString log;
    QString localMsg(msg);
    localMsg.replace("\"", "");
    QString localPath(context.file);
    localPath.replace("\\", "/");
    log.append(QString("[%1] ").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));

    switch (type)
    {
        case QtDebugMsg:
            log.append(QString("[Debug] "));
//            return;
            break;

        case QtInfoMsg:
            log.append(QString("[Info] "));
            break;

        case QtWarningMsg:
            log.append(QString("[Warning] "));
            break;

        case QtCriticalMsg:
            log.append(QString("[Critical] "));
            break;

        case QtFatalMsg:
            log.append(QString("[Fatal] "));
            break;
    }

#ifdef _DEBUG
#ifndef _WINDOWS

    switch (type)
    {
        case QtInfoMsg:
            std::cout << "\033[1m\033[34m" << localMsg.toStdString() << "\033[0m" << std::endl;
            break;

        case QtWarningMsg:
            std::cout << "\033[1m\033[33m" << localMsg.toStdString() << "\033[0m" << std::endl;
            break;

        case QtCriticalMsg:
            std::cout << "\033[1m\033[31m" << localMsg.toStdString() << "\033[0m" << std::endl;
            break;

        case QtFatalMsg:
            std::cout << "\033[1m\033[35m" << localMsg.toStdString() << "\033[0m" << std::endl;
            break;

        default:
        case QtDebugMsg:
            std::cout << "\033[1m\033[36m" << localMsg.toStdString() << "\033[0m" << std::endl;
            break;
    }

#else
    std::cout << localMsg.toStdString() << std::endl;
#endif
#endif
    //
    log.append(QString("%1 \n%2(%3) %4\n").arg(localMsg, context.file).arg(context.line).arg(context.function));
    QFile file(strqLogPath);

    if (!file.isOpen() && !file.open(QIODevice::ReadWrite | QIODevice::Append))
        QString erinfo = file.errorString();
    else
    {
        static QTalk::util::spin_mutex sm;
        std::lock_guard<QTalk::util::spin_mutex> lock(sm);
        file.write(log.toUtf8());
        file.flush();
    }
}

/**
  * @函数名   initLogSys
  * @功能描述 启动日志系统
  * @参数
  * @author   cc
  * @date     2018/10/11
  */
void QTalkApp::initLogSys()
{
    QDateTime curDateTime = QDateTime::currentDateTime();
    QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    strlogPath = QString("%1/logs/%2/")
                 .arg(appdata)
                 .arg(curDateTime.toString("yyyy-MM-dd"));
    strqLogPath = QString("%1/%2_qt.log").arg(strlogPath).arg(curDateTime.toString("yyyy-MM-dd-hh-mm-dd"));
    QDir logDir(strlogPath);

    if (!logDir.exists())
        logDir.mkpath(strlogPath);

    std::string fileName = std::string(strlogPath.toLocal8Bit());
    QTalk::logger::initLog(fileName,
#ifdef _DEBUG
                           QTalk::logger::LEVEL_DEBUG
#else
                           QTalk::logger::LEVEL_INFO
#endif
                          );
    //
    qInstallMessageHandler(LogMsgOutput);
    //
    info_log("system started . client version:{0}", PLAT.getClientVersion());
    qInfo() << "system started . client version: " << PLAT.getClientNumVerison() << PLAT.get_build_date_time().data()
            << "max thread count:" << QThreadPool::globalInstance()->maxThreadCount();
}

/**
  * @函数名   notify
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/11
  */
bool QTalkApp::notify(QObject *receiver, QEvent *e)
{
    try
    {
        auto t = QDateTime::currentMSecsSinceEpoch();

        switch (e->type())
        {
            case QEvent::MouseButtonPress:
                {
                    auto *mouseEvent = dynamic_cast<QMouseEvent *>(e);

                    if (_pUiManager)
                        emit _pUiManager->sgMousePressGlobalPos(mouseEvent->globalPos());

                    if(_pMainWnd)
                        emit _pMainWnd->sgResetOperator();

                    break;
                }

            // qt : This enum has been deprecated
            // ApplicationStateChange instead
            // -> applicationStateChanged signal
//            case QEvent::ApplicationActivate:
//            {
//                if (nullptr != _pMainWnd) {
//                    _pMainWnd->onAppActive();
//                }
//                break;
//            }
//            case QEvent::ApplicationDeactivate:
//            {
//                if (nullptr != _pMainWnd) {
//                    _pMainWnd->onAppDeactivate();
//                }
//                break;
//            }
            default:
                break;
        }

        auto ret = QApplication::notify(receiver, e);
        t = QDateTime::currentMSecsSinceEpoch() - t;

        if(t > 1000)
        {
            try
            {
                qWarning() << "hang hang hang!!! use time:" << t
                           << " type:" << e->type() ;
            }
            catch (...)
            {
            }
        }

        return ret;
    }
    catch (const std::bad_alloc &)
    {
        qWarning() << "Out of memory";
        QApplication::exit(-1);
//        return false;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

//
bool QTalkApp::event(QEvent *e)
{
#ifdef Q_OS_MAC

    if (e->type() == QEvent::FileOpen)
    {
        auto *event = dynamic_cast<QFileOpenEvent *>(e);

        if (!event->url().isEmpty())
        {
            auto url = event->url();
            QUrlQuery query(url.query());
            auto id = query.queryItemValue("id");
            auto name = query.queryItemValue("name");
            qInfo() << "recv url scheme" << url << id;
            e->ignore();
        }
    }

#endif
    return QApplication::event(e);
}

//
void QTalkApp::onApplicationStateChange(Qt::ApplicationState state)
{
    switch (state)
    {
        case Qt::ApplicationSuspended:
            {
                qInfo() << "app will be Suspend";
                break;
            }

        case Qt::ApplicationHidden:
        case Qt::ApplicationInactive:
            {
                if (nullptr != _pMainWnd)
                    _pMainWnd->onAppDeactivate();

                break;
            }

        case Qt::ApplicationActive:
            {
                if (nullptr != _pMainWnd)
                    _pMainWnd->onAppActive();

                break;
            }

        default:
            break;
    }
}

/**
 *
 */
#ifdef _WINDOWS

#include <time.h>
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    // 返回EXCEPTION_CONTINUE_SEARCH，让程序停止运行
    LONG ret = EXCEPTION_CONTINUE_SEARCH;
    time_t nowtime;
    time(&nowtime);
    struct tm *pTime = localtime(&nowtime);
    // 设置core文件生成目录和文件名
    QString dmpName = QString("%1%2.dmp").arg(strlogPath).arg(QDateTime::currentDateTime().toMSecsSinceEpoch());
    HANDLE hFile = ::CreateFileW((dmpName.toStdWString().data()), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION ExInfo;
        ExInfo.ThreadId = ::GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = NULL;
        // write the dump
        BOOL bOK = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithFullMemory, &ExInfo,
                                     NULL, NULL);
        ret = EXCEPTION_EXECUTE_HANDLER;
        ::CloseHandle(hFile);
    }

    return ret;
}

#endif // _WINDOWS


void QTalkApp::initTTF()
{
    std::string appDataPath = PLAT.getAppdataRoamingPath();
    QString ttfPath = QString("%1/ttf").arg(appDataPath.data());
    {
        QString tmpTTF = QString("%1/%2").arg(ttfPath, "FZLTHJW.TTF");

        if(!QFile::exists(tmpTTF))
        {
            if(!QFile::exists(ttfPath))
            {
                QDir dir(appDataPath.data());
                dir.mkpath(ttfPath);
            }

            QTemporaryFile *tmpFile = QTemporaryFile::createNativeFile(":/QTalk/ttf/FZLTHJW.TTF");
            tmpFile->copy(tmpTTF);
        }

        QFontDatabase::addApplicationFont(tmpTTF);
    }
//    {
//        QString tmpTTF = QString("%1/%2").arg(ttfPath, "FZLTZHJW.TTF");
//        if(!QFile::exists(tmpTTF))
//        {
//            if(!QFile::exists(ttfPath))
//            {
//                QDir dir(appDataPath.data());
//                dir.mkpath(ttfPath);
//            }
//
//            QTemporaryFile* tmpFile = QTemporaryFile::createNativeFile(":/QTalk/ttf/FZLTZHJW.TTF");
//            tmpFile->copy(tmpTTF);
//        }
//        qDebug() << QFontDatabase::addApplicationFont(tmpTTF);
//    }
}
