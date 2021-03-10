#include "LoginPanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLocalSocket>
#include <QApplication>
#include <QProcess>
#include <QNetworkConfigurationManager>
#include <QTcpSocket>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QEventLoop>
#include "../CustomUi/UShadowEffect.h"
#include "../UICom/uicom.h"
#include "../QtUtil/lib/ini/ConfigLoader.h"
#include "../Platform/Platform.h"
#include "../Platform/dbPlatForm.h"
#include "../QtUtil/Utils/Log.h"
#include "../CustomUi/QtMessageBox.h"
#include "../Platform/AppSetting.h"
#include "../Platform/NavigationManager.h"
#include "../include/Line.h"

#ifdef _WINDOWS
    #include <windows.h>
#endif // _WINDOWS


#define CONFIG_KEY_SAVEPASSWORD "config_key_savepassword"
#define CONFIG_KEY_DEFAULT "config_key_default"
#define CONFIG_KEY_AUTOLOGIN    "config_key_autologin"
#define CONFIG_KEY_USERNAME    "config_key_username"
#define CONFIG_KEY_PASSWORD    "config_key_password"
#define CONFIG_KEY_HEADPATH    "config_key_head"
#define CONFIG_KEY_DOMAIN      "config_key_domain"


LoginPanel::LoginPanel(QWidget *parent) :
    QDialog(parent)
{
//    _pManager = new UILoginMsgManager;
    _pListener = new UILoginMsgListener(this);
    _mousePressed = false;
    this->setAttribute(Qt::WA_TranslucentBackground); //设置顶层面板背景透明
    setWindowFlags(Qt::FramelessWindowHint);  //设置无边框
    setContentsMargins(10, 10, 10, 10);
    init();
//    UICom::getInstance()->setAcltiveMainWnd(this);
}

LoginPanel::~LoginPanel()
{
    if(_pListener)
        delete _pListener;

    if (nullptr != _pStLoginConfig)
    {
        delete _pStLoginConfig;
        _pStLoginConfig = nullptr;
    }

    if(nullptr != _pLocalServer)
    {
        delete _pLocalServer;
        _pLocalServer = nullptr;
    }
}

void LoginPanel::onGotLoginStatus(const QString &msg)
{
    QString newMsg (msg);
    int language = AppSetting::instance().getLanguage();

    if(QLocale::AnyLanguage == language)
        language = QLocale::system().language();

    if(language != QLocale::English)
    {
        if(msg == "opening database")
            newMsg = tr("正在打开数据库");
        else if(msg == "getting user information")
            newMsg = tr("正在获取用户信息");
        else if(msg == "getting group information")
            newMsg = tr("正在获取群信息");
        else if(msg == "initializing configuration")
            newMsg = tr("正在初始化配置");
        else if(msg == "getting user message")
            newMsg = tr("正在获取单人信息");
        else if(msg == "updating message read mask")
            newMsg = tr("正在更新单人阅读状态");
        else if(msg == "getting group message")
            newMsg = tr("正在获取群信息");
        else if(msg == "getting notice message")
            newMsg = tr("正在获取系统信息");
        else if(msg == "getting user state")
            newMsg = tr("正在获取用户状态");
        else if(msg == "login success")
            newMsg = tr("登录成功 正在启动");
        else if(msg.contains("getting user message:"))
            newMsg = newMsg.replace("getting user message:", tr("正在获取单人消息: "));
        else if(msg.contains("getting group message:"))
            newMsg = newMsg.replace("getting group message:", tr("正在获取群消息: "));
        else if(msg.contains("getting system message:"))
            newMsg = newMsg.replace("getting system message:", tr("正在获取系统消息: "));
    }

    emit showStatusMessage(newMsg);
}

/**
 *
 */
void LoginPanel::onSynDataSuccess()
{
    emit sgSynDataSuccess();
}

void LoginPanel::init()
{
    initLayout();
    connects();
}

/**
 *
 */
void LoginPanel::initLayout()
{
    _pNavManager = new NavManager(this);
    //
    this->setObjectName("loginMainPanel");
    this->setFixedSize(740, 560);
    auto *glay = new QHBoxLayout(this);
    glay->setMargin(0);
    glay->setSpacing(0);
    _loginMainFrm = new QFrame(this);
    _loginMainFrm->setObjectName("loginMainFrm");
    glay->addWidget(_loginMainFrm);
//    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
//    int picnum = qrand() % 18 + 1;
//    QString strStyle = QString("QFrame#loginMainFrm { border-radius:6px; background:rgba(5, 13, 23, 1);"
//                               "border-image:url(:/login/image1/loginbj/%1.png); }").arg(QString::number(picnum));
//    _loginMainFrm->setStyleSheet(strStyle);
#ifndef _MACOS
    auto *defaultShadow = new QGraphicsDropShadowEffect();
    defaultShadow->setBlurRadius(10.0);
    defaultShadow->setColor(QColor(0, 0, 0, 160));
    defaultShadow->setOffset(0, 0);
    _loginMainFrm->setGraphicsEffect(defaultShadow);
#endif
    _mainLayout = new QHBoxLayout(_loginMainFrm);
    _mainLayout->setMargin(0);
    _mainLayout->setSpacing(0);
    _leftFrame = new QFrame(this);
    _leftFrame->setObjectName("leftFrame");
    _leftFrame->setFixedWidth(280);
    _mainLayout->addWidget(_leftFrame);
    auto *leftFrmLay = new QVBoxLayout(_leftFrame);
    leftFrmLay->setMargin(0);
    leftFrmLay->setSpacing(0);
    _pAuthFailedFrm = new QFrame(this);
    _pAuthFailedFrm->setObjectName("AuthFailedFrm");
    _pAuthFailedFrm->setFixedHeight(40);
    leftFrmLay->addWidget(_pAuthFailedFrm);
    auto *authFailLay = new QHBoxLayout(_pAuthFailedFrm);
    auto *errPic = new HeadPhotoLab;
    errPic->setParent(this);
    errPic->setHead(":/login/image1/error.png", 10, false, false, true);
    authFailLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    authFailLay->addWidget(errPic);
    _pAuthFailLabel = new QLabel(this);
    _pAuthFailLabel->setObjectName("AuthFailedLabel");
    authFailLay->addWidget(_pAuthFailLabel);
    authFailLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    _pAuthFailedFrm->setVisible(false);
    _pAuthFailedSpaceFrm = new QFrame(this);
    _pAuthFailedSpaceFrm->setFixedHeight(40);
    leftFrmLay->addWidget(_pAuthFailedSpaceFrm);
    //
    auto *headhbox = new QHBoxLayout;
    leftFrmLay->addLayout(headhbox);
    headhbox->setContentsMargins(0, 10, 0, 0);
    _headPhotoLab = new HeadPhotoLab;
    _headPhotoLab->setParent(this);
#ifdef _STARTALK
    QString headPath = ":/QTalk/image1/StarTalk_defaultHead.png";
#else
    QString headPath = ":/QTalk/image1/headPortrait.png";
#endif
    _headPhotoLab->setHead(headPath, 52, false, true);
    headhbox->addItem(new QSpacerItem(10, 200, QSizePolicy::Minimum, QSizePolicy::Fixed));
    headhbox->addWidget(_headPhotoLab);
    //
    initloginWnd();
    leftFrmLay->addWidget(_pLoginFrm);
    //
    initLogingWnd();
    leftFrmLay->addWidget(_pLogingFrm);
    _pLogingFrm->setVisible(false);
    //
    QFrame *rightFrm = new QFrame(this);
    rightFrm->setObjectName("rightFrame");
    auto *rhbox = new QVBoxLayout(rightFrm);
    rhbox->setMargin(0);
    _mainLayout->addWidget(rightFrm);
    auto *thbox = new QHBoxLayout;
    rhbox->addLayout(thbox);
    thbox->addItem(new QSpacerItem(40, 1, QSizePolicy::Expanding));
    _closeBtn = new QPushButton(this);
    _closeBtn->setObjectName("closeBtn");
    _closeBtn->setFixedSize(30, 30);
    _closeBtn->setFocusPolicy(Qt::NoFocus);
    thbox->addWidget(_closeBtn);
    rhbox->addItem(new QSpacerItem(1, 40, QSizePolicy::Fixed, QSizePolicy::Expanding));
    auto *verLayout = new QHBoxLayout;
    //
    auto *waLabel = new QLabel(this);
    waLabel->setObjectName("wenanLabel");
    waLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    waLabel->setContentsMargins(15, 3, 0, 10);
    waLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    //
    verLayout->addWidget(waLabel);
    rhbox->addLayout(verLayout);
    leftFrmLay->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    //
    connect(_cancelLoginBtn, &QPushButton::clicked, [this]()
    {
        int ret = QtMessageBox::question(this, tr("提示"), tr("确认取消登录 ? "));

        if(ret == QtMessageBox::EM_BUTTON_YES)
        {
            QString program = QApplication::applicationFilePath();
            QStringList arguments;
            arguments << "AUTO_LOGIN=OFF" << "START_BY_STARTER=YES";
            QProcess::startDetached(program, arguments);
            QApplication::exit(0);
        }
    });
    connect(this, &LoginPanel::sgSetTip, waLabel, &QLabel::setText);
    QtConcurrent::run([this]()
    {
        qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
        QVector<QString> tips;
        QFile tipF(":/login/tips.json");
        QByteArray data;

        if(tipF.open(QIODevice::ReadOnly))
        {
            data = tipF.readAll();
            tipF.close();
        }

        QJsonDocument document = QJsonDocument::fromJson(data);

        if(!document.isNull() && document.isArray())
        {
            auto array = document.array();

            for(const auto &o : array)
                tips.push_back(o.toString());
        }

        QString tip;

        if (!tips.empty())
        {
            int tipIdex = qrand() % tips.size();
            tip = tips[tipIdex];
        }

        emit sgSetTip(tip);
    });
}

/**
 *
 */
void LoginPanel::connects()
{
    connect(_loginBtn, &QPushButton::clicked, this, &LoginPanel::onLoginBtnClicked);
    connect(this, &LoginPanel::sgStartLocalServer, this, &LoginPanel::onStartLocalServer, Qt::QueuedConnection);
    connect(_closeBtn, &QPushButton::clicked, [this]()
    {
        emit systemQuitSignal();
    });
    connect(this, &LoginPanel::sgSynDataSuccess, _pTimer, &QTimer::stop);
    connect(_severBtn, &QPushButton::clicked, [this](bool)
    {
        _pNavManager->showCenter(true, this);
    });
    connect(_userNameEdt, &QLineEdit::textChanged, [this](const QString & name)
    {
        _pDefaultConfig = nullptr;
        QString strName = name;
        strName = strName.replace(QRegExp("\\s{1,}"), "").toLower();
        QByteArray navName = _pNavManager->getNavName().toLocal8Bit();
        auto itFind = std::find_if(_pStLoginConfig->children.begin(), _pStLoginConfig->children.end(),
                                   [ strName, navName](QTalk::StConfig * tmpConf)
        {
            return tmpConf->attribute(CONFIG_KEY_USERNAME) == strName &&
                   (!tmpConf->hasAttribute(CONFIG_KEY_DOMAIN) ||
                    tmpConf->attribute(CONFIG_KEY_DOMAIN) == QString(navName));
        });

        if (itFind != _pStLoginConfig->children.end())
        {
            _pDefaultConfig = *itFind;
            //
            QString headPath = _pDefaultConfig->attribute(CONFIG_KEY_HEADPATH);
            QString password = _pDefaultConfig->attribute(CONFIG_KEY_PASSWORD);
            setHead(headPath);

            if(_rememberPassBtn->isChecked())
                _passworldEdt->setText(password);
        }
        else
        {
            setHead("");
            _passworldEdt->setText("");
        }
    });
}

/**
 *
 */
void LoginPanel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        _mousePressed = true;
        _mousePoint = e->globalPos() - this->pos();
        e->accept();
    }

    QDialog::mousePressEvent(e);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author
  * @date 2018.9.24
  */
void LoginPanel::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    _mousePressed = false;
    QDialog::mouseReleaseEvent(e);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author
  * @date 2018.9.24
  */
void LoginPanel::mouseMoveEvent(QMouseEvent *e)
{
    if (_mousePressed)
    {
        this->move(e->globalPos() - _mousePoint);
        e->accept();
    }

    QDialog::mouseMoveEvent(e);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author
  * @date 2018.9.24
  */
void LoginPanel::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)
    QDialog::closeEvent(e);
}

bool LoginPanel::eventFilter(QObject *o, QEvent *e)
{
    return QDialog::eventFilter(o, e);
}

//
void LoginPanel::loadConf()
{
    //std::thread([this](){
    //    QString userDirPath = AppSetting::instance().getUserDirectory().data();
    //    QDir dir(userDirPath);
    //    QSet<QString> users;
    //    auto lst = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    //    for(const QFileInfo& info : lst )
    //    {
    //        if(info.fileName().contains("@"))
    //        {
    //            QString fileName = info.fileName();
    //            int pos = fileName.lastIndexOf("_");
    //            fileName = fileName.mid(0, pos);
    //            users.insert(fileName);
    //        }
    //    }
    //    QString strUsers;
    //    for(const QString& user : users)
    //    {
    //        strUsers.append(user);
    //        strUsers.append("|");
    //    }
    //    if(_pManager)
    //    {
    //        UILoginMsgManager::startUpdater(strUsers.toStdString());
    //    }
    //}).detach();
    //
    std::string usrDir = AppSetting::instance().getUserDirectory();
    //
    _strConfPath = QString("%1/login.data").arg(PLAT.getConfigPath().c_str());

    if(nullptr != _pStLoginConfig)
        delete _pStLoginConfig;

    _pStLoginConfig = new QTalk::StConfig;

    if (QFile::exists(_strConfPath))
    {
        QTalk::qConfig::loadConfig(_strConfPath, true, _pStLoginConfig);

        if (_pStLoginConfig->tagName != "loginConf")
        {
            //
            // 这里这个文件已经有问题了，要干掉生成新的。by dan.liu
            QFile::remove(_strConfPath);
            _pStLoginConfig->tagName = "loginConf";
            return;
        }

        QString navName = "";

        if(nullptr != _pNavManager)
            navName = _pNavManager->getNavName();

        //
        _pDefaultConfig = nullptr;
        QStringList users;

        for (QTalk::StConfig *tmpConf : _pStLoginConfig->children)
        {
            // 非此域
            if(tmpConf->hasAttribute(CONFIG_KEY_DOMAIN) && tmpConf->attribute(CONFIG_KEY_DOMAIN) != navName)
                continue;

            if (tmpConf->hasAttribute(CONFIG_KEY_DEFAULT) &&
                    tmpConf->attribute(CONFIG_KEY_DEFAULT).toInt())
                _pDefaultConfig = tmpConf;

            users << tmpConf->attribute(CONFIG_KEY_USERNAME);
        }

        if(!users.isEmpty() && _userNameEdt)
        {
            _userNameCompleter = new QCompleter(users, this);
            _userNameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
            _userNameEdt->setCompleter(_userNameCompleter);
        }

        if (nullptr != _pDefaultConfig)
        {
            int savePassword = _pDefaultConfig->attribute(CONFIG_KEY_SAVEPASSWORD).toInt();
            int autoLogin = _pDefaultConfig->attribute(CONFIG_KEY_AUTOLOGIN).toInt();
            AppSetting::instance().setAutoLoginEnable(autoLogin);
            QString userName = _pDefaultConfig->attribute(CONFIG_KEY_USERNAME);
            QString headPath = _pDefaultConfig->attribute(CONFIG_KEY_HEADPATH);
            QString password = _pDefaultConfig->attribute(CONFIG_KEY_PASSWORD);
            _userNameEdt->setText(userName);
            _rememberPassBtn->setChecked(savePassword);
            setHead(headPath);

            if (savePassword)
            {
                _autoLoginBtn->setChecked(autoLogin);
                _passworldEdt->setText(password);

                if (autoLogin && _enableAutoLogin)
                    _loginBtn->clicked();
            }
        }
        else
        {
            _userNameEdt->clear();
            _userNameEdt->setFocus();
            setHead("");
        }
    }
    else
        _pStLoginConfig->tagName = "loginConf";
}

void LoginPanel::initloginWnd()
{
    _pLoginFrm = new QFrame(this);
    auto *loginLayout = new QVBoxLayout(_pLoginFrm);
    loginLayout->setMargin(0);
    loginLayout->setSpacing(0);
    auto *userpasslay = new QVBoxLayout;
    loginLayout->addLayout(userpasslay);
    userpasslay->setContentsMargins(30, 30, 30, 0);
    _userNameFrm = new QFrame(this);
    _userNameFrm->setObjectName("userNameFrm");
    _userNameFrm->setFixedSize(220, 35);
    userpasslay->addWidget(_userNameFrm);
    userpasslay->addWidget(new Line(this));
    auto *userNamelay = new QHBoxLayout(_userNameFrm);
    userNamelay->setMargin(0);
    userNamelay->setSpacing(0);
    _userNameEdt = new QLineEdit(this);
    _userNameEdt->setAttribute(Qt::WA_InputMethodEnabled, false);
    _userNameEdt->setPlaceholderText(tr("请输入账号"));
    _userNameEdt->setObjectName("userNameEdt");
    userNamelay->addWidget(_userNameEdt);
    _passworldFrm = new QFrame(this);
    _passworldFrm->setObjectName("passworldFrm");
    _passworldFrm->setFixedSize(220, 35);
    userpasslay->addWidget(_passworldFrm);
    userpasslay->addWidget(new Line(this));
    auto *passworldlay = new QHBoxLayout(_passworldFrm);
    passworldlay->setMargin(0);
    passworldlay->setSpacing(0);
    _passworldEdt = new QLineEdit(this);
    _passworldEdt->setAttribute(Qt::WA_InputMethodEnabled, false);
    _passworldEdt->setPlaceholderText(tr("请输入密码"));
    _passworldEdt->setObjectName("passworldEdt");
    _passworldEdt->setEchoMode(QLineEdit::Password);
    auto *passwordBtn = new QToolButton(this);
    passwordBtn->setObjectName("PasswordBtn");
    passwordBtn->setCheckable(true);
    passwordBtn->setFixedSize(20, 20);
    passworldlay->addWidget(_passworldEdt);
    passworldlay->addWidget(passwordBtn);
    _loginBtn = new QPushButton(this);
    _loginBtn->setObjectName("loginBtn");
    _loginBtn->setFixedSize(30, 30);
    passworldlay->addWidget(_loginBtn);
    _settingBtn = new QPushButton(this);
    _settingBtn->setObjectName("settingBtn");
    _settingBtn->setFixedSize(30, 30);
    _settingBtn->setCheckable(true);
    auto *settingBtnlay = new QHBoxLayout;
    loginLayout->addLayout(settingBtnlay);
    settingBtnlay->setContentsMargins(0, 20, 0, 0);
    settingBtnlay->addWidget(_settingBtn);
    _settingFrm = new QFrame(this);
    _settingFrm->setObjectName("settingFrm");
    loginLayout->addWidget(_settingFrm);
    auto *radioLay = new QHBoxLayout;
    radioLay->setContentsMargins(30, 15, 30, 10);
    _rememberPassBtn = new QCheckBox(tr("记住密码"));
    radioLay->addWidget(_rememberPassBtn);
    radioLay->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    _autoLoginBtn = new QCheckBox(tr("自动登录"));
    radioLay->addWidget(_autoLoginBtn);
    auto *serverLay = new QHBoxLayout;
    serverLay->setMargin(15);
    _severBtn = new QPushButton(tr("服务器"), this);
    _severBtn->setObjectName("severBtn");
    _severBtn->setFixedSize(72, 24);
    serverLay->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    serverLay->addWidget(_severBtn);
    serverLay->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    // version label
    std::string globalversion = PLAT.getGlobalVersion();
    QLabel *verLabel = new QLabel(QString("V.%1").arg(globalversion.c_str()), this);
    verLabel->setObjectName("GlobalVersion");
    verLabel->setAlignment(Qt::AlignCenter);
    //
    auto *setttingLayout = new QVBoxLayout(_settingFrm);
    setttingLayout->setMargin(0);
    setttingLayout->addLayout(radioLay);
    setttingLayout->addLayout(serverLay);
    QFrame *spaceFrm = new QFrame(this);
    spaceFrm->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    loginLayout->addWidget(spaceFrm);
    loginLayout->addWidget(verLabel);
    connect(_passworldEdt, &QLineEdit::textChanged, [this] { style()->polish(_passworldEdt); });
    connect(_userNameEdt, &QLineEdit::textChanged, [this] { style()->polish(_userNameEdt); });
    //
    //_settingBtn
    connect(_settingBtn, &QPushButton::clicked, [this, spaceFrm](bool checked)
    {
        _settingFrm->adjustSize();
        spaceFrm->setFixedSize(_settingFrm->size());
        spaceFrm->setVisible(checked);
        _settingFrm->setVisible(!checked);
    });
    _settingBtn->click();
    //
    connect(passwordBtn, &QToolButton::clicked, [this](bool checked)
    {
        _passworldEdt->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
}

/**
 *
 */
void LoginPanel::initLogingWnd()
{
    _pLogingFrm = new QFrame(this);
    auto *logingLayout = new QVBoxLayout(_pLogingFrm);
    logingLayout->setMargin(0);
    logingLayout->setSpacing(15);
    //
    auto *logingLabel = new QLabel(this);
    logingLabel->setText(tr("登录中"));
    logingLabel->setObjectName("LogingWnd_Loging");
    logingLabel->setAlignment(Qt::AlignCenter);
    logingLayout->addWidget(logingLabel);
    //
    _pStsLabel = new QLabel();
    _pStsLabel->setObjectName("LogingWnd_status");
    _pStsLabel->setAlignment(Qt::AlignCenter);
    logingLayout->addWidget(_pStsLabel);
    //
    _cancelLoginBtn = new QPushButton(tr("取消登录"));
    _cancelLoginBtn->setFixedHeight(26);
    _cancelLoginBtn->setObjectName("cancelLoginButton");
    auto *cancelLay = new QHBoxLayout;
    cancelLay->setContentsMargins(0, 80, 0, 0);
    cancelLay->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));
    cancelLay->addWidget(_cancelLoginBtn);
    cancelLay->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));
    logingLayout->addLayout(cancelLay);
    //
    _timeTimes = 0;
    _pTimer = new QTimer();
    _pTimer->setInterval(500);
    connect(_pTimer, &QTimer::timeout, [this, logingLabel]()
    {
        _timeTimes++;

        if (_timeTimes == 4)
            _timeTimes = 0;

        QString strText = QString(tr("登录中")) + QString(_timeTimes, QChar('.'));
        logingLabel->setText(strText);
    });
    _pTimer->start();
}

void LoginPanel::authFailed()
{
    emit AuthFailedSignal(tr("账户或密码错误!"));
}

bool LoginPanel::getAutoLoginFlag()
{
    if (_pDefaultConfig)
    {
        int isok = _pDefaultConfig->attribute(CONFIG_KEY_AUTOLOGIN).toInt();
        return (bool) isok;
    }

    return false;
}

void LoginPanel::setAutoLoginFlag(bool flag)
{
    if (_pDefaultConfig)
    {
        _autoLoginBtn->setChecked(flag);
        _pDefaultConfig->setAttribute(CONFIG_KEY_AUTOLOGIN, QString::number(flag));
        QTalk::qConfig::saveConfig(_strConfPath, true, _pStLoginConfig);
    }
    else
        error_log("set auto login failed, no default config");
}

void LoginPanel::onLoginBtnClicked()
{
    // init
    static bool isInit  = false;

    if(!isInit)
    {
        isInit = true;
        connect(this, &LoginPanel::showStatusMessage, _pStsLabel, &QLabel::setText, Qt::QueuedConnection);
        connect(this, &LoginPanel::AuthFailedSignal, this, &LoginPanel::onAuthFailed);
    }

    // submit check
    static qint64 submit_time = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if(now - submit_time < 1000)
        return;

    submit_time = now;
    //
    QString strName = _userNameEdt->text();
    QString strPassword = _passworldEdt->text();
    QString domain = _pNavManager->getDefaultDomain();
    QByteArray navName = _pNavManager->getNavName().toLocal8Bit();

    if (strName.isEmpty() || strPassword.isEmpty())
    {
        emit AuthFailedSignal(tr("账户或密码不能为空!"));
        return;
    }

    if(domain.isEmpty())
    {
        emit AuthFailedSignal(tr("导航配置出错!"));
        return;
    }

    //
    strName = strName.replace(QRegExp("\\s{1,}"), "").toLower();

    if(nullptr == _pLocalServer)
        _pLocalServer = new QLocalServer;

#ifndef _DEBUG
    // 单进程监听
    QLocalSocket localSocket;
    QString listenName = QString("%1@%2").arg(strName, domain);
    localSocket.connectToServer(listenName);

    if (localSocket.waitForConnected(2000))
    {
        localSocket.disconnectFromServer();
        localSocket.close();
        emit AuthFailedSignal(tr("该账号已登录!"));
        return;
    }

#endif
    // 设置画面显隐
    _pLoginFrm->setVisible(false);
    _pLogingFrm->setVisible(true);
    _pAuthFailedFrm->setVisible(false);
    _pAuthFailedSpaceFrm->setVisible(true);

    // 设置登陆相关配置文件
    if (nullptr == _pDefaultConfig)
    {
        _pDefaultConfig = new QTalk::StConfig("item");
        _pDefaultConfig->setAttribute(CONFIG_KEY_DEFAULT, true);
        _pStLoginConfig->addChild(_pDefaultConfig);
    }

    _pDefaultConfig->setAttribute(CONFIG_KEY_USERNAME, strName);
    _pDefaultConfig->setAttribute(CONFIG_KEY_DOMAIN, QString(navName.data()));

    if (_rememberPassBtn->isChecked())
    {
        _pDefaultConfig->setAttribute(CONFIG_KEY_PASSWORD, strPassword);
//        _pDefaultConfig->setAttribute(CONFIG_KEY_SAVEPASSWORD, true);
//        _pDefaultConfig->setAttribute(CONFIG_KEY_AUTOLOGIN, _autoLoginBtn->isChecked());
    }
    else
    {
        _pDefaultConfig->setAttribute(CONFIG_KEY_SAVEPASSWORD, false);
        _pDefaultConfig->setAttribute(CONFIG_KEY_AUTOLOGIN, false);
    }

    // 设置默认账户
    for (QTalk::StConfig *tmpConf : _pStLoginConfig->children)
    {
        if(tmpConf->hasAttribute(CONFIG_KEY_DOMAIN) && tmpConf->attribute(CONFIG_KEY_DOMAIN) != navName.data())
            continue;

        QString tmpName = tmpConf->attribute(CONFIG_KEY_USERNAME);
        tmpConf->setAttribute(CONFIG_KEY_DEFAULT, tmpName == strName);
    }

    //
    QTalk::qConfig::saveConfig(_strConfPath, true, _pStLoginConfig);
    // 开始登陆
    // 获取导航信息
    std::string nav = _pNavManager->getDefaultNavUrl().toStdString();

    if(nav.empty())
        emit AuthFailedSignal(tr("导航不能为空!"));

    //
    PLAT.setLoginNav(nav);
    //
    PLAT.setNavName(navName.toStdString());
    //
    _pStsLabel->setText(tr("正在获取导航信息"));
    QFuture<bool> curRet = QtConcurrent::run(&UILoginMsgManager::getNavInfo, nav);

//        curRet.waitForFinished();
    while (!curRet.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);

    bool ret = curRet.result();

    if (!ret)
    {
        emit AuthFailedSignal(tr("导航获取失败, 请检查网络连接!"));
        return;
    }

    // check server host
    const std::string host = NavigationManager::instance().getXmppHost();
    const int port = NavigationManager::instance().getProbufPort();

    if (port == 0 || host.empty())
    {
        warn_log("nav info error (port == 0 || domain.empty())");
        emit AuthFailedSignal(tr("获取服务器地址失败!"));
        return;
    }

    // try connect to server
//    if(0)
//    {
//        auto future = QtConcurrent::run([this, host, port]()
//        {
//            _pStsLabel->setText(tr("正在尝试连接服务器"));
//            std::unique_ptr<QTcpSocket> tcpSocket(new QTcpSocket);
//            tcpSocket->connectToHost(host.data(), port);
//            if(!tcpSocket->waitForConnected(5000))
//            {
//                tcpSocket->abort();
//                return false;
//            }
//            tcpSocket->close();
//            return true;
//        });
//        while (!future.isFinished())
//            QApplication::processEvents(QEventLoop::AllEvents, 500);
//        if(!future.result())
//        {
//            emit AuthFailedSignal(tr("连接服务器失败!"));
//            return;
//        }
//    }
    // login
    _pStsLabel->setText(tr("正在验证账户信息"));
    QtConcurrent::run([strName, strPassword]()
    {
        UILoginMsgManager::SendLoginMessage(strName.toStdString(), strPassword.toStdString());
    });
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/11/08
  */
void LoginPanel::onAuthFailed(const QString &msg)
{
    error_log("login error :{0}", msg.toStdString());
    QString tmpMsg = msg;
    tmpMsg.replace("\"", " ");
    tmpMsg.replace("\n", " ");
    _pAuthFailLabel->setText(msg);
    _pLoginFrm->setVisible(true);
    _pLogingFrm->setVisible(false);
    _pAuthFailedFrm->setVisible(true);
    _pAuthFailedSpaceFrm->setVisible(false);
}

/**
 * 根据导航地址获取domain
 * @return
 */
QString LoginPanel::getDomainByNav(const QString &nav)
{
    return QString::fromStdString(UILoginMsgManager::getNavDomain(nav.toStdString()));
}

void LoginPanel::saveHeadPath()
{
    if (_pDefaultConfig)
    {
        std::string selfUserId = PLAT.getSelfXmppId();
        std::shared_ptr<QTalk::Entity::ImUserInfo> info = DB_PLAT.getUserInfo(selfUserId, true);

        if (info)
        {
            std::string head = QTalk::GetHeadPathByUrl(info->HeaderSrc);
            _pDefaultConfig->setAttribute(CONFIG_KEY_HEADPATH, QString::fromStdString(head));
            QTalk::qConfig::saveConfig(_strConfPath, true, _pStLoginConfig);
        }
    }
}

void LoginPanel::setHead(const QString &headPath)
{
    QFileInfo info(headPath);

    if (!headPath.isEmpty() && info.exists() && info.isFile())
        _headPhotoLab->setHead(headPath, 52, false, true);
    else
    {
#ifdef _STARTALK
        _headPhotoLab->setHead(":/QTalk/image1/StarTalk_defaultHead.png", 52, false, true);
#else
        _headPhotoLab->setHead(":/QTalk/image1/headPortrait.png", 52, false, true);
#endif
    }
}

/**
 *
 * @param errMs
 */
void LoginPanel::loginError(const std::string &errMs)
{
    QString errorMsg;

    if("not-authorized" == errMs || "cancel-rsa" == errMs)
        errorMsg = tr("账户或密码错误");
    else if("out_of_date" == errMs)
        errorMsg = tr("登录凭证失效");
    else
        errorMsg = QString::fromStdString(errMs);

    error_log(errMs);
    emit AuthFailedSignal(errorMsg);
}

/**
 *
 */
void LoginPanel::loginSuccess()
{
    if (_pDefaultConfig && _rememberPassBtn->isChecked())
    {
        _pDefaultConfig->setAttribute(CONFIG_KEY_SAVEPASSWORD, true);
        _pDefaultConfig->setAttribute(CONFIG_KEY_AUTOLOGIN, _autoLoginBtn->isChecked());
        QTalk::qConfig::saveConfig(_strConfPath, true, _pStLoginConfig);
    }
}

void LoginPanel::onStartLocalServer()
{
    static bool isListen = false;
#ifndef _DEBUG
    // 监听
    QString strName = PLAT.getSelfUserId().data();
    QString domain = _pNavManager->getDefaultDomain();
    QString listenName = QString("%1@%2").arg(strName, domain);

    if(!isListen && _pLocalServer)
    {
        isListen = true;
        QLocalServer::removeServer(listenName);
        _pLocalServer->listen(listenName);
    }

#endif
}

/**
 *
 */
void LoginPanel::enableAutoLogin(bool enable)
{
    _enableAutoLogin = enable;
}

bool LoginPanel::event(QEvent *e)
{
    if(e->type() == QEvent::Show)
    {
        QString strStyle = QString("QFrame#loginMainFrm {"
                                   "border-image:url(:/login/image1/loginbj/%1.png); }").arg(qrand() % 18 + 1);
        _loginMainFrm->setStyleSheet(strStyle);
    }

    return QWidget::event(e);
}

//void LoginPanel::paintEvent(QPaintEvent *e) {
//
//    static bool hasPainted = false;
//    if(!hasPainted)
//    {
//        qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
//        int picnum = qrand() % 18 + 1;
//        QString path = QString(":/login/image1/loginbj/%1.png").arg(picnum);
//        QPainter painter(this);
//        painter.save();
//        painter.drawPixmap(_loginMainFrm->geometry(), QPixmap(path));
//        painter.restore();
//        hasPainted = true;
//    }
//    //
//    QWidget::paintEvent(e);
//}
