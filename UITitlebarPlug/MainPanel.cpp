﻿#include "MainPanel.h"

#include <QButtonGroup>
#include <QMouseEvent>
#include <QtConcurrent>
#include "MessageManager.h"
#include "../Platform/Platform.h"
#include "../UICom/uicom.h"
#include "SessionBtn.h"
#include <thread>
#include <QDebug>
#include "../include/Line.h"

#include <QProcess>
#include <QApplication>
#include <QDir>
#include "../CustomUi/QtMessageBox.h"
#include "ChangeHeadWnd.h"

void deleteDir(const QString &path)
{
    if (path.isEmpty())
        return;

    QFileInfo info(path);
    if (!info.exists())
        return;

    if (info.isDir())
    {
        QDir dir(path);
        if (!dir.exists())
            return;

        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        QFileInfoList fileList = dir.entryInfoList();
        for (const auto &fi : fileList)
        {
            deleteDir(fi.absoluteFilePath());
        }
        dir.rmpath(dir.absolutePath());
    }
    else
    {
        info.dir().remove(info.fileName());
    }
}

MainPanel::MainPanel(QWidget *parent) : QFrame(parent)
{

    init();
    //
    connects();
    getSelfCard();
}

MainPanel::~MainPanel()
{
    if (_pMessageListener)
        delete _pMessageListener;
}

/**
  * @功能描述 设置工具栏操控面板
  * @参数
  * @date 2018.9.17
  */
void MainPanel::setCtrlWdt(QWidget *wdt)
{
    _pCtrlWdt = wdt;
    _pCtrlWdt->installEventFilter(this);
}

void MainPanel::getSelfCard()
{
    QtConcurrent::run([]() {
        TitlebarMsgManager::getUserCard(PLAT.getSelfDomain(),
                                        PLAT.getSelfUserId(), 0);
    });
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/09/29
  */
void MainPanel::recvUserCard(const std::vector<QTalk::StUserCard> &userCards)
{
    std::string strSelfId = PLAT.getSelfUserId() + "@" + PLAT.getSelfDomain();

    auto itFind = std::find_if(userCards.begin(), userCards.end(), [strSelfId](const QTalk::StUserCard &user) {
        return user.xmppId == strSelfId;
    });

    if (itFind != userCards.end())
    {
        std::string headPath = TitlebarMsgManager::getHeadPath(itFind->headerSrc);
        if (!headPath.empty() && _userHeadBtn)
        {
            emit setHeadSignal(QString::fromStdString(itFind->nickName), QString::fromStdString(headPath));
        }
    }
}

/**
  * @功能描述
  * @参数
  * @date 2018.9.17
  */
void MainPanel::mousePressEvent(QMouseEvent *e)
{

    _press = true;
    chickPos = QCursor::pos();

    QFrame::mousePressEvent(e);
}

/**
  * @功能描述
  * @参数
  * @date 2018.9.17
  */
void MainPanel::mouseMoveEvent(QMouseEvent *e)
{
    if (_press)
    {
        QPoint curPos = QCursor::pos();
        QPoint movePos = curPos - chickPos;

        if (_pCtrlWdt && _pCtrlWdt->isMaximized() && curPos != chickPos)
        {
            auto geo = _pCtrlWdt->geometry();

#ifdef Q_OS_WIN
            _pCtrlWdt->setWindowState(_pCtrlWdt->windowState() & ~Qt::MaximumSize);
#endif
            _pCtrlWdt->setGeometry(geo);
            //            _pCtrlWdt->showNormal();
            //			_press = false;
            //            return;
        }

        chickPos = curPos;
        if (_pCtrlWdt)
        {
            _pCtrlWdt->move(_pCtrlWdt->x() + movePos.x(), _pCtrlWdt->y() + movePos.y());
        }
    }

    QFrame::mouseMoveEvent(e);
}

void MainPanel::mouseReleaseEvent(QMouseEvent *e)
{
    _press = false;

    QFrame::mouseReleaseEvent(e);
}

/**
  * @功能描述 初始化布局面板各控件
  * @参数
  * @date 2018.9.17
  */
void MainPanel::init()
{
    _pMessageListener = new TitlebarMsgListener(this);

    this->setObjectName("titleMainFrm");
    this->setFixedHeight(_property._mainFrmHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //
    _minimizeBtn = new QToolButton(this);
    _maximizeBtn = new QToolButton(this);
    _closeBtn = new QToolButton(this);
    _restoreBtn = new QToolButton(this);
    _minimizeBtn->setFixedSize(_property._maxminBtnsSize);
    _maximizeBtn->setFixedSize(_property._maxminBtnsSize);
    _restoreBtn->setFixedSize(_property._maxminBtnsSize);
    _closeBtn->setFixedSize(_property._maxminBtnsSize);
    _maxminFrm = new QFrame(this);
    auto *maxminLay = new QHBoxLayout(_maxminFrm);
#ifdef _MACOS
    maxminLay->addWidget(_closeBtn);
    maxminLay->addWidget(_minimizeBtn);
    maxminLay->addWidget(_maximizeBtn);
    maxminLay->addWidget(_restoreBtn);
#else
    maxminLay->addWidget(_minimizeBtn);
    maxminLay->addWidget(_maximizeBtn);
    maxminLay->addWidget(_restoreBtn);
    maxminLay->addWidget(_closeBtn);
#endif
    maxminLay->setMargin(0);
    maxminLay->setSpacing(0);
    //
    _hbox = new QHBoxLayout(this);
    _hbox->setMargin(0);
    _hbox->setSpacing(0);
    _leftCorFrm = new QFrame(this);
    _leftCorFrm->setFixedWidth(20);
    _leftCorFrm->setObjectName("leftCorFrm");
    _hbox->addWidget(_leftCorFrm);
#ifdef _MACOS
    _leftCorFrm->setFixedWidth(12);
    _maxminFrm->setFixedWidth(80);
    _hbox->addWidget(_maxminFrm);
    _maximizeBtn->setObjectName("macmaximizeBtn");
    _closeBtn->setObjectName("gmCloseBtn");
    _minimizeBtn->setObjectName("macminimizeBtn");
    _restoreBtn->setObjectName("macmaximizeBtn");
    _restoreBtn->setVisible(false);
    maxminLay->setContentsMargins(0, 0, 16, 0);
#endif
    _searchFrm = new SeachEditPanel();
    _searchFrm->setFixedSize(_property._searchFrmSize);
    _searchFrm->setObjectName("searchFrm");
#ifdef _MACOS
    _searchFrm->setFixedWidth(150);
#endif
    _hbox->addWidget(_searchFrm);
    _hbox->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

    _pQuickBtn = new QToolButton(this);
    _pQuickBtn->setObjectName(QStringLiteral("creatGroupBtn"));
    _pQuickBtn->setFixedSize(_property._qrCodeBtnSize);
    //    _creatGroupBtn->setToolTip(tr("快速建群"));
    _hbox->addWidget(_pQuickBtn);
    _hbox->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));

    auto *midLay = new QHBoxLayout;

    auto *tabGroup = new QButtonGroup(this);
    _sessionBtn = new SessionBtn(this);
    _sessionBtn->setObjectName(QStringLiteral("sessionBtn"));
    _sessionBtn->setFixedSize(40, 30);
    _sessionBtn->setCheckable(true);
    _contactBtn = new QToolButton(this);
    _contactBtn->setObjectName(QStringLiteral("contactBtn"));
    _contactBtn->setFixedSize(_property._contactBtnSize);
    _contactBtn->setCheckable(true);
    _multifunctionBtn = new QToolButton(this);
    _multifunctionBtn->setObjectName(QStringLiteral("multifunctionBtn"));
    _multifunctionBtn->setFixedSize(_property._multifunctionBtnSize);
    _multifunctionBtn->setCheckable(true);

    tabGroup->addButton(_sessionBtn, 0);
    tabGroup->addButton(_contactBtn, 1);
    tabGroup->addButton(_multifunctionBtn, 2);
    tabGroup->setExclusive(true);

    midLay->addWidget(_sessionBtn);
    midLay->addWidget(_contactBtn);
    midLay->addWidget(_multifunctionBtn);
    midLay->setMargin(0);
    midLay->setSpacing(65);
    _hbox->addLayout(midLay);
    _hbox->addItem(new QSpacerItem(40, 1, QSizePolicy::Expanding));

#ifdef _MACOS
    _hbox->addItem(new QSpacerItem(100, 1, QSizePolicy::Fixed));
#endif

    _userHeadBtn = new HeadPhotoLab(this);
    _userHeadBtn->setObjectName("userBtn");
#ifdef _STARTALK
    QString defaultHead(":/QTalk/image1/StarTalk_defaultHead.png");
#else
    QString defaultHead(":/QTalk/image1/headPortrait.png");
#endif
    _userHeadBtn->setHead(defaultHead, 18, false, false);
    _userHeadBtn->installEventFilter(this);
    _hbox->addWidget(_userHeadBtn);
    _hbox->addItem(new QSpacerItem(15, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

#ifndef _MACOS
    _maxminFrm->setFixedWidth(120);
    _hbox->addWidget(_maxminFrm);
    _minimizeBtn->setObjectName("minimizeBtn");
    _maximizeBtn->setObjectName("maximizeBtn");
    _closeBtn->setObjectName("closeBtn");
    _restoreBtn->setObjectName("restoreBtn");
#endif // !_MACOS
    _pSearchResultPanel = new SearchResultPanel(this);
    //
    _pChangeHeadWnd = new ChangeHeadWnd(this);

    //
    _dropMenu = new DropMenu(this);
    _pAboutWnd = new AboutWnd(this);
    _pSystemSettingWnd = new SystemSettingWnd(this);

    _pQuickMenu = new QMenu(this);
    _pQuickMenu->setAttribute(Qt::WA_TranslucentBackground, true);
    auto *creatGroupAct = new QAction(tr("创建群聊"), this);
    _pQuickMenu->addAction(creatGroupAct);
#ifdef TSCREEN
    auto *throwing = new QAction(tr("发起投屏"), this);
    _pQuickMenu->addAction(throwing);
    connect(throwing, &QAction::triggered, this, &MainPanel::sgShowThrowingScreenWnd);
#endif
    //    this->setFocusProxy(this);
    //
    connect(_closeBtn, SIGNAL(clicked()), this, SLOT(onCtrlWdtClose()));
    connect(_minimizeBtn, SIGNAL(clicked()), this, SLOT(showSmall()));
    connect(_maximizeBtn, SIGNAL(clicked()), this, SLOT(showMaxRestore()));
    connect(_restoreBtn, SIGNAL(clicked()), this, SLOT(showMaxRestore()));
    _restoreBtn->hide();

    connect(tabGroup, SIGNAL(buttonClicked(int)), this, SLOT(onTabGroupClicked(int)));
    connect(_pSearchResultPanel, SIGNAL(sgOpenNewSession(StSessionInfo)), this, SLOT(onOpenNewSession(StSessionInfo)));
    connect(this, &MainPanel::setHeadSignal, this, &MainPanel::setNewHead);
    connect(_pSearchResultPanel, &SearchResultPanel::sgShowMessageRecordWnd, this, &MainPanel::sgShowMessageRecordWnd);
    connect(_pSearchResultPanel, &SearchResultPanel::sgShowFileRecordWnd, this, &MainPanel::sgShowFileRecordWnd);
    connect(_pSearchResultPanel, &SearchResultPanel::sgShowMessageRecordWnd,
            this, &MainPanel::onClearSearch, Qt::QueuedConnection);
    connect(_pSearchResultPanel, &SearchResultPanel::sgShowFileRecordWnd,
            this, &MainPanel::onClearSearch, Qt::QueuedConnection);
    connect(_dropMenu, &DropMenu::sgShowAboutWnd, [this]() {
        if (nullptr != _pAboutWnd)
            _pAboutWnd->showCenter(true, _pCtrlWdt);
    });
    connect(_dropMenu, &DropMenu::sgShowSystemSetting, [this]() {
        if (nullptr != _pSystemSettingWnd)
        {
            //            _pSystemSettingWnd->showCenter(false, _pCtrlWdt);
            _pSystemSettingWnd->setVisible(false);
            _pSystemSettingWnd->setVisible(true);
            QApplication::setActiveWindow(this);
        }
    });
    connect(_dropMenu, &DropMenu::sgDoUpdateClient, this, &MainPanel::sgDoUpdateClient);
    connect(_pSystemSettingWnd, &SystemSettingWnd::sgSetAutoLogin, this, &MainPanel::sgSetAutoLogin);
    connect(_pSystemSettingWnd, &SystemSettingWnd::sgFeedbackLog, this, &MainPanel::feedbackLog);

    connect(_pSystemSettingWnd, &SystemSettingWnd::saveConfig, [this]() {
        emit sgSaveSysConfig();
        TitlebarMsgManager::saveConfig();
    });
    connect(_pSystemSettingWnd, &SystemSettingWnd::sgClearSystemCache, [this]() {
        int ret = QtMessageBox::question(this, tr("友情提示"), tr("是否要清除应用缓存，清除后应用会自动重启？"), QtMessageBox::EM_BUTTON_YES | QtMessageBox::EM_BUTTON_NO);
        if (ret == QtMessageBox::EM_BUTTON_YES)
        {
            // 清除文件夹
            QString userPath = PLAT.getAppdataRoamingUserPath().data();
            // image
            deleteDir(QString("%1/image").arg(userPath));
            // video
            deleteDir(QString("%1/video").arg(userPath));
            // temp
            deleteDir(QString("%1/temp").arg(userPath));
            //
            TitlebarMsgManager::clearSystemCache();

            // 重启应用
            QString program = QApplication::applicationFilePath();
            QProcess::startDetached(program);
            QApplication::exit(0);
        }
    });

    connect(_pChangeHeadWnd, &ChangeHeadWnd::sgChangeHead, [this](const QString &headPath) {
        if (headPath.isEmpty() || headPath == _headPath)
            return;
        TitlebarMsgManager::changeUserHead(headPath.toLocal8Bit().toStdString());
    });

    connect(_pSearchResultPanel, &SearchResultPanel::sgSetEditFocus, [this]() {
        this->activateWindow();
        _searchFrm->_searchEdt->setFocus();
        emit sgOperator(tr("搜索"));
    });

    connect(_pQuickBtn, &QToolButton::clicked, [this]() {
        QPoint pos;
        auto geo = this->geometry();
        pos.setY(mapToGlobal(geo.bottomLeft()).y() - 5);
        pos.setX(mapToGlobal(_pQuickBtn->geometry().topLeft()).x());
        _pQuickMenu->exec(pos);
    });

    connect(creatGroupAct, &QAction::triggered, [this]() {
        QString userCard = QString::fromStdString(PLAT.getSelfXmppId());
        emit creatGroup(userCard);
    });

    _sessionBtn->setChecked(true);
}

void MainPanel::onTabGroupClicked(int tab)
{
    static int old_tab = 0;
    if (old_tab == tab)
    {
        if (tab == 0)
            emit sgJumpToNewMessage();
        return;
    }

    old_tab = tab;
    QString desc;
    switch (tab)
    {
    case 0:
        desc = "tab - 会话";
        break;
    case 1:
        desc = "tab - 联系人";
        break;
    case 2:
        desc = "tab - 应用";
        break;
    default:
        break;
    }

    emit sgOperator(desc);
    emit sgCurFunChanged(tab);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.06
  */
void MainPanel::connects()
{
    connect(_searchFrm, &SeachEditPanel::sgIsOpenSearch, this, &MainPanel::onSearchResultVisible);

    connect(_searchFrm, &SeachEditPanel::sgSelectUp, _pSearchResultPanel, &SearchResultPanel::sgSelectUp);
    connect(_searchFrm, &SeachEditPanel::sgSelectDown, _pSearchResultPanel, &SearchResultPanel::sgSelectDown);
    connect(_searchFrm, &SeachEditPanel::sgSelectItem, _pSearchResultPanel, &SearchResultPanel::sgSelectItem);
    connect(_searchFrm, &SeachEditPanel::sgKeyEsc, [this]() {
        if (_pSearchResultPanel->isVisible())
            _pSearchResultPanel->setVisible(false);
    });

    connect(_pSearchResultPanel, &SearchResultPanel::sgOpenSearch, this, &MainPanel::onSearchResultVisible);
    connect(_searchFrm, &SeachEditPanel::sgStartSearch, _pSearchResultPanel, &SearchResultPanel::addSearchReq);

    //    connect(_sessionBtn, &SessionBtn::clicked, this, &MainPanel::sgJumpToNewMessage);
    connect(_dropMenu, &DropMenu::showSelfUserCard, [this]() {
        QString userCard = QString::fromStdString(PLAT.getSelfXmppId());
        emit showSelfUserCard(userCard);
    });
    connect(_dropMenu, &DropMenu::sysQuit, this, &MainPanel::systemQuitSignal);

    connect(_pSystemSettingWnd, &SystemSettingWnd::sgUpdateHotKey, this, &MainPanel::sgUpdateHotKey);
    connect(_pSystemSettingWnd, &SystemSettingWnd::msgSoundChanged, this, &MainPanel::msgSoundChanged);
    //
    connect(_dropMenu, &DropMenu::sgSwitchUserStatus, this, &MainPanel::onSwitchUserStatus);
    connect(this, &MainPanel::sgSwitchUserStatusRet, _dropMenu, &DropMenu::onSwitchUserStatusRet, Qt::QueuedConnection);
}

/**
 *
 */
void MainPanel::onSwitchUserStatus(const QString &status)
{

    _dropMenu->setVisible(false);
    QtConcurrent::run([status]() {
        TitlebarMsgManager::chanegUserStatus(status.toStdString());
    });
}

void MainPanel::showSmall()
{

    if (_pCtrlWdt)
    {
        //#ifdef _MACOS
        //        emit sgShowMinWnd();
        //#elif
        _pCtrlWdt->showMinimized();
        //#endif
    }
}

void MainPanel::showMaxRestore()
{
    if (_pCtrlWdt)
    {

#ifndef _MACOS
        if (_pCtrlWdt->isMaximized())
        {
            _pCtrlWdt->showNormal();
        }
        else
        {
            _pCtrlWdt->showMaximized();
        }
#else
        if (_pCtrlWdt->isFullScreen())
        {
            _pCtrlWdt->showNormal();
        }
        else
        {
            _pCtrlWdt->showFullScreen();
        }
#endif
    }
}

void MainPanel::onCtrlWdtClose()
{

    if (nullptr == _pCtrlWdt)
        return;

    if (_pCtrlWdt->isFullScreen())
        _pCtrlWdt->setWindowState(_pCtrlWdt->windowState() & ~Qt::WindowFullScreen);
    else
    {
        //if(_pCtrlWdt->isMaximized()){
        //    _pCtrlWdt->setWindowState(_pCtrlWdt->windowState() & ~Qt::WindowMaximized);
        //}

        _pCtrlWdt->setVisible(false);
    }
}

bool MainPanel::eventFilter(QObject *o, QEvent *e)
{
    if (o == _userHeadBtn && e->type() == QEvent::MouseButtonPress)
    {
        if (_dropMenu)
        {
            QPoint pos = _userHeadBtn->geometry().bottomLeft();
            auto x = qMin(pos.x() - _dropMenu->width() / 2, this->geometry().right() - _dropMenu->width() - 15);
            _dropMenu->move(mapToGlobal(QPoint(x, this->geometry().bottom())));
            _dropMenu->show();
        }
    }
    else if (o == _pCtrlWdt && e->type() == QEvent::WindowStateChange)
    {
#ifndef Q_OS_MAC
        _maximizeBtn->setVisible(!_pCtrlWdt->isMaximized());
        _restoreBtn->setVisible(_pCtrlWdt->isMaximized());
#endif // !Q_OS_MAC
    }
    else if (o == _pSearchResultPanel)
    {
    }

    return QFrame::eventFilter(o, e);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.12.06
  */
void MainPanel::onSearchResultVisible(const bool &visible)
{
    if (_pSearchResultPanel)
    {
        if (visible)
        {
            QWidget *wgt = UICom::getInstance()->getAcltiveMainWnd();
            if (nullptr == wgt)
                return;

            QPoint pos1 = QPoint(_leftCorFrm->rect().x(), _leftCorFrm->rect().bottom());
            QPoint pos = _leftCorFrm->mapToGlobal(pos1);
            pos = QPoint(pos.x(), pos.y());
            _pSearchResultPanel->setFixedHeight(wgt->height() - 80);
            _pSearchResultPanel->setVisible(true);
            _pSearchResultPanel->move(pos);
            //            _searchFrm->setEditFocus();
        }
        else
        {
            _pSearchResultPanel->close();
        }
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.06
  */
void MainPanel::onMousePressGolbalPos(const QPoint &gpos)
{
    if (_searchFrm && _pSearchResultPanel)
    {
        QPoint startpos = _searchFrm->mapToGlobal(QPoint(0, 0));
        QRect _searchFrmGR = QRect(startpos, QSize(_searchFrm->width(), _searchFrm->height()));

        startpos = _pSearchResultPanel->mapToGlobal(QPoint(0, 0));
        QRect _searchResultGR = QRect(startpos, QSize(_pSearchResultPanel->width(), _pSearchResultPanel->height()));

        if (_pSearchResultPanel->isVisible() && !_searchResultGR.contains(gpos) && !_searchFrmGR.contains(gpos))
        {
            _searchFrm->closeSearch();
            _pSearchResultPanel->closeSearch();
            _pSearchResultPanel->hide();
            this->setFocus();
        }

#ifdef _WINDOWS
        startpos = _dropMenu->mapToGlobal(QPoint(0, 0));
        QRect dropMenuRect = QRect(startpos, QSize(_dropMenu->width(), _dropMenu->height()));
        if (_dropMenu->isVisible() && !dropMenuRect.contains(gpos))
        {
            _dropMenu->setVisible(false);
        }
#endif // _WINDOWS
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.08
  */
void MainPanel::onOpenNewSession(const StSessionInfo &into)
{
    _searchFrm->closeSearch();
    _pSearchResultPanel->closeSearch();
    _pSearchResultPanel->hide();
    onSwitchFunc(0);
    emit sgOpenNewSession(into);
}

void MainPanel::onAppDeactivated()
{

    if (_pSearchResultPanel && _searchFrm)
    {
        _searchFrm->closeSearch();
        _pSearchResultPanel->closeSearch();
        _pSearchResultPanel->hide();
        _pSearchResultPanel->setVisible(false);
    }
#ifdef _WINDOWS
    if (_dropMenu->isVisible())
    {
        _dropMenu->setVisible(false);
    }
#endif
}

/**
 *
 * @param s
 */
void MainPanel::onSwitchFunc(int s)
{
    switch (s)
    {
    case 0:
    {
        _sessionBtn->click();
        break;
    }
    case 1:
    {
        _contactBtn->click();
        break;
    }
    case 2:
    {
        _multifunctionBtn->click();
        break;
    }
    default:
        break;
    }
}

/**
 *
 * @param count
 */
void MainPanel::updateUnreadCount(int cout)
{
    _sessionBtn->setUnreadCount(cout);
}

void MainPanel::mouseDoubleClickEvent(QMouseEvent *e)
{

    if (_pCtrlWdt)
    {
        if (_pCtrlWdt->isMaximized())
            _pCtrlWdt->showNormal();
        else if (_pCtrlWdt->isFullScreen())
            return;
        else
            _pCtrlWdt->showMaximized();
    }
    QWidget::mouseDoubleClickEvent(e);
}

/**
 *
 * @param newHeadPath
 */
void MainPanel::setNewHead(const QString &userName, const QString &newHeadPath)
{
    _headPath = newHeadPath;
    _userHeadBtn->setHead(newHeadPath, 18, false, true);
    _dropMenu->setName(userName);
    _dropMenu->setHead(newHeadPath);
}

/**
 *
 * @param ret
 * @param locaHead
 */
void MainPanel::onChangeHeadRet(bool ret, const std::string &locaHead)
{
    if (ret)
    {
        std::string userName = PLAT.getSelfName();
        emit setHeadSignal(QString::fromStdString(userName), QString::fromStdString(locaHead));
        emit sgOperator(tr("更换头像"));
        //_userBtn->setHead(QString::fromStdString(locaHead), 18, false, true);
    }
}

/**
 *
 * @param xmppId
 * @param isSelf
 */
void MainPanel::onShowHeadWnd(const QString &headPath, bool isSelf)
{
    if (_pChangeHeadWnd)
    {
        emit sgOperator(tr("查看头像"));
        if (isSelf)
            _pChangeHeadWnd->onChangeHeadWnd(headPath);
        else
            _pChangeHeadWnd->onShowHead(headPath);
    }
}

void MainPanel::recvSwitchUserStatus(const std::string &sts)
{
    emit sgAutoReply(sts == "away");
    emit sgSwitchUserStatusRet(QString::fromStdString(sts));
}

void MainPanel::onClearSearch()
{
    this->setFocus();
    _searchFrm->closeSearch();
    _pSearchResultPanel->closeSearch();
    _pSearchResultPanel->hide();
}

void MainPanel::onShowAboutWnd()
{
    if (nullptr != _pAboutWnd)
    {
        _pAboutWnd->showCenter(true, _pCtrlWdt);
        _pAboutWnd->raise();
    }
}

void MainPanel::onShowSystemWnd()
{
    if (nullptr != _pSystemSettingWnd)
    {
        _pSystemSettingWnd->setVisible(true);
        QApplication::setActiveWindow(_pSystemSettingWnd);
        _pSystemSettingWnd->raise();
    }
}

void MainPanel::onShowUpdateLabel(bool visible)
{
    if (_dropMenu)
    {
        _dropMenu->setTipVisible(visible);
    }

    if (_userHeadBtn)
    {
        _userHeadBtn->setTip(visible);
    }
}