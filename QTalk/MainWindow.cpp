#include "MainWindow.h"
#include "GlobalManager.h"
#include <assert.h>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QStandardItemModel>
#include <QDesktopWidget>
#include <QDateTime>
#include <QtNetwork/QHostInfo>
#include <QPropertyAnimation>
#include <QMenuBar>
#include <QtConcurrent>
#include <QScreen>
#include "../UICom/uicom.h"
#include "../interface/view/IUITitleBarPlug.h"
#include "../interface/view/IUINavigationPlug.h"
#include "../interface/view/IUIChatViewPlug.h"
#include "../interface/view/IUIGroupManagerPlug.h"
#include "../interface/view/IUIPictureBroswerPlug.h"
#include "../interface/view/IUIAddressBookPlug.h"
#include "../UICom/UIEntity.h"
#include "../CustomUi/UShadowEffect.h"
#include "../interface/view/IUICardManagerPlug.h"
#include "../interface/view/IUILoginPlug.h"
#include "../Platform/Platform.h"
#include "MessageManager.h"
#include "SystemTray.h"
#include "../interface/view/IUIOAManagerPlug.h"
#include "../Platform/AppSetting.h"
#include "../CustomUi/QtMessageBox.h"
#include "MacApp.h"
#include "../QtUtil/Utils/utils.h"

#ifdef _WINDOWS
#include <windows.h>
#include <windowsx.h>
#endif

#ifdef _MACOS
#include <sys/stat.h>
#endif

#define WIN_STATE "WIN_STATE"
#define WIN_WIDTH "WIN_WIDTH"
#define WIN_HEIGHT "WIN_HEIGHT"
#define WIN_X "WIN_X"
#define WIN_Y "WIN_Y"

//class StSessionInfo;
extern bool _sys_run;
MainWindow::MainWindow(QWidget *parent) :
#ifdef _MACOS
                                          UShadowDialog(parent, true, false)
#else
                                          UShadowDialog(parent, true, false)
#endif
{
    //
    new QTalkMsgListener(this);
#if defined(_STARTALK)
    setWindowTitle("StarTalk");
#else
    setWindowTitle("QTalk");
#endif
    //
    _timr = new QTimer;
    _timr->setInterval(90 * 1000);
    connect(_timr, &QTimer::timeout, this, &MainWindow::sendHeartBeat);
    connect(this, &MainWindow::LoginSuccess, this, &MainWindow::LoginResult);
    connect(this, &MainWindow::sgRestartWithMessage, this, &MainWindow::restartWithMessage, Qt::QueuedConnection);
    connect(this, &MainWindow::sgCheckUpdate, this, &MainWindow::onCheckUpdater, Qt::QueuedConnection);
    //
    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    for (const QHostAddress &address : info.addresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            auto addr = address.toString();
            if (addr != "127.0.0.1")
                _ipv4Address += QString("%1; ").arg(addr);
        }
    }
    //
#ifdef Q_OS_MAC
    _pWindowMenuBar = new QMenuBar(nullptr);
    //
    QMenu *toolMenu = _pWindowMenuBar->addMenu(tr("工具"));
    auto *addNew = new QAction(tr("程序多开"), toolMenu);
    _pFeedBackLog = new QAction(tr("快速反馈日志"), toolMenu);
    toolMenu->addAction(addNew);
    toolMenu->addAction(_pFeedBackLog);
    //
    connect(addNew, &QAction::triggered, this, &MainWindow::sgRunNewInstance);

    auto *dockMenu = new QMenu(this);
    dockMenu->setAsDockMenu();
    auto *addNewDock = new QAction(tr("程序多开"), dockMenu);
    dockMenu->addAction(addNewDock);
    connect(addNewDock, &QAction::triggered, this, &MainWindow::sgRunNewInstance);
#endif
    //   auto screens = QApplication::screens();
    //   for(auto* sc : screens)
    //       qInfo() << sc->name() << sc->availableGeometry() ;

#if defined(Q_OS_WIN)
    // connect(qApp, &QApplication::screenAdded, [this](QScreen* screen){
    //     qInfo() << screen->name() << this->geometry();
    // });

    connect(qApp, &QApplication::screenRemoved, this, &MainWindow::onScreenRemoved);
#endif
    // process info
    _prcessInfo = new ProcessInfo;
    _prcessInfo->start();
}

MainWindow::~MainWindow()
{
    _prcessInfo->terminate();
    delete _prcessInfo;

    if (_pLocalServer)
        delete _pLocalServer;
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#ifdef _WINDOWS
    static const int boundaryWidth = 5;
    MSG *msg = (MSG *)message;
    double pixelRatio = this->devicePixelRatioF();
    pixelRatio = qMax(pixelRatio, 1.0);
    int tempBorderWidth = boundaryWidth;
    switch (msg->message)
    {
    case WM_NCHITTEST:
        qreal xPos = GET_X_LPARAM(msg->lParam) / pixelRatio - this->frameGeometry().x();
        qreal yPos = GET_Y_LPARAM(msg->lParam) / pixelRatio - this->frameGeometry().y();

        if (xPos < tempBorderWidth && yPos < tempBorderWidth)
            *result = HTTOPLEFT;
        else if (xPos >= width() - tempBorderWidth && yPos < tempBorderWidth)
            *result = HTTOPRIGHT;
        else if (xPos < tempBorderWidth && yPos >= height() - tempBorderWidth)
            *result = HTBOTTOMLEFT;
        else if (xPos >= width() - tempBorderWidth && yPos >= height() - tempBorderWidth)
            *result = HTBOTTOMRIGHT;
        else if (xPos < tempBorderWidth)
            *result = HTLEFT;
        else if (xPos >= width() - tempBorderWidth)
            *result = HTRIGHT;
        else if (yPos < tempBorderWidth)
            *result = HTTOP;
        else if (yPos >= height() - tempBorderWidth)
            *result = HTBOTTOM;
        else
            return false;
        return true;
    }
#endif // _WINDOWS
    return false;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (_sys_run)
    {
        this->setVisible(false);
        e->ignore();
    }
}

void MainWindow::init()
{
    initPanels();
    initQml();
    //
    GlobalManager::instance()->setStyleSheetAll();
}

//
void MainWindow::initQml()
{
    _pView = new QmlView;
    _pView->init();
    //    QUrl source ("qrc:/qml/main.qml");
    //    view->setSource(source);
    //    view->show();
}

/**
  * @函数名
  * @功能描述 获取所需面板插件及初始化面板
  * @参数
  * @date 2018.9.16
  */
void MainWindow::initPanels()
{
    initTitleBar();
    initChatView();
    initAddressBook();
    initGroupManager();
    initCardManager();
    initPictureBrowser();
    initOAManager();
    initNavigation();
}

void MainWindow::initLayouts()
{
    _mainFrm = new QFrame(this);
    _mainFrm->setObjectName("MainWindowMainFrm");
    auto *glay = new QVBoxLayout;
    glay->setContentsMargins(0, 0, 0, 0);
    glay->setSpacing(0);
    _pCenternWgt->setLayout(glay);
    glay->addWidget(_mainFrm);
    //    _mainBottomFrm = new QFrame(this);
    //    _mainBottomFrm->setFixedHeight(6);
    //    _mainBottomFrm->setObjectName("MainWindowMainBottomFrm");
    //    glay->addWidget(_mainBottomFrm);

    if (!_mainLayout)
    {
        _mainLayout = new QVBoxLayout;
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(0);
        _mainFrm->setLayout(_mainLayout);
    }
    if (_titleBar)
    {
        _mainLayout->addWidget(_titleBar);
        if (_titleBarPlugin)
        {
            _titleBarPlugin->setCtrlWdt(this);
        }
    }
    _bottomFrm = new QFrame(this);
    _mainLayout->addWidget(_bottomFrm);
    bodyLay = new QStackedLayout(_bottomFrm);
    bodyLay->setMargin(0);
    bodyLay->setSpacing(0);

    // 聊天窗口区域
    _bottomSplt = new QSplitter;
    _bottomSplt->setHandleWidth(1);
    bodyLay->addWidget(_bottomSplt);
    bodyLay->setCurrentWidget(_bottomSplt);
    if (_navigationPanel)
    {
        _bottomSplt->addWidget(_navigationPanel);
    }
    if (_chatViewPanel)
    {
        _bottomSplt->addWidget(_chatViewPanel);
    }
    _bottomSplt->setStretchFactor(1, 1);
    _bottomSplt->setCollapsible(0, false);
    _bottomSplt->setCollapsible(1, false);

    // 通讯录相关
    if (_pAddressBook)
    {
        _pAddressBook->setVisible(false);
        bodyLay->addWidget(_pAddressBook);
    }
    // OA相关
    if (_pOAManager)
    {
        _pOAManager->setVisible(false);
        bodyLay->addWidget(_pOAManager);
    }
}

/**
  * @功能描述 加载标题栏插件
  * @参数
  * @date 2018.9.16
  */
void MainWindow::initTitleBar()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UITitlebarPlug");
    if (plugin)
    {
        _titleBarPlugin = qobject_cast<IUITitlebarPlug *>(plugin);
        if (_titleBarPlugin)
        {
            _titleBarPlugin->init();
            _titleBar = _titleBarPlugin->widget();
        }
    }
    assert(_titleBarPlugin);
    assert(_titleBar);
}

void MainWindow::initCardManager()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UICardManager");
    if (plugin)
    {
        _pCardManagerPlug = qobject_cast<IUICardManagerPlug *>(plugin);
        if (_pCardManagerPlug)
        {
            _pCardManagerPlug->init();
            _pCardManager = _pCardManagerPlug->widget();
        }
    }
    assert(_pCardManagerPlug);
    assert(_pCardManager);
}

/**
  * @功能描述 加载导航插件
  * @参数
  * @date 2018.9.17
  */
void MainWindow::initNavigation()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UINavigationPlug");
    if (plugin)
    {
        _navigationPlugin = qobject_cast<IUINavigationPlug *>(plugin);
        if (_navigationPlugin)
        {
            _navigationPlugin->init();
            _navigationPanel = _navigationPlugin->widget();
        }
    }
    assert(_navigationPlugin);
    assert(_navigationPanel);
}

/**
  * @功能描述 加载聊天面板插件
  * @参数
  * @date 2018.9.17
  */
void MainWindow::initChatView()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UIChatViewPlug");
    if (plugin)
    {
        _chatViewPlugin = qobject_cast<IUIChatViewPlug *>(plugin);
        if (_chatViewPlugin)
        {
            _chatViewPlugin->init();
            _chatViewPanel = _chatViewPlugin->widget();
        }
    }
    assert(_chatViewPlugin);
    assert(_chatViewPanel);
}

/**
  * @功能描述 链接插件间信号槽
  * @参数
  * @date 2018.9.18
  */
void MainWindow::connectPlugs()
{
    connect(this, SIGNAL(sgSystemQuit()), this, SLOT(systemQuit()));

    qRegisterMetaType<QVector<int>>("QVector<int>");
    //    connect(_chatViewPanel, SIGNAL(recvMessageSignal()), _pSysTrayIcon, SLOT(onRecvMessage()));
    connect(_titleBar, SIGNAL(sgCurFunChanged(int)),
            this, SLOT(onCurFunChanged(int)));
    connect(_pAddressBook, SIGNAL(sgSwitchCurFun(int)),
            this, SLOT(onCurFunChanged(int)));
    connect(_pCardManager, SIGNAL(sgSwitchCurFun(int)),
            this, SLOT(onCurFunChanged(int)));
    connect(_chatViewPanel, SIGNAL(sgSwitchCurFun(int)),
            this, SLOT(onCurFunChanged(int)));

    connect(_titleBar, SIGNAL(sgJumpToNewMessage()), _navigationPanel, SLOT(jumpToNewMessage()));
    connect(this, SIGNAL(appDeactivated()), _titleBar, SLOT(onAppDeactivated()));
    connect(this, SIGNAL(appDeactivated()), _chatViewPanel, SLOT(onAppDeactivated()));
    connect(this, SIGNAL(appDeactivated()), _navigationPanel, SLOT(onAppDeactivated()));
    connect(this, SIGNAL(appDeactivated()), _pSysTrayIcon, SLOT(onAppDeactivated()));

    connect(_navigationPanel, SIGNAL(sgSessionInfo(const StSessionInfo &)),
            _chatViewPanel, SLOT(onChatUserChanged(const StSessionInfo &)), Qt::DirectConnection);
    connect(_titleBar, SIGNAL(showSelfUserCard(const QString &)),
            _pCardManager, SLOT(shwoUserCard(const QString &)));
    connect(_chatViewPanel, SIGNAL(sgShowUserCard(const QString &)),
            _pCardManager, SLOT(shwoUserCard(const QString &)));
    connect(_chatViewPanel, SIGNAL(showUserCardSignal(const QString &)),
            _pCardManager, SLOT(shwoUserCard(const QString &)));
    connect(_navigationPanel, SIGNAL(showUserCardSignal(const QString &)),
            _pCardManager, SLOT(shwoUserCard(const QString &)));
    connect(_navigationPanel, SIGNAL(showGroupCardSignal(const QString &)),
            _pCardManager, SLOT(showGroupCard(const QString &)));
    connect(_chatViewPanel, SIGNAL(showGroupCardSignal(const QString &)),
            _pCardManager, SLOT(showGroupCard(const QString &)));

    qRegisterMetaType<QTalk::Entity::UID>("QTalk::Entity::UID");
    connect(_navigationPanel, SIGNAL(removeSession(const QString &)),
            _chatViewPanel, SLOT(onRemoveSession(const QString &)));
    connect(_titleBar, SIGNAL(sgOpenNewSession(const StSessionInfo &)),
            _navigationPanel, SLOT(onNewSession(const StSessionInfo &)));
    connect(this, SIGNAL(sgJumtoSession(const StSessionInfo &)),
            _navigationPanel, SLOT(onNewSession(const StSessionInfo &)));
    connect(_pCardManager, SIGNAL(sgOpenNewSession(const StSessionInfo &)),
            _navigationPanel, SLOT(onNewSession(const StSessionInfo &)));
    connect(_chatViewPanel, SIGNAL(sgOpenNewSession(const StSessionInfo &)),
            _navigationPanel, SLOT(onNewSession(const StSessionInfo &)));
    connect(_pAddressBook, SIGNAL(sgOpenNewSession(const StSessionInfo &)),
            _navigationPanel, SLOT(onNewSession(const StSessionInfo &)));
    connect(_pAddressBook, SIGNAL(sgSwitchCurFun(int)),
            _titleBar, SLOT(onSwitchFunc(int)));
    connect(_pCardManager, SIGNAL(sgSwitchCurFun(int)),
            _titleBar, SLOT(onSwitchFunc(int)));
    connect(_chatViewPanel, SIGNAL(sgJumpToStructre(const QString &)),
            _pAddressBook, SLOT(jumpToUserStructre(const QString &)));
    connect(_chatViewPanel, SIGNAL(sgSwitchCurFun(int)),
            _titleBar, SLOT(onSwitchFunc(int)));
    connect(_chatViewPanel, SIGNAL(creatGroup(const QString &)),
            _pGroupManager, SLOT(onCreatGroup(const QString &)));
    connect(_titleBar, SIGNAL(creatGroup(const QString &)),
            _pGroupManager, SLOT(onCreatGroup(const QString &)));
    connect(_chatViewPanel, SIGNAL(addGroupMember(const QString &)),
            _pGroupManager, SLOT(onAddGroupMember(const QString &)));
    connect(_chatViewPanel, SIGNAL(showChatPicture(const QString &, const QString &, int)),
            _pPictureBrowser, SLOT(onShowChatPicture(const QString &, const QString &, int)));
    connect(_chatViewPanel, SIGNAL(sgShowPicture(const QString &, const QString &)),
            _pPictureBrowser, SLOT(showPicture(const QString &, const QString &)));
    //    connect(_pGroupManager, SIGNAL(setTreeDataFinised()), _pAddressBook, SLOT(updateStaffUi()));
    connect(_navigationPanel, SIGNAL(updateTotalUnreadCount(int)), _titleBar, SLOT(updateUnreadCount(int)));
    connect(_titleBar, SIGNAL(feedbackLog(const QString &)), _chatViewPanel, SLOT(packAndSendLog(const QString &)));
    connect(this, SIGNAL(systemShortCut()), _chatViewPanel, SLOT(systemShortCut()));
    connect(_titleBar, SIGNAL(systemQuitSignal()), this, SLOT(systemQuit()), Qt::QueuedConnection);
    connect(this, SIGNAL(systemQuitSignal()), this, SLOT(systemQuit()), Qt::QueuedConnection);

    connect(_chatViewPanel, SIGNAL(sgShortCutSwitchSession(int)),
            _navigationPanel, SLOT(onShortCutSwitchSession(int)));

    connect(_navigationPanel, SIGNAL(sgShowUnreadMessage(int, const QTalk::Entity::UID &, const QString &, qint64, int)),
            _pSysTrayIcon, SIGNAL(sgShowUnreadMessage(int, const QTalk::Entity::UID &, const QString &, qint64, int)));
    //
    connect(_pCardManager, SIGNAL(sgJumpToStructre(const QString &)), _pAddressBook, SLOT(onJumpToStructre(const QString &)));
    connect(_pCardManager, SIGNAL(sgShowHeadWnd(const QString &, bool)),
            _titleBar, SLOT(onShowHeadWnd(const QString &, bool)));
    //connect(_chatViewPanel, SIGNAL(sgForwardMessage(const QString&)), _pGroupManager, SLOT(onForfardMessage(const QString&)));
    connect(_titleBar, SIGNAL(sgSetAutoLogin(bool)), this, SLOT(setAutoLogin(bool)));
    connect(_titleBar, SIGNAL(sgUpdateHotKey()), this, SLOT(onUpdateHotKey()));

    connect(_titleBar, SIGNAL(sgSaveSysConfig()), GlobalManager::instance(), SLOT(saveSysConfig()));
    connect(_titleBar, SIGNAL(sgSaveSysConfig()), this, SLOT(onSaveSysConfig()));
    connect(_titleBar, SIGNAL(msgSoundChanged()), _chatViewPanel, SLOT(onMsgSoundChanged()));

    //
    connect(_titleBar, SIGNAL(sgShowMessageRecordWnd(const QString &, const QString &)),
            _chatViewPanel, SLOT(onShowSearchResult(const QString &, const QString &)));
    connect(_titleBar, SIGNAL(sgShowFileRecordWnd(const QString &)),
            _chatViewPanel, SLOT(onShowSearchFileWnd(const QString &)));

    connect(_chatViewPanel, SIGNAL(sgWakeUpWindow()), this, SLOT(wakeUpWindow()));
    connect(this, SIGNAL(sgAppActive()), _chatViewPanel, SLOT(onAppActive()));
    connect(this, SIGNAL(sgAppActive()), _navigationPanel, SLOT(onAppActive()));
    connect(_titleBar, SIGNAL(sgActiveWnd()), this, SLOT(wakeUpWindow()));
    //
    connect(_chatViewPanel, SIGNAL(sgUserSendMessage()),
            this, SLOT(onUserSendMessage()));
    connect(GlobalManager::instance(), SIGNAL(sgMousePressGlobalPos(QPoint)), _titleBar,
            SLOT(onMousePressGolbalPos(QPoint)));
    // show draft
    connect(_chatViewPanel, SIGNAL(sgShowDraft(const QTalk::Entity::UID &, const QString &)),
            _navigationPanel, SLOT(onShowDraft(const QTalk::Entity::UID &, const QString &)));
    // sgShowNotify
    qRegisterMetaType<QTalk::StNotificationParam>("QTalk::StNotificationParam");
    connect(_chatViewPanel, SIGNAL(sgShowNotify(const QTalk::StNotificationParam &)),
            _pSysTrayIcon, SLOT(onShowNotify(const QTalk::StNotificationParam &)));
    //
    connect(this, SIGNAL(sgShowUpdateClientLabel(bool)), _titleBar, SLOT(onShowUpdateLabel(bool)));
    connect(_pView, SIGNAL(sgShowUpdateClientLabel(bool)), _titleBar, SLOT(onShowUpdateLabel(bool)));
    connect(_titleBar, SIGNAL(sgDoUpdateClient()), this, SLOT(onShowCheckUpdateWnd()));
    // 自动回复
    connect(_titleBar, SIGNAL(sgAutoReply(bool)), _chatViewPanel, SLOT(setAutoReplyFlag(bool)));

    // 操作日志
    connect(_titleBar, SIGNAL(sgOperator(const QString &)), this, SLOT(addOperatorLog(const QString &)));
    connect(_chatViewPanel, SIGNAL(sgOperator(const QString &)), this, SLOT(addOperatorLog(const QString &)));
    connect(_navigationPanel, SIGNAL(sgOperator(const QString &)), this, SLOT(addOperatorLog(const QString &)));
    connect(_pCardManager, SIGNAL(sgOperator(const QString &)), this, SLOT(addOperatorLog(const QString &)));
    connect(_pAddressBook, SIGNAL(sgOpeartor(const QString &)), this, SLOT(addOperatorLog(const QString &)));
    connect(_pOfflineTimer, &QTimer::timeout, [this]() {
        //        if(_pMessageManager)
        {
            _isOffline = true;
            _pOfflineTimer->stop();
            _logout_t = QDateTime::currentMSecsSinceEpoch();
            QTalkMsgManager::sendOnlineState(_login_t, _logout_t, _ipv4Address.toStdString());
        }
    });
    connect(_pLogTimer, &QTimer::timeout, this, &MainWindow::onHourTimer);
    //
#ifdef TSCREEN
    connect(_pOAManager, SIGNAL(sgShowThrowingScreenWnd()), this, SLOT(startTScreen()));
    connect(_titleBar, SIGNAL(sgShowThrowingScreenWnd()), this, SLOT(startTScreen()));
#endif
}

//
void MainWindow::onHourTimer()
{
#if !defined(_STARTALK) && !defined(_QCHAT)
    if (_loginDate != QDate::currentDate())
    {
        //
        _loginDate = QDate::currentDate();
        QtConcurrent::run(&QTalkMsgManager::reportLogin);
    }

#endif
    // check update
    if (AppSetting::instance().getNewVersion() <= PLAT.getClientNumVerison())
    {
        checkUpdater();
    }

    // report
    if (_operators.empty())
        return;
    std::vector<QTalk::StActLog> operators(_operators);
    QtConcurrent::run(&QTalkMsgManager::sendOperatorStatistics, _ipv4Address.toStdString(), operators);
    _operators.clear();
}

#include <QLocalSocket>
void MainWindow::startTScreen()
{
#ifdef _LINUX
    return;
#endif
    //
    static const char *QUNAR_THROW_SCREEN = "QUNAR_THROW_SCREEN";
    QLocalSocket localSocket;
    localSocket.connectToServer(QUNAR_THROW_SCREEN);
    if (localSocket.waitForConnected(2000))
    {
        localSocket.disconnectFromServer();
        localSocket.close();
        return;
    }
    //
    QStringList arguments;
#if defined(_WINDOWS)
    const QString &cmd = QString("%1/%2").arg(QApplication::applicationDirPath(), "TScreen/TScreen.exe");
    arguments << QString("LOGIN_USER_NAME=").append(PLAT.getSelfUserId().data());
    QProcess::startDetached(cmd, arguments);
#elif defined(_MACOS)
    const QString &cmd = QString("%1/%2").arg(QApplication::applicationDirPath(), "TScreen");
    chmod(cmd.toStdString().data(), S_IRWXU);
    arguments << QString("LOGIN_USER_NAME=").append(PLAT.getSelfName().data());
    QProcess::startDetached(cmd, arguments);
#endif
}

/**
 *
 * @param autoFlag
 */
void MainWindow::setAutoLogin(bool autoFlag)
{
    _pLoginPlug->setAutoLoginFlag(autoFlag);
}

/**
  * @函数名   initSystemTray
  * @功能描述 初始化系统托盘
  * @参数
  * @author   cc
  * @date     2018/11/09
  */
void MainWindow::initSystemTray()
{
    _pSysTrayIcon = new SystemTray(this);
#ifdef _MACOS
    connect(_pFeedBackLog, &QAction::triggered, _pSysTrayIcon, &SystemTray::onSendLog);
#endif
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/11/11
  */
void MainWindow::initGroupManager()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UIGroupManager");
    if (plugin)
    {
        _pGroupManagerPlug = qobject_cast<IUIGroupManagerPlug *>(plugin);
        if (_pGroupManagerPlug)
        {
            _pGroupManagerPlug->init();
            _pGroupManager = _pGroupManagerPlug->widget();
        }
    }
    assert(_pGroupManagerPlug);
    assert(_pGroupManager);
}

void MainWindow::InitLogin(bool _enable, const QString &loginMsg)
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UILoginPlug");
    if (plugin)
    {
        _pLoginPlug = qobject_cast<IUILoginPlug *>(plugin);
        if (_pLoginPlug)
        {
            _pLoginPlug->init();
            _pLoginPlug->enableAutoLogin(_enable);
            _pLoginPlug->initConf();
            if (!loginMsg.isEmpty())
                _pLoginPlug->setLoginMessage(loginMsg);
            _logindlg = (QDialog *)_pLoginPlug->widget();
        }
        if (_logindlg)
        {
            GlobalManager::instance()->setStyleSheetForPlugin("UILoginPlug");
            connect(_logindlg, SIGNAL(sgSynDataSuccess()), this, SLOT(openMainWindow()));
            connect(_logindlg, SIGNAL(systemQuitSignal()), this, SLOT(systemQuit()), Qt::QueuedConnection);
            _logindlg->show();
        }
    }
}

// 登陆成功 打开数据库同步服务器数据
void MainWindow::LoginResult(bool result)
{
    if (result)
    {
        _timr->start();
    }
}

void MainWindow::OnLoginSuccess(const std::string &strSessionId)
{
    emit LoginSuccess(true);
}

void MainWindow::openMainWindow()
{
    if (_initUi)
    {
        _isOffline = false;
        _login_t = QDateTime::currentMSecsSinceEpoch();
        _pOfflineTimer = new QTimer(this);
        _pLogTimer = new QTimer(this);
        _pOfflineTimer->setInterval(1000 * 60 * 60);
        _pLogTimer->setInterval(1000 * 60 * 60);
        _pLogTimer->start();
        //
        _pLocalServer = new LocalServer;
        auto key = QString("%1==%2").arg(QApplication::applicationName().toLower(), PLAT.getSelfUserId().data());
        _pLocalServer->runServer(key);
        //
        _initUi = false;
        init();
        initLayouts();
        connectPlugs();
        _chatViewPanel->setFocus();
        //
        _navigationPlugin->updateReadCount();
        // setwindow states
        QString configPath = QString("%1/mainWnd").arg(PLAT.getConfigPath().data());
        //
        _pConfigLoader = new QTalk::ConfigLoader(configPath.toLocal8Bit());
        if (_pConfigLoader->reload())
        {
            int wndstate = _pConfigLoader->getInteger(WIN_STATE);
            int wndWidth = _pConfigLoader->getInteger(WIN_WIDTH);
            int wndHeight = _pConfigLoader->getInteger(WIN_HEIGHT);
            int wndX = _pConfigLoader->getInteger(WIN_X);
            int wndY = _pConfigLoader->getInteger(WIN_Y);
            if (wndWidth <= 0 || wndHeight <= 0)
            {
                QDesktopWidget *deskTop = QApplication::desktop();
                int curMonitor = deskTop->screenNumber(_logindlg);
                QRect deskRect = deskTop->screenGeometry(curMonitor);
                wndWidth = deskRect.height();
                wndHeight = (int)(deskRect.height() * 0.75);
                wndX = (deskRect.width() - wndWidth) / 2 + deskRect.x();
                wndY = (deskRect.height() - wndHeight) / 2 + deskRect.y();
                //
                _pConfigLoader->setInteger(WIN_WIDTH, wndWidth);
                _pConfigLoader->setInteger(WIN_HEIGHT, wndHeight);
                _pConfigLoader->setInteger(WIN_X, wndX);
                _pConfigLoader->setInteger(WIN_Y, wndY);
                _pConfigLoader->saveConfig();
            }

            setGeometry(wndX, wndY, wndWidth, wndHeight);
            switch (wndstate)
            {
            case WND_MAXSIZE:
                //showMaximized();
                //break;
            case WND_NORMAL:
            default:
            {
                showNormal();
                // 判断是否超出屏幕范围
                adjustWndRect();
                break;
            }
            }
        }
        UICom::getInstance()->setAcltiveMainWnd(this);
        // 注册快捷键
        _pScreentShot = new QHotkey(this);
        _pWakeWnd = new QHotkey(this);
        if (AppSetting::instance().getHotCutEnable())
        {
            onUpdateHotKey();
        }
        connect(_pScreentShot, &QHotkey::activated, this, &MainWindow::onScreentShot);
        connect(_pWakeWnd, &QHotkey::activated, this, &MainWindow::wakeUpWindow);
        connect(_pLocalServer, &LocalServer::sgWakeupWindow, this, &MainWindow::wakeUpWindow);
        //
        if (_logindlg)
        {
            _logindlg->setVisible(false);
            _pLoginPlug->saveHeadPath();
        }

        std::function<int(STLazyQueue<bool> *)> func = [this](STLazyQueue<bool> *queue) -> int {
            int count = 0;
            while (!queue->empty())
            {
                queue->pop();
                count++;
            }
            emit systemShortCut();
            return count;
        };
        _pScreentShotQueue = new STLazyQueue<bool>(300, func);

        //
        _noOperatorThread = new NoOperationThread;
        connect(this, &MainWindow::sgResetOperator,
                _noOperatorThread, &NoOperationThread::resetUnOperatorTime,
                Qt::QueuedConnection);
        connect(_noOperatorThread, SIGNAL(sgUserLeave(bool)), _chatViewPanel, SLOT(setAutoReplyFlag(bool)));
        connect(_noOperatorThread, SIGNAL(sgUserLeave(bool)), this, SLOT(setUserStatus(bool)));
        connect(_titleBar, SIGNAL(sgAutoReply(bool)), _noOperatorThread, SLOT(setAutoReplyFlag(bool)));
        //
#ifdef _MACOS
        QMenu *wndMenu = _pWindowMenuBar->addMenu(tr("窗口"));
        auto *minSize = new QAction(tr("最小化"), wndMenu);
        auto *maxSize = new QAction(tr("缩放"), wndMenu);
        auto *showWnd = new QAction(tr("显示面板"), wndMenu);
        wndMenu->addAction(minSize);
        wndMenu->addAction(maxSize);
        wndMenu->addSeparator();
        wndMenu->addAction(showWnd);

        QMenu *systemMenu = _pWindowMenuBar->addMenu(tr("系统"));
        auto *setting = new QAction(tr("系统设置"), wndMenu);
        auto *about = new QAction(tr("关于"), wndMenu);
        systemMenu->addAction(setting);
        systemMenu->addAction(about);

        connect(minSize, &QAction::triggered, [this]() {
            MacApp::showMinWnd(this);
        });
        connect(maxSize, &QAction::triggered, [this]() {
            if (this->isMaximized())
                this->showNormal();
            else
                this->showMaximized();
        });
        connect(showWnd, &QAction::triggered, [this]() {
            this->wakeUpWindow();
        });

        connect(setting, SIGNAL(triggered()), _titleBar, SLOT(onShowSystemWnd()));
        connect(about, SIGNAL(triggered()), _titleBar, SLOT(onShowAboutWnd()));
#endif
        //
#if !defined(_STARTALK) && !defined(_QCHAT)
        _loginDate = QDate::currentDate();
        QtConcurrent::run(&QTalkMsgManager::reportLogin);
#endif
        //
        dealDumpFile();
        checkUpdater();
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/12/14
  */
void MainWindow::onCurFunChanged(int index)
{
    //
    switch (index)
    {
    case 0:
        bodyLay->setCurrentWidget(_bottomSplt);
        break;
    case 1:
        bodyLay->setCurrentWidget(_pAddressBook);
        break;
    case 2:
        bodyLay->setCurrentWidget(_pOAManager);
        break;
    default:
        break;
    }
}

//
void MainWindow::onAppActive()
{
    emit sgResetOperator();
#ifdef Q_OS_MAC
    MacApp::resetWindow(this);
#endif

    if (_initUi)
    {
    }
    else
    {
        if (_isOffline)
        {
            _isOffline = false;
            _login_t = QDateTime::currentMSecsSinceEpoch();
        }
        else
        {
            if (_pOfflineTimer)
                _pOfflineTimer->stop();
        }

        _pSysTrayIcon->onWndActived();
#ifdef _MACOS
        if (!this->isVisible())
            MacApp::wakeUpWnd(this);
#endif
        emit sgAppActive();
        setVisible(true);
        raise();
    }
}

void MainWindow::initPictureBrowser()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UIPictureBrowser");
    if (plugin)
    {
        _pPictureBrowserPlug = qobject_cast<IUIPictureBroswerPlug *>(plugin);
        if (_pPictureBrowserPlug)
        {
            _pPictureBrowserPlug->init();
            _pPictureBrowser = _pPictureBrowserPlug->widget();
        }
    }
    assert(_pPictureBrowserPlug);
    assert(_pPictureBrowser);
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/12/15
  */
void MainWindow::initAddressBook()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UIAddressBook");
    if (plugin)
    {
        _pAddressBookPlug = qobject_cast<IUIAddressBookPlug *>(plugin);
        if (_pAddressBookPlug)
        {
            _pAddressBookPlug->init();
            _pAddressBook = _pAddressBookPlug->widget();
        }
    }
    assert(_pAddressBookPlug);
    assert(_pAddressBook);
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/12/15
  */
void MainWindow::initOAManager()
{
    QObject *plugin = GlobalManager::instance()->getPluginInstanceQt("UIOAManager");
    if (plugin)
    {
        _pOAManagerPlug = qobject_cast<IUIOAManagerPlug *>(plugin);
        if (_pOAManagerPlug)
        {
            _pOAManagerPlug->init();
            _pOAManager = _pOAManagerPlug->widget();
        }
    }
    assert(_pOAManagerPlug);
    assert(_pOAManager);
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/12/15
  */
void MainWindow::onAppDeactivate()
{
    if (!_isOffline && _pOfflineTimer)
    {
        _pOfflineTimer->start();
    }

    emit appDeactivated();
}

void MainWindow::saveWndState()
{
    int wndstate = WND_NORMAL;
    if (windowState() == Qt::WindowMaximized)
    {
        wndstate = WND_MAXSIZE;
    }
    QRect geo = this->normalGeometry();

    if (_pConfigLoader)
    {
        _pConfigLoader->setInteger(WIN_STATE, wndstate);
        _pConfigLoader->setInteger(WIN_WIDTH, geo.width());
        _pConfigLoader->setInteger(WIN_HEIGHT, geo.height());
        _pConfigLoader->setInteger(WIN_X, geo.x());
        _pConfigLoader->setInteger(WIN_Y, geo.y());
        _pConfigLoader->saveConfig();
    }
}

/**
 * 系统退出
 */
void MainWindow::systemQuit()
{
    _sys_run = false;
    //
    QLocalServer::removeServer(PLAT.getSelfXmppId().data());

    _logout_t = QDateTime::currentMSecsSinceEpoch();

    saveWndState();
    QApplication::exit(0);
}

/**
 *
 */
void MainWindow::sendHeartBeat()
{
    QtConcurrent::run(&QTalkMsgManager::sendHearBeat);
}

// 多屏处理
void MainWindow::adjustWndRect()
{

    QRect thisGeo = this->geometry();
    QDesktopWidget *deskTop = QApplication::desktop();
    int curMonitor = deskTop->screenNumber(this);
    QRect deskRect = deskTop->screenGeometry(curMonitor);
    if (!deskRect.contains(thisGeo, true))
    {

        auto wndWidth = deskRect.height();
        auto wndHeight = (int)(deskRect.height() * 0.75);
        auto wndX = (deskRect.width() - wndWidth) / 2 + deskRect.x();
        auto wndY = (deskRect.height() - wndHeight) / 2 + deskRect.y();
        setGeometry(wndX, wndY, wndWidth, wndHeight);
    }
}

/**
 *
 */
void MainWindow::onUpdateHotKey()
{
    std::string screentHot = AppSetting::instance().getScreenshotHotKey();
    std::string wakeWnd = AppSetting::instance().getWakeWndHotKey();
    bool isok = _pScreentShot->setShortcut(QKeySequence::fromString(screentHot.data(), QKeySequence::NativeText), true);
    isok = _pWakeWnd->setShortcut(QKeySequence::fromString(wakeWnd.data(), QKeySequence::NativeText), true);
}

void MainWindow::onScreentShot()
{
    _pScreentShotQueue->push(true);
}

void MainWindow::hideEvent(QHideEvent *e)
{
    if (Qt::ApplicationActive == QApplication::applicationState())
        onAppDeactivate();
    QWidget::hideEvent(e);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape || e->key() == Qt::Key_Space)
    {
        e->accept();
    }

#ifdef _MACOS
    if (e->modifiers() == Qt::ControlModifier)
    {
        if (e->key() == Qt::Key_W)
            this->setVisible(false);
        else if (e->key() == Qt::Key_M)
            this->showMinimized();
        else if (e->key() == Qt::Key_Q)
        {
            //            int ret =
            //            if(ret == QtMessageBox::EM_BUTTON_YES)
            //            {
            //                exit(0);
            //            }
            //            e->accept();
            //            return;
        }
    }
#endif
    UShadowDialog::keyPressEvent(e);
}

QWidget *MainWindow::getActiveWnd()
{
    QWidget *wakeUpWgt = nullptr;
    if (_initUi)
        wakeUpWgt = _logindlg;
    else
        wakeUpWgt = this;

    return wakeUpWgt;
}

void MainWindow::wakeUpWindow()
{
    QWidget *wakeUpWgt = getActiveWnd();

    if (!wakeUpWgt)
    {
        qWarning() << "no active windows";
        return;
    }

    if (wakeUpWgt->isMinimized())
        wakeUpWgt->setWindowState((wakeUpWgt->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    wakeUpWgt->setVisible(true);
    QApplication::setActiveWindow(wakeUpWgt);
    wakeUpWgt->raise();

#ifdef Q_OS_WIN
    HWND hForgroundWnd = GetForegroundWindow();
    DWORD dwForeID = ::GetWindowThreadProcessId(hForgroundWnd, NULL);
    DWORD dwCurID = ::GetCurrentThreadId();

    ::AttachThreadInput(dwCurID, dwForeID, TRUE);
    ::SetForegroundWindow((HWND)winId());
    ::AttachThreadInput(dwCurID, dwForeID, FALSE);
#endif // Q_OS_WIN
}

/**
 *
 * @param desc
 */
void MainWindow::addOperatorLog(const QString &desc)
{
    QTalk::StActLog log;
    log.desc = desc.toStdString();
    log.operatorTime = QDateTime::currentMSecsSinceEpoch();
    _operators.push_back(log);
}

///**
// *
// */
//void MainWindow::onShockWnd()
//{
//    if(!this->isMaximized() && !this->isFullScreen())
//    {
//        QRect now = this->geometry();
//        auto* animation = new QPropertyAnimation(this, "pos");
//        animation->setDuration(500);
//        animation->setStartValue(QPoint(now.x(), now.y() + 20));
//        animation->setEndValue(QPoint(now.x(), now.y() - 20));
//        animation->setEasingCurve(QEasingCurve::InOutBounce);
//        connect(animation, &QPropertyAnimation::finished, [this, now](){
//            setGeometry(now);
//        });
//        animation->start(QAbstractAnimation::DeleteWhenStopped);
//    }
//}

/**
 *
 */
void MainWindow::onSaveSysConfig()
{
    if (_noOperatorThread)
        _noOperatorThread->setLeaveMinute();

    //
    GlobalManager::instance()->saveSysConfig();
}

void MainWindow::setUserStatus(bool isAway)
{
    QTalkMsgManager::chanegUserStatus(isAway ? "away" : "online");
}

void MainWindow::onUserSendMessage()
{
    _noOperatorThread->resetUnOperatorTime();
    QTalkMsgManager::chanegUserStatus("online");
}

void MainWindow::checkUpdater()
{
    //#ifndef QT_DEBUG
    auto version = GlobalManager::instance()->_updater_version;
    QtConcurrent::run(&QTalkMsgManager::checkUpdater, version);
    //#endif
}

//
#include <QDesktopServices>
void MainWindow::onCheckUpdater(bool hasUpdate, const QString &link, bool force)
{
    if (hasUpdate)
    {
#ifdef Q_OS_LINUX

        if (!PLAT.isMainThread())
        {
            emit sgCheckUpdate(hasUpdate, link, force);
            return;
        }

        if (hasUpdate)
        {
            QtMessageBox::information(this, tr("升级提醒"),
                                      tr("主程序有更新，麻烦您及时到官网下载最新版本"));
            return;
        }
#else
        emit sgShowUpdateClientLabel(true);

        if (force)
        {
            _pView->showWnd();
        }
#endif
    }
}

#ifdef Q_OS_MAC
void MainWindow::onShowMinWnd()
{
    MacApp::showMinWnd(this);
}

#endif

void MainWindow::changeEvent(QEvent *event)
{

    switch (event->type())
    {
    case QEvent::WindowStateChange:
    {
        auto sts = this->windowState();
        if (sts == Qt::WindowNoState)
        {
        }
        break;
    }
    default:
        break;
    }

    UShadowDialog::changeEvent(event);
}

void MainWindow::onGetHistoryError()
{
    emit sgRestartWithMessage(tr("获取历史记录失败, 请重新登录!"));
}

//
void MainWindow::reLoginWithErr(const std::string &err)
{
    emit sgRestartWithMessage(err.data());
}

void MainWindow::systemQuitByEvt()
{
    emit sgSystemQuit();
}

//
void MainWindow::restartWithMessage(const QString &msg)
{

    QString program = QApplication::applicationFilePath();
    QStringList arguments;
    arguments << "AUTO_LOGIN=OFF"
              << QString("MSG=%1").arg(msg);
    QProcess::startDetached(program, arguments);
    QApplication::exit(0);
}

void delDmpFun(const QString &path, const std::string &ip)
{
    QDir dir(path);
    QFileInfoList infoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &tmpInfo : infoList)
    {
        if (tmpInfo.isSymLink())
            continue;

        if (tmpInfo.isFile())
        {
            auto now = QDateTime::currentMSecsSinceEpoch();
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            auto birth = tmpInfo.birthTime().toMSecsSinceEpoch();
#else
            auto birth = tmpInfo.lastModified().toMSecsSinceEpoch();
#endif

            if (tmpInfo.suffix().toLower() == "dmp")
            {
                if (tmpInfo.size() > 0 && tmpInfo.size() <= 50 * 1024 * 1024 && now - birth < 1000 * 60 * 60 * 24 * 7)
                {
                    std::string dumpFilePath = std::string(tmpInfo.absoluteFilePath().toLocal8Bit());
                    QTalkMsgManager::reportDump(ip, dir.dirName().toStdString(), dumpFilePath, birth);
                }
                QFile::remove(tmpInfo.absoluteFilePath());
            }
            else if (now - birth > 1000 * 60 * 60 * 24)
                QFile::remove(tmpInfo.absoluteFilePath());
        }
        else if (tmpInfo.isDir())
        {
            auto infoPath = tmpInfo.absoluteFilePath();
            delDmpFun(infoPath, ip);
            if (QDir(infoPath).isEmpty())
                dir.rmpath(infoPath);
        }
    }
}

////
//qint64 calculateFolderSize(const QString& folderPath)
//{
//    qint64 size = 0;
//
//    QDir dir(folderPath);
//    QFileInfoList infoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
//    for (const auto& tmpInfo : infoList)
//    {
//        if(tmpInfo.isSymLink()) continue;
//
//        if (tmpInfo.isFile())
//        {
//            size += tmpInfo.size();
//        }
//        else if(tmpInfo.isDir())
//        {
//            auto infoPath = tmpInfo.absoluteFilePath();
//            size += calculateFolderSize(infoPath);
//        }
//    }
//
//    return size;
//}

void clearCache(const QString &folderPath)
{
    QDir dir(folderPath);
    QFileInfoList infoList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &tmpInfo : infoList)
    {
        if (tmpInfo.isSymLink())
            continue;
        auto now = QDateTime::currentMSecsSinceEpoch();
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
        auto birth = tmpInfo.birthTime().toMSecsSinceEpoch();
#else
        auto birth = tmpInfo.lastModified().toMSecsSinceEpoch();
#endif
        if (now - birth < 1000 * 60 * 60 * 24 * 2)
            continue;

        if (tmpInfo.isFile())
        {
            QFile::remove(tmpInfo.absoluteFilePath());
        }
        else if (tmpInfo.isDir())
        {
            auto infoPath = tmpInfo.absoluteFilePath();
            clearCache(infoPath);
        }
    }
}

//
void MainWindow::dealDumpFile()
{
    QTimer::singleShot(30 * 1000, [this]() {
        QtConcurrent::run([this]() {
            // deal dump
            QDateTime curDateTime = QDateTime::currentDateTime();
            auto appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toLocal8Bit();
            QString logDirPath = QString("%1/logs/").arg(appdata.data());
            delDmpFun(logDirPath, _ipv4Address.toStdString());
            // clear cache
            auto userDir = PLAT.getAppdataRoamingUserPath();
            clearCache((userDir + "/video").data());
            clearCache((userDir + "/voice").data());
            clearCache((userDir + "/temp").data());
            clearCache((userDir + "/image/temp").data());
            clearCache((userDir + "/image/source").data());
            clearCache((appdata + "/HotPic").data());
            // delete old app
            QString oldApp = QString("%1/apps").arg(appdata.data());
            if (QFile::exists(oldApp))
            {
                QDir dir;
                dir.setPath(oldApp);
                qInfo() << "delete old appps" << dir.removeRecursively();
            }
        });
    });
}

//
void MainWindow::onScreenRemoved(QScreen *screen)
{
    Q_UNUSED(screen)
    {
        QTimer::singleShot(500, [this]() {
            auto screenGeometry = QApplication::primaryScreen()->availableGeometry();
            //if(!screenGeometry.contains(this->geometry()))
            {
                auto geometry = this->geometry();
                geometry.setWidth(qMin(screenGeometry.width() - 100, geometry.width()));
                geometry.setHeight(qMin(screenGeometry.height() - 100, geometry.height()));
                geometry.setX((screenGeometry.width() - geometry.width()) / 2);
                geometry.setY((screenGeometry.height() - geometry.height()) / 2);

                qInfo() << geometry;
            }
        });
    }
}

#include <QMetaObject>
bool MainWindow::event(QEvent *e)
{
    return QWidget::event(e);
}

void MainWindow::onShowCheckUpdateWnd()
{
    if (_pView)
    {
        _pView->showWnd();
    }
}
