#include "StatusWgt.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QPainter>
#include "../Platform/dbPlatForm.h"
#include "../UICom/UIEntity.h"
#include "../include/im_enum.h"
#include "ChatViewMainPanel.h"
#include "../QtUtil/Utils/Log.h"
#include "../Platform/Platform.h"
#include "../Platform/NavigationManager.h"
#include "../WebService/WebService.h"

#define DEM_BTN_ICON_LEN 30

extern ChatViewMainPanel *g_pMainPanel;

/**
 *
 */
void StatusLabel::setStatus(Status sts)
{
    _sts = sts;
    update();
    switch (sts)
    {
    case EM_STS_ONLINE:
        setToolTip(tr("在线"));
        break;
    case EM_STS_BUSY:
        setToolTip(tr("繁忙"));
        break;
    case EM_STS_AWAY:
        setToolTip(tr("离开"));
        break;
    case EM_STS_OFFLINE:
        setToolTip(tr("离线"));
        break;
    default:
    case EM_STS_INVALID:
        setToolTip("");
        break;
    }
}

void StatusLabel::paintEvent(QPaintEvent *e)
{
    QPainter painter;
    painter.begin(this);
    //
    QColor brushColor;
    switch (_sts)
    {
    case EM_STS_ONLINE:
        brushColor = QColor(82, 196, 26);
        break;
    case EM_STS_BUSY:
        brushColor = QColor(250, 173, 20);
        break;
    case EM_STS_AWAY:
        brushColor = QColor(255, 78, 63);
        break;
    case EM_STS_OFFLINE:
        brushColor = QColor(181, 181, 181);
        break;
    default:
    case EM_STS_INVALID:
        return;
    }
    //
    painter.setBrush(QBrush(brushColor));
    painter.setPen(Qt::NoPen);
    QRect rect = this->contentsRect();
    QRect cons = QRect((rect.width() - 8) / 2, (rect.height() - 8) / 2, 8, 8);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.drawEllipse(cons);
    painter.end();
    QLabel::paintEvent(e);
}

/***/
StatusWgt::StatusWgt(QWidget *parent)
    : QFrame(parent), _isGroupChat(false)
{
    initUi();

    connect(this, &StatusWgt::updateName, this, &StatusWgt::setName);
    connect(this, &StatusWgt::updateMood, this, &StatusWgt::setMood);
}

StatusWgt::~StatusWgt() = default;

/**
  * @函数名   switchUser
  * @功能描述 相应切换用户
  * @参数     t群/个人 userName 用户名
     void
  * @author   cc
  * @date     2018/09/19
  */
void StatusWgt::switchUser(QUInt8 t, const QTalk::Entity::UID &uid, const QString &userName)
{
    _isGroupChat = (t == QTalk::Enum::ChatType::GroupChat);
    _isConsultServer = (t == QTalk::Enum::ChatType::ConsultServer);
    _uid = uid;

    //    _pEdit->setVisible(_isGroupChat);
    _pLabelPlat->setVisible(false);
    _pStsLabel->setVisible(!_isGroupChat);
    _pmood->setVisible(!_isGroupChat);
    _pBtnStructure->setVisible(!_isGroupChat && PLAT.getShowStaff());
    _medalWgt->setVisible(t == QTalk::Enum::TwoPersonChat);

    if (t == QTalk::Enum::TwoPersonChat)
    {
        onUpdateMedal();
    }

    if (_isGroupChat)
    {
        //        std::string coEdit = AppSetting::instance().getCoEdit();
        _pBtnAddGroup->setToolTip(tr("邀请群成员"));
        this->setMinimumHeight(50);
    }
    else
    {
        this->setMinimumHeight(70);
        _pBtnAddGroup->setToolTip(tr("创建群组"));
    }

    QFontMetricsF uf(_pLabelChatUser->font());
    QString name = uf.elidedText(userName, Qt::ElideRight, 300);

    QFontMetricsF mf(_pmood->font());

    //群用户名为空时从数据库取一次 其他的每次都从数据库取
    if (nullptr != _pLabelChatUser)
    {
        if (_isGroupChat)
        {
            if (!userName.isEmpty())
            {
                setName(name);
            }
            else
            {
                std::shared_ptr<QTalk::Entity::ImGroupInfo> groupInfo = DB_PLAT.getGroupInfo(
                    uid.usrId());
                if (groupInfo)
                    setName(QString::fromStdString(groupInfo->Name));
                else
                    setName(uid.qUsrId().section("@", 0, 0));
            }
        }
        else
        {
            std::shared_ptr<QTalk::Entity::ImUserInfo> userInfo = DB_PLAT.getUserInfo(
                uid.realId());
            if (userInfo)
            {
                setName(QString::fromStdString(QTalk::getUserName(userInfo)));
                _pBtnStructure->setToolTip(QString::fromStdString(userInfo->DescInfo));
                QString mood = QString::fromStdString(userInfo->Mood);
                _pmood->setToolTip(mood);
                mood = mf.elidedText(mood, Qt::ElideRight, 500);
                mood = mood.replace("\n", " ");
                _pmood->setText(mood);
            }
            else
            {
                _pmood->setText("");
                if (userName.isEmpty())
                    setName(uid.qReadJid().section("@", 0, 0));
                else
                    setName(name);
            }
            // 显示状态
            auto status = PLAT.getUserStatus(uid.realId());
            updateUserSts(status.data());
        }
    }
}

/**
 * 
 */
void StatusWgt::updateUserSts(const QString &sts)
{
    if (sts == "online")
    {
        _pStsLabel->setStatus(EM_STS_ONLINE);
    }
    else if (sts == "busy")
    {
        _pStsLabel->setStatus(EM_STS_BUSY);
    }
    else if (sts == "away")
    {
        _pStsLabel->setStatus(EM_STS_AWAY);
    }
    else if (sts == "offline")
    {
        _pStsLabel->setStatus(EM_STS_OFFLINE);
    }
}

/**
  * @函数名   initUi
  * @功能描述 初始化画面
  * @参数
  * @author   cc
  * @date     2018/09/19
  */
void StatusWgt::initUi()
{
    this->setObjectName("StatusWgt");
    this->setMinimumHeight(50);
    this->setMaximumHeight(68);
    //
    _pLabelChatUser = new QLabel(this);
    _pLabelChatUser->setMinimumHeight(20);
    _pLabelChatUser->setTextFormat(Qt::PlainText);
    _pLabelChatUser->setWordWrap(false);
    _pLabelPlat = new HeadPhotoLab;
    _pLabelPlat->setParent(this);
    _pmood = new QLabel(this);
    _pStsLabel = new StatusLabel(this);
    _pBtnStructure = new QPushButton(this);
    _pBtnAddGroup = new QPushButton(this);
    _pBtnLock = new QPushButton(this);
    //    _pEdit = new QPushButton(this);
    _medalWgt = new MedalWgt(15, this);

    _pLabelChatUser->setObjectName("ChatUser");
    _pLabelPlat->setObjectName("UserDept");
    _pmood->setObjectName("Mood");
    _pBtnStructure->setObjectName("Structure");
    _pBtnAddGroup->setObjectName("AddGroup");
    _pBtnLock->setObjectName("Lock");
    //    _pEdit->setObjectName("CO_Edit");
    _pLabelChatUser->installEventFilter(this);
    _pStsLabel->setFixedWidth(12);
    _pLabelPlat->setFixedSize(20, 20);

    _pmood->setTextFormat(Qt::PlainText);
    //
    _pBtnStructure->setToolTip(tr("组织架构"));

    //
    _pBtnStructure->setFixedSize(DEM_BTN_ICON_LEN, DEM_BTN_ICON_LEN);
    //    _pEdit->setFixedSize(DEM_BTN_ICON_LEN, DEM_BTN_ICON_LEN);
    _pBtnAddGroup->setFixedSize(DEM_BTN_ICON_LEN, DEM_BTN_ICON_LEN);
    _pBtnLock->setFixedSize(DEM_BTN_ICON_LEN, DEM_BTN_ICON_LEN);

    if (!_isGroupChat && _uid.isConsult())
    {
        _pBtnAddGroup->setVisible(false);
        //        _pBtnStructure->setVisible(false);
    }

    //    std::string coEdit = AppSetting::instance().getCoEdit();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 10, 15, 10);

    auto *vlayout = new QVBoxLayout();
    auto *hlayout = new QHBoxLayout();
    hlayout->addWidget(_pStsLabel);
    hlayout->addWidget(_pLabelChatUser);
    hlayout->addWidget(_pLabelPlat);
    hlayout->addWidget(_medalWgt);
    hlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

    vlayout->addLayout(hlayout);
    _pmood->setContentsMargins(18, 0, 0, 0);
    vlayout->addWidget(_pmood);

    layout->addLayout(vlayout);
    layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));
    layout->addWidget(_pBtnStructure);
    //    layout->addWidget(_pEdit);
    layout->addWidget(_pBtnAddGroup);
    layout->addWidget(_pBtnLock);
    layout->setAlignment(vlayout, Qt::AlignVCenter);
    setLayout(layout);
    _pBtnLock->setVisible(false);
    connect(_pBtnAddGroup, &QPushButton::clicked, [this]() {
        if (g_pMainPanel)
        {
            if (_isGroupChat)
            {
                emit g_pMainPanel->sgOperator(tr("加入群组"));
                emit g_pMainPanel->addGroupMember(_uid.qUsrId());
            }
            else
            {
                emit g_pMainPanel->sgOperator(tr("创建群组"));
                emit g_pMainPanel->creatGroup(_uid.qReadJid());
            }
        }
    });

    connect(_pBtnStructure, &QPushButton::clicked, [this]() {
        if (g_pMainPanel)
        {
            emit g_pMainPanel->sgOperator(tr("组织架构"));
            emit g_pMainPanel->sgSwitchCurFun(1);
            emit g_pMainPanel->sgJumpToStructre(_uid.qReadJid());
        }
    });

    //    connect(_pEdit, &QPushButton::clicked, [this, coEdit]() {
    //
    //        const std::string groupId = _uid.usrId();
    //        QString tmpUrl = coEdit.data();
    //        QString url = coEdit.data();;
    //
    //        if(coEdit.empty())
    //        {
    //            url = QString("%1/qtalkpad/docs/").arg(NavigationManager::instance().getHttpHost().data());
    //        }
    //        url.append(tmpUrl.contains("?") ? "&" : "?").append("to=").append(groupId.data());
    //        WebService::loadCoEdit(QUrl(url));
    //    });
}

void StatusWgt::setName(const QString &name)
{
    if (_pLabelChatUser)
    {
        QFontMetricsF f(_pLabelChatUser->font());
        _pLabelChatUser->setToolTip(name);
        auto newName = name;
        newName.replace("\n", " ");
        newName.replace("\r\n", " ");
        _pLabelChatUser->setText(f.elidedText(newName, Qt::ElideRight, 300));
    }
}

void StatusWgt::showResource(const std::string &resource)
{

    _pLabelPlat->setVisible(true);
    QString res = QString::fromStdString(resource);
    QStringList lst = res.split("]");
    QString version, plat;
    for (const auto &item : lst)
    {
        if (item.startsWith("V[") || item.startsWith("_V["))
        {
            version = item.mid(item.indexOf('[') + 1, item.size() - 1);
        }
        else if (item.startsWith("P[") || item.startsWith("_P["))
        {
            auto index = item.indexOf('[');
            plat = item.mid(index + 1, item.size() - 1);
        }
    }

    if (!plat.isEmpty())
    {
        if (!version.isEmpty())
            version = QString("%1_%2").arg(plat, version);

        if (plat == "Mac" ||
            plat == "LINUX" ||
            plat == "PC64" ||
            plat == "PC32")
        {
            _pLabelPlat->setHead(":/chatview/image1/StatusBar/pc_plat.png", 9, false, false, true);
        }
        else
        {
            _pLabelPlat->setHead(":/chatview/image1/StatusBar/phone_plat.png", 9, false, false, true);
        }
    }

    if (!version.isEmpty())
    {
        _pLabelPlat->setToolTip(version);
    }
}

/**
 *
 * @param o
 * @param e
 * @return
 */
bool StatusWgt::eventFilter(QObject *o, QEvent *e)
{

    if (o == _pLabelChatUser)
    {
        if (e->type() == QEvent::Enter)
        {
            setCursor(Qt::PointingHandCursor);
        }
        else if (e->type() == QEvent::Leave)
        {
            setCursor(Qt::ArrowCursor);
        }
        else if (e->type() == QEvent::MouseButtonPress)
        {
            if (_isGroupChat)
                emit g_pMainPanel->showGroupCardSignal(_uid.qUsrId());
            else
                emit g_pMainPanel->showUserCard(_uid.qReadJid());
        }
    }
    return QObject::eventFilter(o, e);
}

void StatusWgt::setMood(const QString &mood)
{
    QFontMetricsF mf(_pmood->font());
    _pmood->setToolTip(mood);
    QString eMood = mf.elidedText(mood, Qt::ElideRight, 300);
    eMood = eMood.replace("\n", " ");
    _pmood->setText(eMood);
}

void StatusWgt::onUpdateMedal()
{
    std::set<QTalk::StUserMedal> user_medal;
    g_pMainPanel->getUserMedal(_uid.usrId(), user_medal);
    _medalWgt->addMedals(user_medal, true);
}