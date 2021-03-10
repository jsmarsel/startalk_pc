//
// Created by cc on 2019/12/25.
//

#include "ChatViewItem.h"
#include "ChatViewMainPanel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QSplitter>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <sstream>
#include "ChatMainWgt.h"
#include "StatusWgt.h"
#include "InputWgt.h"
#include "ToolWgt.h"
#include "ChatUtil.h"
#include "GroupMember.h"
#include "GroupTopic.h"
#include "GroupChatSidebar.h"
#include "ShareMessageFrm.h"
#include "../Emoticon/EmoticonMainWgt.h"
#include "../QtUtil/Entity/JID.h"
#include "../entity/UID.h"
#include "../Platform/dbPlatForm.h"
#include "MessageAnalysis.h"
#include "search/LocalSearchMainWgt.h"
#include "../Platform/Platform.h"
#include "../QtUtil/Utils/utils.h"

#define DEM_ATALL_STR "@all"

using namespace std;
using namespace QTalk;
extern ChatViewMainPanel *g_pMainPanel;
ChatViewItem::ChatViewItem()
    : QFrame(),
      _leftLay(nullptr),
      _sendBtnLay(nullptr),
      _sendBtn(nullptr)
{
    //
    initUi();
    //
    qRegisterMetaType<Entity::ImMessageInfo>("Entity::ImMessageInfo");
    qRegisterMetaType<std::string>("std::string");
    connect(this, &ChatViewItem::sgRemoveGroupMember, _pInputWgt, &InputWgt::removeGroupMember);

    if(_pGroupSidebar)
        connect(this, &ChatViewItem::sgRemoveGroupMember, _pGroupSidebar->_pGroupMember, &GroupMember::deleteMember);

    connect(this, &ChatViewItem::sgUpdateUserStatus, _pStatusWgt, &StatusWgt::updateUserSts);
    connect(_pShareMessageFrm, &ShareMessageFrm::sgSetShareMessageState, this, &ChatViewItem::setShareMessageState);
    connect(_pToolWgt, &ToolWgt::showSearchWnd, this, &ChatViewItem::onShowSearchWnd);
    qRegisterMetaType<std::vector<Entity::ImTransfer>>("std::vector<Entity::ImTransfer>");
    connect(this, &ChatViewItem::sgDeleteLater, this, &ChatViewItem::deleteLater, Qt::QueuedConnection);
    //
    connect(this, &ChatViewItem::sgShowMessage, _pChatMainWgt, &ChatMainWgt::onShowMessage, Qt::QueuedConnection);
    connect(this, &ChatViewItem::sgLoadingMovie, this, &ChatViewItem::onShowLoading, Qt::QueuedConnection);
}

ChatViewItem::~ChatViewItem()
{
    if (nullptr != _pStatusWgt)
    {
        delete _pStatusWgt;
        _pStatusWgt = nullptr;
    }

    if (nullptr != _pChatMainWgt)
    {
        delete _pChatMainWgt;
        _pChatMainWgt = nullptr;
    }

    if (nullptr != _pGroupSidebar)
    {
        delete _pGroupSidebar;
        _pGroupSidebar = nullptr;
    }

    if (nullptr != _pToolWgt)
    {
        delete _pToolWgt;
        _pToolWgt = nullptr;
    }

    if (nullptr != _pInputWgt)
    {
        delete _pInputWgt;
        _pInputWgt = nullptr;
    }

    if(nullptr != _pSearchMainWgt)
    {
        delete _pSearchMainWgt;
        _pSearchMainWgt = nullptr;
    }
}

void ChatViewItem::setShareMessageState(bool flag)
{
    //
    _pChatMainWgt->setShareMessageState(flag);
    _pInputFrm->setVisible(!flag);
    _pShareMessageFrm->setVisible(flag);

    if(flag)
    {
        _pShareMessageFrm->setFixedHeight(_pInputFrm->height());
        _pShareMessageFrm->setSelectCount(0);
    }
}

/**
  * @函数名   initUi
  * @功能描述 初始化ui
  * @参数
  * @author   cc
  * @date     2018/09/25
  */
void ChatViewItem::initUi()
{
    setObjectName("ChatViewItem");
    _pLoading = makeLoadingLabel(true, {40, 40}, this);
    _pLoading->movie()->stop();
    // first
    _pShareMessageFrm = new ShareMessageFrm(this);
    // top status widget
    _pStatusWgt = new StatusWgt();
    _pStatusWgt->setObjectName("StatusWgt");
    //
    _pChatMainWgt = new ChatMainWgt(this);
    //
    _pGroupSidebar = new GroupChatSidebar(this);
    //
    _pInputWgt = new InputWgt(_pChatMainWgt, this);
    //
    splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(1);
    //
    _pToolWgt = new ToolWgt(_pInputWgt, this);
    _pToolWgt->setObjectName("ToolWgt");
    _leftLay = new QVBoxLayout;
    _leftLay->setMargin(0);
    _leftLay->setMargin(0);
    _leftLay->addWidget(_pLoading);
    _pLoading->setVisible(false);
    //
    _pInputFrm = new QFrame(this);
    auto *inpultLay = new QVBoxLayout(_pInputFrm);
    inpultLay->setContentsMargins(0, 0, 0, 0);
    inpultLay->setSpacing(0);
    inpultLay->addWidget(_pToolWgt);
    inpultLay->addWidget(_pInputWgt);
    //
    splitter->addWidget(_pChatMainWgt);
    splitter->addWidget(_pInputFrm);
    splitter->addWidget(_pShareMessageFrm);
    _leftLay->addWidget(splitter);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    //
    _pShareMessageFrm->setVisible(false);
    //
//    _pSearchMainWgt = new LocalSearchMainWgt();
    // 布局
    pMidLayout = new QHBoxLayout;
    pMidLayout->setMargin(0);
    pMidLayout->addLayout(_leftLay);
    auto *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(_pStatusWgt);
    layout->addLayout(pMidLayout);
    setLayout(layout);
    //
    sendBtnFrm = new QFrame(this);
    sendBtnFrm->setObjectName("sendBtnFrm");
    _sendBtn = new QPushButton(tr("发送"), this);
    _sendBtn->setObjectName("SendMessageBtn");
    _sendBtn->setToolTip(tr("发送消息 %1 ").arg(AppSetting::instance().getSendMessageKey().data()));
    _sendBtn->setFixedSize(100, 30);
    _sendBtnLay = new QHBoxLayout(sendBtnFrm);
    _sendBtnLay->setContentsMargins(0, 5, 15, 5);
    _sendBtnLay->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    _sendBtnLay->addWidget(_sendBtn);
    _leftLay->addWidget(sendBtnFrm);
    connect(_sendBtn, &QPushButton::clicked, _pInputWgt, &InputWgt::sendMessage);
}

/**
  * @函数名   conversionId()
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/17
  */
QString ChatViewItem::conversionId()
{
    if (_strConversionId.isEmpty())
        _strConversionId = QString::fromStdString(QTalk::utils::getMessageId());

    return _strConversionId;
}



/**
  * @函数名   showMessageSlot
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/09/20
  */
void ChatViewItem::showMessage(const Entity::ImMessageInfo &message, int jumType)
{
    if(_pStatusWgt != nullptr)
    {
        if(message.ChatType != Enum::ChatType::GroupChat && !message.SendJid.empty())
        {
            Entity::JID jid(message.SendJid);
            _pStatusWgt->showResource(jid.resources());
        }
    }

    if(message.Type == INT_MIN)
        return;

    StNetMessageResult info;
    info.msg_id = message.MsgId.data();
    info.msg_type = message.Type;
    info.type = message.ChatType ;
    info.direction = message.Direction;
    info.state = message.State;
    info.read_flag = message.ReadedTag;
    info.time = message.LastUpdateTime;
    info.from = message.From.data();
    info.body = message.Content.data();
    info.extend_info = message.ExtendedInfo.data();
    info.backup_info = message.BackupInfo.data();
    info.xmpp_id = message.XmppId.data();
    info.real_id = message.RealJid.data();
    info.user_name = message.UserName.data();
    info.user_head = message.HeadSrc.data();
    QTalk::analysisMessage(info);
    emit sgShowMessage(info, jumType);
}

/**
  * @函数名   showMessageSlot
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/09/20
  */
Entity::UID ChatViewItem::getPeerId() const
{
    return _uid;
}

void ChatViewItem::onRecvAddGroupMember(const std::string &memberId, const std::string &nick, int affiliation)
{
    if(_pGroupSidebar)
        _pGroupSidebar->updateGroupMember(memberId, nick, affiliation);
}

//
void ChatViewItem::onRecvRemoveGroupMember(const std::string &memberId)
{
    if(_pGroupSidebar)
        _pGroupSidebar->deleteMember(memberId);
}

//
void ChatViewItem::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
        emit g_pMainPanel->sgShortCutSwitchSession(e->key());

    QWidget::keyPressEvent(e);
}

//
void ChatViewItem::onShowSearchWnd()
{
    if(nullptr == _pSearchMainWgt)
    {
        _pSearchMainWgt = new LocalSearchMainWgt;
        _pSearchMainWgt->setStyleSheet(g_pMainPanel->_qss);
        _pSearchMainWgt->sgUpdateName(_name);
    }

    QPoint pos = this->geometry().topRight();
    pos = mapToGlobal(pos);
    _pSearchMainWgt->resetUi();
    _pSearchMainWgt->showAllMessage(_uid);
    _pSearchMainWgt->move({pos.x() - 360, pos.y()});
    _pSearchMainWgt->setVisible(true);
    QApplication::setActiveWindow(_pSearchMainWgt);
    _pSearchMainWgt->raise();
}

//
void ChatViewItem::switchSession(const QUInt8 &chatType, const QString &userName, const QTalk::Entity::UID &uid)
{
    _uid = uid;
    _name = userName;
    _chatType = (Enum::ChatType) chatType;

    //
    if(_pSearchMainWgt)
        emit _pSearchMainWgt->sgUpdateName(userName);

    _pStatusWgt->switchUser(chatType, uid, userName);
    _pToolWgt->switchSession(chatType);
//    _pInputWgt->clear();
    std::string userId = Entity::JID(_uid.usrId()).username();

    if (_chatType == Enum::System )
    {
//        _pToolWgt->setVisible(false);
        _pInputFrm->setVisible(false);
        _sendBtn->setVisible(false);
        sendBtnFrm->setVisible(false);
    }
    else
    {
        _pInputFrm->setVisible(true);
//        _pToolWgt->setVisible(true);
        sendBtnFrm->setVisible(AppSetting::instance().getShowSendMessageBtnFlag());
        _sendBtn->setVisible(AppSetting::instance().getShowSendMessageBtnFlag());
    }

    _pGroupSidebar->setVisible(_chatType == Enum::GroupChat);

    if (_chatType == Enum::GroupChat)
    {
        pMidLayout->addWidget(_pGroupSidebar);

        if(_pGroupSidebar )
            _pGroupSidebar->onUpdateGroupInfo(uid.qUsrId(), userName);
    }
    else if(pMidLayout->indexOf(_pGroupSidebar) != -1)
        pMidLayout->removeWidget(_pGroupSidebar);

//    if(_pGroupSidebar)
//        _pGroupSidebar->clearData();
//    _pChatMainWgt->clearData();
}

//
void ChatViewItem::onShowLoading(bool show)
{
    _pLoading->setVisible(show);

    if(show)
        _pLoading->movie()->start();
    else
        _pLoading->movie()->stop();
}

void ChatViewItem::freeView()
{
    _pChatMainWgt->freeView();
}
