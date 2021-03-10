#include "SessionFrm.h"
#include <QEvent>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QScrollBar>
#include <set>
#include <QJsonArray>
#include <QWindowStateChangeEvent>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QFileInfo>
#include <QPushButton>
#include "../CustomUi/HeadPhotoLab.h"
#include "../CustomUi/UCButton.h"
#include "../UICom/UIEntity.h"
#include "../Message/ChatMessage.h"
#include "../Platform/Platform.h"
#include "SessionitemDelegate.h"
#include "../QtUtil/Entity/JID.h"
#include "../QtUtil/Utils/Log.h"
#include "../QtUtil/Utils/utils.h"
#include "../Platform/dbPlatForm.h"
#include "../include/perfcounter.h"
#include "../CustomUi/QtMessageBox.h"
#include "../include/Line.h"

#define SYSTEM_XMPPID "SystemMessage"
#define SYSTEM_NAME "系统消息"

using namespace QTalk::Entity;

SessionFrm::SessionFrm(QWidget *parent) : QFrame(parent)
{
    initLayout();
    connects();
    _messageListener = new NavigationMsgListener(this);
    QtConcurrent::run(&NavigationMsgManager::getSessionData);
}

SessionFrm::~SessionFrm()
{
    if (_messageListener)
        delete _messageListener;
}

/**
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.18
  */
void SessionFrm::initLayout()
{
    this->setFocusPolicy(Qt::NoFocus);
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setMargin(0);
    mainLay->setSpacing(0);
    this->setMinimumWidth(260);
    // disconnect tab
    _pTcpDisconnect = new TcpDisconnect(this);
    mainLay->addWidget(_pTcpDisconnect);
    _pTcpDisconnect->setVisible(false);
    //
//    _pBackBtnFrm = new QFrame(this);
//    _pBackBtnFrm->setObjectName("SessionBackFrm");
//    _pBackBtnFrm->setFixedHeight(40);
//    auto *backBtnLay = new QHBoxLayout(_pBackBtnFrm);
//    backBtnLay->setContentsMargins(0, 0, 25, 0);
//    backBtnLay->setSpacing(0);
//    auto *backBtn = new QPushButton(this);
//    _pTitleLabel = new QLabel(tr("未读消息"), this);
//    _pTitleLabel->setObjectName("SessionLabel");
//    _pTitleLabel->setAlignment(Qt::AlignCenter);
//    backBtn->setObjectName("SessionBackBtn");
//    backBtn->setFixedSize(25, 25);
//    backBtnLay->addWidget(backBtn, 0);
//    backBtnLay->addWidget(_pTitleLabel, 1);
//    mainLay->addWidget(_pBackBtnFrm);
//    _pBackBtnFrm->setVisible(false);
    //
    _pSelectFrm = new QFrame(this);
//    _pSelectFrm->setObjectName("SessionBackFrm");
    _pSelectFrm->setFixedHeight(50);
    auto *selectLay = new QHBoxLayout(_pSelectFrm);
    selectLay->setContentsMargins(0, 0, 0, 0);
    selectLay->setSpacing(2);
    mainLay->addWidget(_pSelectFrm);
    auto *tabGroup = new UCButtonGroup(this);
    auto *allBtn = new UCButton(tr("全部"), this);
    auto *topBtn = new UCButton(tr("置顶"), this);
    auto *unReadBtn = new UCButton(tr("未读"), this);
    auto *unNoticeBtn = new UCButton(tr("免打扰"), this);
    //
    tabGroup->addButton(allBtn, EM_SELECT_ALL);
    tabGroup->addButton(topBtn, EM_SELECT_TOP);
    tabGroup->addButton(unReadBtn, EM_SELECT_UNREAD);
    tabGroup->addButton(unNoticeBtn, EM_SELECT_UNNOTICE);
    selectLay->addWidget(allBtn, 2);
    selectLay->addWidget(topBtn, 2);
    selectLay->addWidget(unReadBtn, 2);
    selectLay->addWidget(unNoticeBtn, 3);
    tabGroup->setCheck(EM_SELECT_ALL);
    connect(tabGroup, &UCButtonGroup::clicked, [this](int index)
    {
        sortAndFilterView(index);
    });
    //
    mainLay->addWidget(new Line(this));
    //
    _pStackWgt = new QStackedWidget(this);
    // session view
    _pSessionView = new QListView(this);
    _pStackWgt->addWidget(_pSessionView);
    _pSessionView->setObjectName("SessionView");
    _pSessionView->verticalScrollBar()->setVisible(false);
    _pSessionView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pSessionView->verticalScrollBar()->setSingleStep(15);
    _pItemDelegate = new SessionitemDelegate(_pSessionView);
    _pSrcModel = new QStandardItemModel;
    _pModel = new SessionSortModel();
    _pSessionView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pSrcModel->setColumnCount(1);
    _pModel->setDynamicSortFilter(true);
    _pModel->setSourceModel(_pSrcModel);
    _pSessionView->setModel(_pModel);
    _pSessionView->setItemDelegate(_pItemDelegate);
    _pSessionView->setDragEnabled(false);
    _pSessionScrollBar = new QScrollBar(Qt::Vertical, this);
    _pSessionScrollBar->setRange(0, 0);
    _pSessionScrollBar->setMinimumHeight(100);
    _pSessionScrollBar->setVisible(false);
    connect(_pSessionView->verticalScrollBar(), SIGNAL(valueChanged(int)), _pSessionScrollBar, SLOT(setValue(int)));
    connect(_pSessionScrollBar, SIGNAL(valueChanged(int)), _pSessionView->verticalScrollBar(), SLOT(setValue(int)));
    connect(_pSessionView->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), _pSessionScrollBar, SLOT(setRange(int, int)));
    _pSessionView->setFrameShape(QFrame::NoFrame);
    _pSessionView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pSessionView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLay->addWidget(_pStackWgt);
    _pSessionView->installEventFilter(this);
    _pContextMenu = new QMenu(_pSessionView);
    _pContextMenu->setAttribute(Qt::WA_TranslucentBackground, true);
    _showCardAct = new QAction(tr("资料卡片"), _pContextMenu);
    _closeSessionAct = new QAction(tr("移除会话"), _pContextMenu);
    _clearUnreadAct = new QAction(tr("一键清除未读"), _pContextMenu);
    _toTopOrCancelTopAct = new QAction(_pContextMenu);
    _noticeAct = new QAction(_pContextMenu);
    _starAct = new QAction(_pContextMenu);
    _blackAct = new QAction(_pContextMenu);
    _quitGroupAct = new QAction(tr("退出群聊"), _pContextMenu);
    _pContextMenu->addAction(_toTopOrCancelTopAct);
    _pContextMenu->addAction(_noticeAct);
    _pContextMenu->addSeparator();
    _pContextMenu->addAction(_starAct);
    _pContextMenu->addAction(_blackAct);
    _pContextMenu->addAction(_showCardAct);
    _pContextMenu->addSeparator();
    _pContextMenu->addAction(_clearUnreadAct);
    _pContextMenu->addAction(_closeSessionAct);
    _pContextMenu->addSeparator();
    _pContextMenu->addAction(_quitGroupAct);
}

void SessionFrm::connects()
{
    connect(_pSessionView, &QListView::pressed, this, &SessionFrm::onItemSelectChanged);
    connect(_showCardAct, &QAction::triggered, this, &SessionFrm::onShowCardAct);
    connect(_closeSessionAct, &QAction::triggered, this, &SessionFrm::onCloseSession);
    connect(_toTopOrCancelTopAct, &QAction::triggered, this, &SessionFrm::onToTopAct);
    connect(_noticeAct, &QAction::triggered, this, &SessionFrm::onUnNoticeAct);
    connect(_starAct, &QAction::triggered, this, &SessionFrm::onStarAct);
    connect(_blackAct, &QAction::triggered, this, &SessionFrm::onBlackAct);
    connect(_clearUnreadAct, &QAction::triggered, this, &SessionFrm::onClearUnreadAct);
    connect(_quitGroupAct, &QAction::triggered, this, &SessionFrm::onQuitGroupAct);
    //
    qRegisterMetaType<ReceiveSession>("ReceiveSession");
    qRegisterMetaType<QTalk::Entity::UID>("QTalk::Entity::UID");
    qRegisterMetaType<QTalk::StGroupInfo>("QTalk::StGroupInfo");
    qRegisterMetaType<StSessionInfo>("StSessionInfo");
    connect(this, &SessionFrm::sgReceiveSession, this, &SessionFrm::onReceiveSession);
    connect(this, SIGNAL(sgDownLoadHeadPhotosFinish()), this, SLOT(repaint()));
    connect(this, &SessionFrm::sgUpdateOnline, this, &SessionFrm::onUpdateOnline);
    connect(this, &SessionFrm::sgUpdateOnlineUsers, this, &SessionFrm::onUpdateOnlineUsers);
    connect(this, &SessionFrm::setDisconnectWgtVisible, _pTcpDisconnect, &TcpDisconnect::setVisible);
    connect(this, &SessionFrm::updateGroupInfoSignal, this, &SessionFrm::onUpdateGroupInfo);
    connect(this, &SessionFrm::updateReadedCountSignal, this, &SessionFrm::onUpdateReadedCount);
    connect(this, &SessionFrm::recvRevokeMessageSignal, this, &SessionFrm::recvRevikeMessage);
    connect(this, &SessionFrm::loadSession, this, &SessionFrm::onloadSessionData);
    connect(this, &SessionFrm::sgUserConfigChanged, this, &SessionFrm::onUserConfigChanged);
    connect(this, &SessionFrm::destoryGroupSignal, this, &SessionFrm::onDestroyGroup);
    connect(this, &SessionFrm::sgGotMState, this, &SessionFrm::onGotMState);
    connect(this, &SessionFrm::sgShortCutSwitchSession, this, &SessionFrm::onShortCutSwitchSession);
    connect(this, &SessionFrm::sgUpdateUserInfo, this, &SessionFrm::onUpdateUserInfo);
}

/**
 * @brief SessionFrm::sortAndFilterView
 * @param index
 */
void SessionFrm::sortAndFilterView(int index )
{
    static int _s_index = 0;

    if(index != -1)
        _s_index = index;

    _pModel->setFilterRole(_s_index);
    _pModel->sort(0);
}

/**
  * @函数名
  * @功能描述 处理
  * @参数
  * @author cc
  * @date 2018.9.20
  */
void SessionFrm::onReceiveSession(const ReceiveSession &mess, bool isSend)
{
    perf_counter_warning(100, "onReceiveSession:{0}", mess.xmppId.toStdString());
    QString userId = mess.xmppId;
    QString realJid = (mess.realJid.isNull() || mess.realJid.isEmpty()) ? mess.xmppId : mess.realJid;
    userId = userId.left(userId.indexOf("/"));
    UID uid(userId, realJid);
    QStandardItem *item = _sessionMap.value(uid);
    bool notCurItem = true;
    bool unNotice = _mapNotice.find(userId.toStdString()) != _mapNotice.end();
    QString sessionName;

    if (nullptr != item)
    {
        notCurItem = !(uid == _curUserId);

        if (mess.sendJid != _strSelfId.data())
        {
            if (notCurItem)
            {
                if (mess.chatType == QTalk::Enum::GroupChat)
                {
                    unsigned int atCount = item->data(ITEM_DATATYPE_ATCOUNT).toUInt();

                    if (mess.messageContent.contains("@all"))
                        atCount |= 0x000F;
                    else if (mess.messageContent.contains(
                                 QString("@%1").arg(PLAT.getSelfName().data())))
                        atCount |= 0x00F0;

                    item->setData(atCount, ITEM_DATATYPE_ATCOUNT);
                }
            }
            else
            {
                NavigationMsgManager::sendReadedMessage(mess.messageId.toStdString(),
                                                        realJid.toStdString(),
                                                        mess.chatType);
            }
        }

        //
        sessionName = item->data(ITEM_DATATYPE_USERNAME).toString();
    }
    else
    {
        QString headPath;
        QTalk::Entity::JID jid(userId.toStdString());
        bool noLocalHead = true;

        if (mess.chatType == QTalk::Enum::GroupChat)
        {
            std::shared_ptr<QTalk::Entity::ImGroupInfo> groupInfo =
                DB_PLAT.getGroupInfo(userId.toStdString(), true);

            if (groupInfo)
            {
                headPath = QTalk::GetHeadPathByUrl(groupInfo->HeaderSrc).data();
                QFileInfo headfile(headPath);
                noLocalHead = !headfile.isFile() || !headfile.exists();

                if(noLocalHead)
                {
                    QtConcurrent::run([groupInfo]()
                    {
                        NavigationMsgManager::downloadGroupHeads({groupInfo->HeaderSrc});
                    });
                }
            }
        }
        else
        {
            auto userInfo = DB_PLAT.getUserInfo(userId.toStdString(), true);

            if (userInfo)
            {
                if (!userInfo->HeaderSrc.empty())
                {
                    headPath = QTalk::GetHeadPathByUrl(userInfo->HeaderSrc).data();
                    QFileInfo headfile(headPath);
                    noLocalHead = !headfile.isFile() || !headfile.exists();
                }

                if(noLocalHead)
                {
                    QtConcurrent::run([userId]()
                    {
                        NavigationMsgManager::downloadUserHeads({userId.toStdString()});
                    });
                }
            }
        }

        item = new QStandardItem;
        item->setData(mess.chatType, ITEM_DATATYPE_CHATTYPE);
        item->setData(userId, ITEM_DATATYPE_USERID);
        item->setData(realJid, ITEM_DATATYPE_REALJID);
        item->setData(_mapStick.find(uid.toStdString()) != _mapStick.end(), ITEM_DATATYPE_ISTOP);
        item->setData(unNotice, ITEM_DATATYPE_UNNOTICE);
        //
        std::string id = jid.basename();

        if (SYSTEM_XMPPID == jid.username())
        {
            sessionName = tr(SYSTEM_NAME);
            item->setData(":/UINavigationPlug/image1/system.png", ITEM_DATATYPE_HEADPATH);
        }
        else
        {
            sessionName = getUserName(id, mess.chatType == QTalk::Enum::GroupChat);
            item->setData(headPath, ITEM_DATATYPE_HEADPATH);
            item->setToolTip(sessionName);
        }

        item->setData(sessionName, ITEM_DATATYPE_USERNAME);
        _sessionMap.insert(uid, item);
        _pSrcModel->appendRow(item);
    }

    item->setData(mess.messageId, ITEM_DATATYPE_LAST_MESSAGE_ID);
    item->setData(mess.messageRecvTime, ITEM_DATATYPE_LASTTIME);

    if (!isSend && notCurItem)
    {
        int count = item->data(ITEM_DATATYPE_UNREADCOUNT).toInt();
        item->setData(++count, ITEM_DATATYPE_UNREADCOUNT);

        if (count > 0 && !unNotice)
            emit sgShowUnreadMessage(mess.chatType, uid, sessionName, mess.messageRecvTime, count);

        if (!unNotice)
            emit updateTotalUnreadCount(++_totalUnReadCount);
    }

    QString userName;

    if (mess.chatType == QTalk::Enum::GroupChat)
        userName = QString::fromStdString(QTalk::getUserName(mess.sendJid.toStdString()));

    if (mess.messtype == QTalk::Entity::MessageTypeShock && _strSelfId.data() != mess.sendJid)
        item->setData(true, ITEM_DATATYPE_QQQ);

    item->setData(GenerateContent(mess.messageContent, mess.chatType, mess.messtype, userName),
                  ITEM_DATATYPE_MESSAGECONTENT);
    // sort
    _pSessionView->update();
    sortAndFilterView();
    emit sgUpdateOnlineUsers(userId);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.27
  */
void SessionFrm::onloadSessionData()
{
    //
    QMutexLocker locker(&_mutex);
    _totalUnReadCount = 0;
    std::set<std::string> withoutHeadUsers, withoutGroupHeadGroups;

    if (pSessions)
    {
        {
            auto it = _mapStick.begin();

            for (; it != _mapStick.end(); it++)
            {
                const std::string &xmppId = it->first;
                UID uid(xmppId);
                auto itFind = std::find_if(pSessions->begin(), pSessions->end(),
                                           [uid](const std::shared_ptr<QTalk::Entity::ImSessionInfo> &session)
                {
                    return session->XmppId == uid.usrId();
                });

                if (itFind == pSessions->end())
                {
                    QJsonParseError jsoErr{};
                    QJsonDocument jsoDoc = QJsonDocument::fromJson(it->second.data(), &jsoErr);

                    if (jsoErr.error == QJsonParseError::NoError)
                    {
                        QJsonObject obj = jsoDoc.object();
                        std::shared_ptr<QTalk::Entity::ImSessionInfo> session(new QTalk::Entity::ImSessionInfo);
                        session->XmppId = uid.usrId();
                        session->RealJid = uid.realId();
                        session->ChatType = obj.value("chatType").toInt(); //
                        session->LastUpdateTime = 0;
                        session->UnreadCount = 0;
                        pSessions->push_back(session);
                    }
                }
            }
        }

        //
        for (const auto &sessionInfo : *pSessions)
        {
            std::string xmppId = sessionInfo->XmppId;
            std::string realJid = sessionInfo->RealJid;
            QString headPath;
            bool noLocalHead = true;

            if (sessionInfo->ChatType == QTalk::Enum::GroupChat)
            {
                std::shared_ptr<QTalk::Entity::ImGroupInfo> groupInfo =
                    DB_PLAT.getGroupInfo(sessionInfo->XmppId);

                if (groupInfo)
                {
                    headPath = QTalk::GetHeadPathByUrl(groupInfo->HeaderSrc).data();
                    QFileInfo headfile(headPath);
                    noLocalHead = !headfile.isFile() || !headfile.exists();

                    if (noLocalHead)
                        withoutGroupHeadGroups.insert(groupInfo->HeaderSrc);
                }
            }
            else
            {
                auto userInfo = DB_PLAT.getUserInfo(sessionInfo->XmppId);

                if (userInfo)
                {
                    if (!userInfo->HeaderSrc.empty())
                    {
                        headPath = QTalk::GetHeadPathByUrl(userInfo->HeaderSrc).data();
                        QFileInfo headfile(headPath);
                        noLocalHead = !headfile.isFile() || !headfile.exists();
                    }

                    if (noLocalHead)
                        withoutHeadUsers.insert(sessionInfo->XmppId);
                }
            }

            QStandardItem *item = nullptr;
            UID uid(xmppId, realJid);
            bool unNotice = _mapNotice.find(sessionInfo->XmppId) != _mapNotice.end();

            if (_sessionMap.contains(uid))
                item = _sessionMap.value(uid);
            else
            {
                item = new QStandardItem;

                if (sessionInfo->ChatType == QTalk::Enum::GroupChat)
                    item->setData(0, ITEM_DATATYPE_ATCOUNT);
            }

            item->setData(sessionInfo->UnreadCount, ITEM_DATATYPE_UNREADCOUNT);

            if (!unNotice)
                _totalUnReadCount += sessionInfo->UnreadCount;

            item->setData(sessionInfo->ChatType, ITEM_DATATYPE_CHATTYPE);
            item->setData(QString::fromStdString(sessionInfo->XmppId), ITEM_DATATYPE_USERID);
            item->setData(sessionInfo->LastUpdateTime, ITEM_DATATYPE_LASTTIME);
            item->setData(QString::fromStdString(realJid), ITEM_DATATYPE_REALJID);
            item->setData(QString::fromStdString(sessionInfo->LastMessageId), ITEM_DATATYPE_LAST_MESSAGE_ID);
            QString sessionName;

            if (SYSTEM_XMPPID == QTalk::Entity::JID(sessionInfo->XmppId).username())
            {
                sessionName = tr(SYSTEM_NAME);
                item->setData(":/UINavigationPlug/image1/system.png", ITEM_DATATYPE_HEADPATH);
            }
            else
            {
                sessionName = getUserName(xmppId, sessionInfo->ChatType == QTalk::Enum::GroupChat);
                item->setData(headPath, ITEM_DATATYPE_HEADPATH);
            }

            item->setToolTip(sessionName);
            item->setData(sessionName, ITEM_DATATYPE_USERNAME);

            if (sessionInfo->UnreadCount > 0 && !unNotice)
            {
                emit sgShowUnreadMessage(sessionInfo->ChatType,
                                         uid, sessionName, sessionInfo->LastUpdateTime, sessionInfo->UnreadCount);
            }

            // content
            {
                QString userName;

                if (sessionInfo->ChatType == QTalk::Enum::GroupChat)
                    userName = QString::fromStdString(QTalk::getUserName(sessionInfo->UserId));

                item->setData(GenerateContent(QString::fromStdString(sessionInfo->Content), sessionInfo->ChatType,
                                              sessionInfo->MessType, userName),
                              ITEM_DATATYPE_MESSAGECONTENT);
            }
            //is top
            item->setData(_mapStick.find(uid.toStdString()) != _mapStick.end(), ITEM_DATATYPE_ISTOP);
            item->setData(unNotice, ITEM_DATATYPE_UNNOTICE);

            //
            if (!_sessionMap.contains(uid))
            {
                _pSrcModel->appendRow(item);
                _sessionMap.insert(uid, item);
            }
        }
    }

    //
    if(!withoutHeadUsers.empty())
        QtConcurrent::run(&NavigationMsgManager::downloadUserHeads, withoutHeadUsers);

    if(!withoutGroupHeadGroups.empty())
        QtConcurrent::run(&NavigationMsgManager::downloadGroupHeads, withoutGroupHeadGroups);

    _strSelfId = PLAT.getSelfXmppId();
    // sort
    sortAndFilterView();
    //
    emit sgUpdateOnline();
    emit updateTotalUnreadCount(_totalUnReadCount);
}

/**
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.18
  */
void SessionFrm::onItemSelectChanged(const QModelIndex &index)
{
    //_headPhotoLab
    if (!index.isValid())
        return;

    auto userId = index.data(ITEM_DATATYPE_USERID).toString();
    auto relJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(userId, relJid);
    //
    StSessionInfo sessionInfo;
    sessionInfo.chatType = static_cast<QUInt8>(index.data(ITEM_DATATYPE_CHATTYPE).toInt());
    sessionInfo.userId = userId;
    sessionInfo.realJid = relJid;
    sessionInfo.userName = index.data(ITEM_DATATYPE_USERNAME).toString();
    sessionInfo.headPhoto = index.data(ITEM_DATATYPE_HEADPATH).toString();
    {
        _curUserId = UID(sessionInfo.userId, sessionInfo.realJid);
        _pItemDelegate->setProperty("CURRENT_ID", userId);
//        _pModel->setProperty("CURRENT_ID", userId);
    }

    if (sessionInfo.userName.isEmpty())
    {
        sessionInfo.userName = QTalk::getGroupName(sessionInfo.userId.toStdString()).data();
        _sessionMap[uid]->setData(sessionInfo.userName, ITEM_DATATYPE_USERNAME);
    }

    // draft
    _sessionMap[uid]->setData("", ITEM_DATATYPE_DRAFT);
    {
        _sessionMap[uid]->setData(false, ITEM_DATATYPE_QQQ);
    }
    //if (sessionInfo.chatType != QTalk::Enum::GroupChat) {
    unsigned int count = _sessionMap[uid]->data(ITEM_DATATYPE_UNREADCOUNT).toUInt();
    _sessionMap[uid]->setData(0, ITEM_DATATYPE_UNREADCOUNT);
    bool unNotice = _mapNotice.find(sessionInfo.userId.toStdString()) != _mapNotice.end();

    if (!unNotice)
    {
        _totalUnReadCount -= count;
        emit updateTotalUnreadCount(_totalUnReadCount);
        emit sgShowUnreadMessage(0, uid, "", 0, 0);
    }

    _sessionMap[uid]->setData(0, ITEM_DATATYPE_ATCOUNT);
    QtConcurrent::run([sessionInfo, index, count]()
    {
        if (sessionInfo.chatType != QTalk::Enum::GroupChat)
            NavigationMsgManager::sendGetUserStatus(sessionInfo.realJid.toStdString());

        if (count > 0)
        {
            //if(sessionInfo.chatType == QTalk::Enum::TwoPersonChat)
            {
                // 发送消息已读
                QString messageId = index.data(ITEM_DATATYPE_LAST_MESSAGE_ID).toString();
                NavigationMsgManager::sendReadedMessage(messageId.toStdString(), sessionInfo.realJid.toStdString(),
                                                        sessionInfo.chatType);
            }
        }
    });
    emit sgSessionInfo(sessionInfo);
    sortAndFilterView();
}
/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.29
  */
void SessionFrm::onDownLoadHeadPhotosFinish()
{
    emit sgDownLoadHeadPhotosFinish();
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.30
  */
void SessionFrm::onDownLoadGroupHeadPhotosFinish()
{
    emit sgDownLoadHeadPhotosFinish();
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.12
  */
void SessionFrm::onUpdateOnline()
{
    if (!PLAT.isMainThread())
    {
        emit sgUpdateOnline();
        return;
    }

    perf_counter_warning(200, "onUpdateOnline");

    for (const UID &uid : _sessionMap.keys())
        setUserStatus(uid);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.15
  */
void SessionFrm::onUpdateOnlineUsers(const QString &id)
{
    if (!PLAT.isMainThread())
    {
        emit sgUpdateOnlineUsers(id);
        return;
    }

    setUserStatus(UID(id), true);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.11.08
  */
void SessionFrm::onNewSession(const StSessionInfo &info)
{
    ReceiveSession mess;
    mess.xmppId = info.userId;

    if (DB_PLAT.isHotLine(mess.xmppId.toStdString()))
        mess.chatType = QTalk::Enum::Consult;
    else
        mess.chatType = (QTalk::Enum::ChatType)info.chatType;

    mess.realJid = info.realJid.isEmpty() ? info.userId : info.realJid;
    mess.headSrc = info.headPhoto;
    // 临时处理
    UID uid(mess.xmppId, mess.realJid);

    if (_sessionMap.contains(uid))
    {
        QStandardItem *item = _sessionMap[uid];
        mess.messageContent = item->data(ITEM_DATATYPE_MESSAGECONTENT).toString();
        mess.messageRecvTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        mess.messageId = item->data(ITEM_DATATYPE_LAST_MESSAGE_ID).toString();
    }
    else
    {
        std::string messagedId = QTalk::utils::getMessageId();
        mess.messageContent = "";
        mess.messageRecvTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        mess.messageId = QString::fromStdString(messagedId);
        //
        QTalk::Entity::ImMessageInfo msgInfo;
        msgInfo.MsgId = messagedId;
        msgInfo.XmppId = uid.usrId();
        msgInfo.RealJid = uid.realId();
        msgInfo.ChatType = info.chatType;
        msgInfo.Platform = 0;
        msgInfo.From = _strSelfId;
        msgInfo.To = uid.usrId();
        msgInfo.Content = "";
        msgInfo.Type = QTalk::Entity::MessageTypeEmpty;
        msgInfo.State = 1;
        msgInfo.Direction = QTalk::Entity::MessageDirectionSent;
        msgInfo.LastUpdateTime = mess.messageRecvTime;
        msgInfo.SendJid = _strSelfId;
        NavigationMsgManager::addEmptyMessage(msgInfo);
    }

    //
    onReceiveSession(mess, true);
    //
    QModelIndex index = _sessionMap[uid]->index();
    _pSessionView->setCurrentIndex(_pModel->mapFromSource(index));
    onItemSelectChanged(index);
}
///**
//  * @函数名
//  * @功能描述
//  * @参数
//  * @author cc
//  * @date 2018.9.30
//  */
//void SessionFrm::GenerateHeadPhotoName(QString &photosrc)
//{
//    std::string tmpPath = QTalk::GetFileNameByUrl(photosrc.toStdString());
//    photosrc = tmpPath.data();
//    Q_ASSERT(!photosrc.startsWith("http"));
//}
/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.15
  */
void SessionFrm::setUserStatus(const QTalk::Entity::UID &uid, bool)
{
    QStandardItem *item = _sessionMap.value(uid);

    if (nullptr != item)
    {
        auto chatType = item->data(ITEM_DATATYPE_CHATTYPE).toInt();

        if (chatType == QTalk::Enum::GroupChat)
        {
            item->setData(true, ITEM_DATATYPE_ISONLINE);
            return;
        }

        {
            QTalk::Entity::JID jid(uid.usrId());

            if (jid.username() == SYSTEM_XMPPID)
            {
                item->setData(true, ITEM_DATATYPE_ISONLINE);
                return;
            }
        }

        if (uid.usrId() == _strSelfId)
        {
            item->setData(true, ITEM_DATATYPE_ISONLINE);
            return;
        }

        bool isOnline = PLAT.isOnline(uid.realId());
        item->setData(isOnline, ITEM_DATATYPE_ISONLINE);
    }
}

/**
 * @brief SessionFrm::eventFilter
 * @param o
 * @param e
 * @return
 */
bool SessionFrm::eventFilter(QObject *o, QEvent *e)
{
    if (o == _pSessionView)
    {
        if (e->type() == QEvent::ContextMenu)
        {
            QUInt8 chatType = static_cast<QUInt8>(_pSessionView->currentIndex().data(ITEM_DATATYPE_CHATTYPE).toInt());
            std::string id = _pSessionView->currentIndex().data(ITEM_DATATYPE_USERID).toString().toStdString();
            bool isTop = _pSessionView->currentIndex().data(ITEM_DATATYPE_ISTOP).toBool();
            bool unNotice = _pSessionView->currentIndex().data(ITEM_DATATYPE_UNNOTICE).toBool();
            _toTopOrCancelTopAct->setText(isTop ? tr("取消置顶") : tr("会话置顶"));
            _noticeAct->setText(unNotice ? tr("取消免打扰") : tr("消息免打扰"));
            _quitGroupAct->setVisible(chatType == QTalk::Enum::GroupChat);

            if (chatType == QTalk::Enum::GroupChat)
            {
                _starAct->setVisible(false);
                _blackAct->setVisible(false);
            }
            else
            {
                //
                bool isStar = std::find(_arSatr.begin(), _arSatr.end(), id) != _arSatr.end();
                bool isBlack = std::find(_arBlackList.begin(), _arBlackList.end(), id) != _arBlackList.end();
                _starAct->setText(isStar ? tr("取消星标") : tr("星标联系人"));
                _blackAct->setText(isBlack ? tr("移出黑名单") : tr("加入黑名单"));
            }

            _pContextMenu->exec(QCursor::pos());
            _starAct->setVisible(true);
            _blackAct->setVisible(true);
        }
        else if (e->type() == QEvent::MouseMove)
            _pSessionView->update();
        else if (e->type() == QEvent::Resize)
        {
            _pSessionScrollBar->setGeometry(_pSessionView->width() - 15, 55,
                                            17, _pSessionView->height() - 10);
        }
    }

    return QFrame::eventFilter(o, e);
}

/**
 * @brief SessionFrm::event
 * @param e
 * @return
 */
bool SessionFrm::event(QEvent *e)
{
    if (e->type() == QEvent::Enter)
    {
        if (_pSessionScrollBar->maximum() > 0 && !_pSessionScrollBar->isVisible())
            _pSessionScrollBar->setVisible(true);
    }
    else if (e->type() == QEvent::Leave)
    {
        if (_pSessionScrollBar->isVisible())
            _pSessionScrollBar->setVisible(false);
    }
    else if (e->type() == QEvent::WindowActivate || e->type() == QEvent::Show)
    {
        if (_curUserId.isEmpty())
            onAppActive();
    }

    return QFrame::event(e);
}

/**
 * @brief SessionFrm::onUpdateGroupInfo
 * @param gInfo
 */
void SessionFrm::onUpdateGroupInfo(const QTalk::StGroupInfo &gInfo)
{
    if (!PLAT.isMainThread())
    {
        emit updateGroupInfoSignal(gInfo);
        return;
    }

    UID uid(gInfo.groupId);

    if (_sessionMap.contains(uid))
    {
        QStandardItem *item = _sessionMap[uid];
        auto info = DB_PLAT.getGroupInfo(gInfo.groupId, true);

        if (nullptr == info)
            return;

        QString name = QString::fromStdString(info->Name);

        if (!name.isEmpty())
        {
            item->setData(name, ITEM_DATATYPE_USERNAME);
            item->setToolTip(name);
            item->setData(QTalk::GetHeadPathByUrl(info->HeaderSrc).data(), ITEM_DATATYPE_HEADPATH);
        }
    }
}

/**
 * @brief SessionFrm::GenerateContent
 * @param content
 * @param chatType
 * @param msgType
 * @param userName
 * @return
 */
QString SessionFrm::GenerateContent(const QString &content, const QUInt8 &chatType, const int &msgType,
                                    const QString &userName)
{
    QString ret = "";

    if (chatType == QTalk::Enum::GroupChat && !userName.isEmpty() && msgType != QTalk::Entity::MessageTypeGroupNotify)
        ret += userName + ":";

    switch (msgType)
    {
        case QTalk::Entity::MessageTypePhoto:
            ret += tr("[图片]");
            break;

        case QTalk::Entity::MessageTypeImageNew:
            ret += tr("[表情]");
            break;

        case QTalk::Entity::MessageTypeFile:
            ret += tr("[文件]");
            break;

        case QTalk::Entity::MessageTypeCommonTrdInfo:
        case QTalk::Entity::MessageTypeCommonTrdInfoV2:
            ret += tr("[链接卡片]");
            break;

        case QTalk::Entity::MessageTypeSourceCode:
            ret += tr("[代码块]");
            break;

        case QTalk::Entity::MessageTypeSmallVideo:
            ret += tr("[视频]");
            break;

        case QTalk::Entity::WebRTC_MsgType_VideoCall:
            ret += tr("[实时视频]");
            break;

        case QTalk::Entity::WebRTC_MsgType_AudioCall:
            ret += tr("[实时音频]");
            break;

        case QTalk::Entity::WebRTC_MsgType_Video_Group:
            ret += tr("[群组视频]");
            break;

        case QTalk::Entity::MessageTypeVoice:
            ret += tr("[语音]");
            break;

        case QTalk::Entity::MessageTypeProduct:
        case QTalk::Entity::MessageTypeNote:
            ret += tr("[产品详情]");
            break;

        case QTalk::Entity::MessageTypeSystem:
            ret += tr("[系统消息]");
            break;

        case QTalk::Entity::MessageTypeNotice:
            ret += tr("[公告消息]");
            break;

        case QTalk::Entity::MessageTypeGrabMenuVcard:
        case QTalk::Entity::MessageTypeGrabMenuResult:
            ret += tr("[抢单消息]");
            break;

        case 65537:
        case 65538:
            ret += tr("[热线提示信息]");
            break;

        default:
            {
                QString tmpContent = content.split("\n").first();
                QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");
                regExp.setMinimal(true);
                int pos = 0;

                while ((pos = regExp.indexIn(tmpContent)) != -1)
                {
                    QString item = regExp.cap(0); // 符合条件的整个字符串
                    QString type = regExp.cap(1); // 多媒体类型

                    if ("url" == type)
                        tmpContent.replace(pos, item.size(), tr("[链接]"));
                    else if ("image" == type)
                        tmpContent.replace(pos, item.size(), tr("[图片]"));
                    else if ("emoticon" == type)
                        tmpContent.replace(pos, item.size(), tr("[表情]"));
                    else
                        tmpContent.replace(pos, item.size(), tr("[未知]"));
                }

                ret += tmpContent;
                break;
            }
    }

    return ret;
}

/**
 * @brief SessionFrm::onUpdateReadedCount
 * @param uid
 * @param count
 */
void SessionFrm::onUpdateReadedCount(const QTalk::Entity::UID &uid, const int &count)
{
    if (!PLAT.isMainThread())
    {
        emit updateReadedCountSignal(uid, count);
        return;
    }

    if (_curUserId == uid)
        return;

    if (_sessionMap.contains(uid) && nullptr != _sessionMap[uid])
    {
        int chatType = _sessionMap[uid]->data(ITEM_DATATYPE_CHATTYPE).toInt();
        QString name = _sessionMap[uid]->data(ITEM_DATATYPE_USERNAME).toString();
        QInt64 lasTime = _sessionMap[uid]->data(ITEM_DATATYPE_LASTTIME).toLongLong();
        int tmpC = _sessionMap[uid]->data(ITEM_DATATYPE_UNREADCOUNT).toUInt();
        int minc = qMin(tmpC, count);
        _sessionMap[uid]->setData(tmpC - minc, ITEM_DATATYPE_UNREADCOUNT);
        emit sgShowUnreadMessage(chatType, uid, name, lasTime, tmpC - minc);
        bool unNotice = _mapNotice.find(uid.usrId()) != _mapNotice.end();

        if (!unNotice)
        {
            _totalUnReadCount -= minc;
            emit updateTotalUnreadCount(_totalUnReadCount);
        }

        //         if (chatType == QTalk::Enum::GroupChat) {
        //             int atCount = DB_PLAT.getAtCount(uid.usrId());
        //             _sessionMap[uid]->setData(atCount, ITEM_DATATYPE_ATCOUNT);
        //         }
        //
        //debug_log("ui更新群未读数 id:{0} 未读数{1}", id.toStdString(), unreadCount);
    }
}

/**
 * @brief SessionFrm::recvRevikeMessage
 * @param uid
 * @param fromId
 */
void SessionFrm::recvRevikeMessage(const QTalk::Entity::UID &uid, const QString &fromId)
{
    if (!PLAT.isMainThread())
    {
        emit recvRevokeMessageSignal(uid, fromId);
        return;
    }

    if (_sessionMap.contains(uid) && nullptr != _sessionMap[uid])
    {
        QString userName = tr("你");

        if (fromId.toStdString() != _strSelfId)
        {
            std::shared_ptr<QTalk::Entity::ImUserInfo> info =
                DB_PLAT.getUserInfo(fromId.toStdString());

            if (info && !info->Name.empty())
                userName = QString::fromStdString(QTalk::getUserName(info));
            else
                userName = "";
        }

        QInt64 time = QDateTime::currentDateTime().toMSecsSinceEpoch();
        _sessionMap[uid]->setData(tr("%1撤回了一条消息").arg(userName), ITEM_DATATYPE_MESSAGECONTENT);
        _sessionMap[uid]->setData(time, ITEM_DATATYPE_LASTTIME);
        //        _sessionMap[uid]->setData(GenerateTimeText(time), ITEM_DATATYPE_LASTSTRTIME);
        // sort
        _pModel->sort(0);
    }
}

/**
 * @brief SessionFrm::onShowCardAct
 */
void SessionFrm::onShowCardAct(bool)
{
    QModelIndex index = _pSessionView->currentIndex();

    if (index.isValid())
    {
        QString xmppId = index.data(ITEM_DATATYPE_USERID).toString();

        if (index.data(ITEM_DATATYPE_CHATTYPE).toInt() == QTalk::Enum::GroupChat)
            emit showGroupCardSignal(xmppId);
        else
            emit showUserCardSignal(xmppId);
    }
}

/**
 * @brief SessionFrm::onCloseSession
 */
void SessionFrm::onCloseSession(bool)
{
    QModelIndex index = _pSessionView->currentIndex();
    QString peerId = index.data(ITEM_DATATYPE_USERID).toString();
    QString realJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(peerId, realJid);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        emit removeSession(uid.qUsrId());
        _pSrcModel->removeRow((*itFind)->row());
        _sessionMap.remove(uid);
        NavigationMsgManager::removeSession(uid.usrId());
    }
}

/**
 * @brief SessionFrm::onToTopAct
 */
void SessionFrm::onToTopAct(bool)
{
    QModelIndex index = _pSessionView->currentIndex();
    int chatType = index.data(ITEM_DATATYPE_CHATTYPE).toInt();
    QString peerId = index.data(ITEM_DATATYPE_USERID).toString();
    bool isTop = index.data(ITEM_DATATYPE_ISTOP).toBool();
    QString val = QString("{\"topType\":%1,\"chatType\":%2}").arg(isTop ? 0 : 1).arg(chatType);
    QString realJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(peerId, realJid);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        QtConcurrent::run(NavigationMsgManager::setUserSetting, isTop,
                          std::string("kStickJidDic"), uid.toStdString(), val.toStdString());
    }
}

/**
 * @brief SessionFrm::updateUserConfig
 */
void SessionFrm::updateUserConfig()
{
    QMutexLocker locker(&_mutex);
    auto it = _sessionMap.begin();

    for (; it != _sessionMap.end(); it++)
    {
        (*it)->setData(_mapStick.find(it.key().toStdString()) != _mapStick.end(), ITEM_DATATYPE_ISTOP);
        (*it)->setData(_mapNotice.find(it.key().toStdString()) != _mapNotice.end(), ITEM_DATATYPE_UNNOTICE);
    }

    sortAndFilterView();
}

/**
 * 销毁群结果处理
 * @param groupId
 */
void SessionFrm::onDestroyGroup(const QString &groupId)
{
    if (!PLAT.isMainThread())
    {
        emit destoryGroupSignal(groupId);
        return;
    }

    UID uid(groupId);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        // 销毁群之后更新未读数
        unsigned int count = (*itFind)->data(ITEM_DATATYPE_UNREADCOUNT).toUInt();
        _totalUnReadCount -= count;
        emit updateTotalUnreadCount(_totalUnReadCount);
        //
        emit removeSession(uid.qUsrId());
        _pSrcModel->removeRow((*itFind)->row());
        _sessionMap.remove(uid);
        NavigationMsgManager::removeSession(uid.usrId());
        //        QModelIndex index = _pModel->index(0, 0);
        //        _pSessionView->setCurrentIndex(index);
        //        onItemSelectChanged(_pModel->mapToSource(index));
    }
}

/**
 * 消息面打扰
 */
void SessionFrm::onUnNoticeAct(bool)
{
    QModelIndex index = _pSessionView->currentIndex();
    QString peerId = index.data(ITEM_DATATYPE_USERID).toString();
    bool unNotice = index.data(ITEM_DATATYPE_UNNOTICE).toBool();
    QString val = QString::number(unNotice ? 0 : 1);
    QString realJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(peerId, realJid);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        QtConcurrent::run(NavigationMsgManager::setUserSetting, unNotice,
                          std::string("kNoticeStickJidDic"), peerId.toStdString(), val.toStdString());
    }
}

/**
 * 星标联系人
 */
void SessionFrm::onStarAct(bool)
{
    emit sgOperator(tr("星标联系人"));
    QModelIndex index = _pSessionView->currentIndex();
    QString peerId = index.data(ITEM_DATATYPE_USERID).toString();
    bool isStar = std::find(_arSatr.begin(), _arSatr.end(), peerId.toStdString()) != _arSatr.end();
    QString val = QString::number(isStar ? 0 : 1);
    QString realJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(peerId, realJid);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        QtConcurrent::run(NavigationMsgManager::setUserSetting,
                          isStar,
                          std::string("kStarContact"),
                          peerId.toStdString(),
                          val.toStdString());
    }
}

/**
 * 加入黑名单
 */
void SessionFrm::onBlackAct(bool)
{
    emit sgOperator(tr("加入黑名单"));
    QModelIndex index = _pSessionView->currentIndex();
    QString peerId = index.data(ITEM_DATATYPE_USERID).toString();
    bool isBlack = std::find(_arBlackList.begin(), _arBlackList.end(), peerId.toStdString()) != _arBlackList.end();
    QString val = QString::number(isBlack ? 0 : 1);
    QString realJid = index.data(ITEM_DATATYPE_REALJID).toString();
    UID uid(peerId, realJid);
    auto itFind = _sessionMap.find(uid);

    if (itFind != _sessionMap.end() && nullptr != *itFind)
    {
        QtConcurrent::run(&NavigationMsgManager::setUserSetting,
                          isBlack,
                          std::string("kBlackList"),
                          peerId.toStdString(),
                          val.toStdString());
    }
}

/**
 *
 */
void SessionFrm::jumpToNewMessage()
{
    if (!PLAT.isMainThread())
        return;

    int row = _pModel->rowCount();

    for (int i = _jumpIndex; i < row; i++)
    {
        QModelIndex index = _pModel->index(i, 0);
        QModelIndex srcIndex = _pModel->mapToSource(index);
        int unreadCout = srcIndex.data(ITEM_DATATYPE_UNREADCOUNT).toInt();

        if (unreadCout > 0)
        {
            _pSessionView->scrollTo(index, QAbstractItemView::PositionAtTop);
            _jumpIndex = i + 1;
            return;
        }
    }

    if (_jumpIndex > 0)
    {
        _jumpIndex = 0;
        jumpToNewMessage();
    }
}

/**
 * @brief SessionFrm::getUserName
 * @param id
 * @param isGroup
 * @return
 */
QString SessionFrm::getUserName(const std::string &id, bool isGroup)
{
    std::string userId = QTalk::Entity::JID(id).username();

    if (SYSTEM_XMPPID == userId)
        return tr(SYSTEM_NAME);

    if (!id.empty())
    {
        if (isGroup)
            return QString::fromStdString(QTalk::getGroupName(id));
        else
            return QString::fromStdString(QTalk::getUserName(id));
    }

    return userId.data();
}

/**
 *
 * @param key
 */
void SessionFrm::onShortCutSwitchSession(int key)
{
    int index = _pSessionView->currentIndex().row();

    switch (key)
    {
        case Qt::Key_Up:
            {
                index--;

                if (index <= 0)
                    index = 0;

                break;
            }

        case Qt::Key_Down:
            {
                index++;

                if (index >= _pModel->rowCount())
                    index = _pModel->rowCount();

                break;
            }

        default:
            break;
    }

    QModelIndex modelIndex = _pModel->index(index, 0);
    QModelIndex srcIndex = _pModel->mapToSource(modelIndex);
    _pSessionView->setCurrentIndex(modelIndex);
    onItemSelectChanged(srcIndex);
}

/**
 * @brief SessionFrm::onUpdateUserInfo
 * @param users
 */
void SessionFrm::onUpdateUserInfo(const QVector<QString> &users)
{
    for(auto &u : users)
    {
        auto *item = _sessionMap.value(UID(u));

        if (item)
        {
            auto info = DB_PLAT.getUserInfo(u.toStdString(), true);

            if (info)
            {
                item->setData(QTalk::getUserName(u.toStdString()).data(),
                              ITEM_DATATYPE_USERNAME);
                item->setData(QTalk::GetHeadPathByUrl(info->HeaderSrc).data(),
                              ITEM_DATATYPE_HEADPATH);
            }
        }
    }
}

/**
 * @brief SessionFrm::onClearUnreadAct
 */
void SessionFrm::onClearUnreadAct(bool)
{
    //
    QMap<UID, QUInt8> mapUnreadIds;
    auto it = _sessionMap.begin();

    for (; it != _sessionMap.end(); it++)
    {
        unsigned int count = (*it)->data(ITEM_DATATYPE_UNREADCOUNT).toUInt();

        if (count > 0)
        {
            QUInt8 chatType = static_cast<QUInt8>((*it)->data(ITEM_DATATYPE_CHATTYPE).toInt());
            (*it)->setData(0, ITEM_DATATYPE_UNREADCOUNT);
            mapUnreadIds[it.key()] = chatType;
        }
    }

    _totalUnReadCount = 0;
    emit updateTotalUnreadCount(_totalUnReadCount);
    QtConcurrent::run([mapUnreadIds, this]()
    {
        auto itr = mapUnreadIds.begin();

        for (; itr != mapUnreadIds.end(); itr++)
        {
            QString messageId = _sessionMap[itr.key()]->data(ITEM_DATATYPE_LAST_MESSAGE_ID).toString();
            emit sgShowUnreadMessage(0, itr.key(), "", 0, 0);
            NavigationMsgManager::sendReadedMessage(messageId.toStdString(), itr.key().realId(), *itr);
        }
    });
}

/**
 * @brief SessionFrm::onUserConfigChanged
 * @param uid
 */
void SessionFrm::onUserConfigChanged(const QTalk::Entity::UID &uid)
{
    if (_sessionMap.contains(uid))
    {
        int type = _sessionMap[uid]->data(ITEM_DATATYPE_CHATTYPE).toInt();
        bool isTop = _mapStick.find(uid.toStdString()) != _mapStick.end();
        bool unNotice = _mapNotice.find(uid.realId()) != _mapNotice.end();
        _sessionMap[uid]->setData(isTop, ITEM_DATATYPE_ISTOP);
        _sessionMap[uid]->setData(unNotice, ITEM_DATATYPE_UNNOTICE);
        _sessionMap[uid]->setData(getUserName(uid.realId(), type == QTalk::Enum::GroupChat), ITEM_DATATYPE_USERNAME);
        // sort
        _pSessionView->update();
        sortAndFilterView();
    }
}

/**
 * @brief SessionFrm::onShowDraft
 * @param uid
 * @param draft
 */
void SessionFrm::onShowDraft(const QTalk::Entity::UID &uid, const QString &draft)
{
    enum
    {
        Type_Invalid,
        Type_Text,
        Type_Image,
        Type_At,
        Type_Url,
    };

    if (!_sessionMap.contains(uid))
        return;

    //
    QString a_data;
    auto document = QJsonDocument::fromJson(draft.toUtf8());

    if (document.isNull())
    {
    }
    else
    {
        QJsonArray array = document.array();

        for (auto &&i : array)
        {
            QJsonObject obj = i.toObject();
            int key = obj.value("key").toInt();
            QString value = obj.value("value").toString();

            switch (key)
            {
                case Type_Text:
                    {
                        a_data.append(value);
                        break;
                    }

                case Type_Url:
                    {
                        a_data.append(tr("[链接]"));
                        break;
                    }

                case Type_Image:
                    {
                        a_data.append(tr("[图片]"));
                        break;
                    }

                case Type_At:
                    {
                        a_data.append(QString("@%1").arg(value));
                        break;
                    }

                case Type_Invalid:
                default:
                    break;
            }
        }
    }

    a_data.replace("\n", " ");

    if (_sessionMap.contains(uid) && !a_data.trimmed().isEmpty())
        _sessionMap[uid]->setData(a_data, ITEM_DATATYPE_DRAFT);
}

/**
 * @brief SessionFrm::onAppDeactivated
 */
void SessionFrm::onAppDeactivated()
{
    _curUserId = UID();
}

/**
 * @brief SessionFrm::onAppActive
 */
void SessionFrm::onAppActive()
{
    auto index = _pSessionView->currentIndex();

    if (index.isValid())
    {
        auto chatType = index.data(ITEM_DATATYPE_CHATTYPE).toInt();
        auto userId = index.data(ITEM_DATATYPE_USERID).toString();
        auto realJid = index.data(ITEM_DATATYPE_REALJID).toString();
        UID uid(userId, realJid);
        _curUserId = uid;
        {
            _sessionMap[uid]->setData(false, ITEM_DATATYPE_QQQ);
            sortAndFilterView();
        }
        int count = index.data(ITEM_DATATYPE_UNREADCOUNT).toUInt();

        if (count > 0)
        {
            QString messageId = index.data(ITEM_DATATYPE_LAST_MESSAGE_ID).toString();
            NavigationMsgManager::sendReadedMessage(messageId.toStdString(), realJid.toStdString(),
                                                    chatType);
            _sessionMap[uid]->setData(0, ITEM_DATATYPE_UNREADCOUNT);
            _sessionMap[uid]->setData(0, ITEM_DATATYPE_ATCOUNT);
            bool unNotice = _mapNotice.find(realJid.toStdString()) != _mapNotice.end();

            if (!unNotice)
            {
                _totalUnReadCount -= count;
                emit updateTotalUnreadCount(_totalUnReadCount);
                emit sgShowUnreadMessage(0, uid, "", 0, 0);
            }
        }
    }
}

/**
 *
 * @param messageId
 * @param time
 */
void SessionFrm::onGotMState(const QTalk::Entity::UID &uid, const QString &messageId, const long long &time)
{
    if (!PLAT.isMainThread())
    {
        emit sgGotMState(uid, messageId, time);
        return;
    }

    if (_sessionMap.contains(uid) && _sessionMap[uid])
    {
        auto itemMsgId = _sessionMap[uid]->data(ITEM_DATATYPE_LAST_MESSAGE_ID).toString();

        if (messageId == itemMsgId)
            _sessionMap[uid]->setData(time, ITEM_DATATYPE_LASTTIME);
    }
}

/**
 * @brief SessionFrm::onQuitGroupAct
 */
void SessionFrm::onQuitGroupAct(bool)
{
    QModelIndex index = _pSessionView->currentIndex();

    if (index.isValid())
    {
        QString xmppId = index.data(ITEM_DATATYPE_USERID).toString();
        int chatType = index.data(ITEM_DATATYPE_CHATTYPE).toInt();
        QString name = index.data(ITEM_DATATYPE_USERNAME).toString();
        int ret = QtMessageBox::warning(this, tr("警告"), tr("即将退出群聊%1 (%2), 是否继续?").arg(name, xmppId),
                                        QtMessageBox::EM_BUTTON_YES | QtMessageBox::EM_BUTTON_NO);

        if (ret == QtMessageBox::EM_BUTTON_YES)
        {
            QtConcurrent::run([xmppId, chatType]()
            {
                if (chatType == QTalk::Enum::GroupChat)
                    NavigationMsgManager::quitGroupById(xmppId.toStdString());
            });
        }
    }
}

/// add by 2020-12-29
/**
 * @brief SessionFrm::receiveSession
 * @param mess
 */
void SessionFrm::receiveSession(R_Message mess)
{
    QTalk::Entity::ImMessageInfo message = mess.message;
    ReceiveSession info;
    info.chatType = (QTalk::Enum::ChatType)message.ChatType;
    info.messageId = QString::fromStdString(message.MsgId);
    info.messageContent = QString::fromStdString(message.Content);
    info.xmppId = QString::fromStdString(QTalk::Entity::JID(message.SendJid).basename());
    info.realJid = QString::fromStdString(QTalk::Entity::JID(message.RealJid).basename());
    info.messageRecvTime = message.LastUpdateTime;
    std::string from = QTalk::Entity::JID(message.From).basename();
    info.sendJid = QString::fromStdString(from);
    info.messtype = message.Type;
    emit sgReceiveSession(info, from == _strSelfId);
}

/**
 * @brief SessionFrm::sendSession
 * @param mess
 */
void SessionFrm::sendSession(S_Message mess)
{
    QTalk::Entity::ImMessageInfo message = mess.message;
    ReceiveSession info;
    info.chatType = (QTalk::Enum::ChatType)message.ChatType;
    info.messageId = QString::fromStdString(message.MsgId);
    info.messageContent = QString::fromStdString(message.Content);
    info.xmppId = QString::fromStdString(message.To);
    info.realJid = QString::fromStdString(message.RealJid);
    info.messageRecvTime = message.LastUpdateTime;
    info.messtype = message.Type;
    info.sendJid = QString::fromStdString(_strSelfId);
    emit sgReceiveSession(info, true);
}

/**
 * @brief SessionFrm::onTcpDisconnect
 */
void SessionFrm::onTcpDisconnect()
{
    emit setDisconnectWgtVisible(true);
}

/**
 * @brief SessionFrm::onLoginSuccess
 */
void SessionFrm::onLoginSuccess()
{
    emit setDisconnectWgtVisible(false);
    QtConcurrent::run(&NavigationMsgManager::getSessionData);
}

/**
 * @brief SessionFrm::onUpdateUserConfig
 * @param arConfigs
 */
void SessionFrm::onUpdateUserConfig(const std::vector<QTalk::Entity::ImConfig> &arConfigs)
{
    //
    std::map<std::string, std::string> tmpStick;
    std::map<std::string, std::string> tmpNotice;
    std::vector<std::string> arSatr;
    std::vector<std::string> arBlackList;
    auto it = arConfigs.begin();

    for (; it != arConfigs.end(); it++)
    {
        if (it->ConfigKey == "kStickJidDic")
        {
            QString xmppId = QString::fromStdString(it->ConfigSubKey);
            tmpStick[xmppId.toStdString()] = it->ConfigValue;
        }
        else if (it->ConfigKey == "kNoticeStickJidDic")
            tmpNotice[it->ConfigSubKey] = it->ConfigValue;
        else if (it->ConfigKey == "kStarContact")
            arSatr.push_back(it->ConfigSubKey);
        else if (it->ConfigKey == "kBlackList")
            arBlackList.push_back(it->ConfigSubKey);
    }

    //
    {
        QMutexLocker locker(&_mutex);
        _mapStick = tmpStick;
        _mapNotice = tmpNotice;
        _arSatr = arSatr;
        _arBlackList = arBlackList;
        //
        pSessions = DB_PLAT.reloadSession();
    }
    emit loadSession();
}

/**
 * @brief SessionFrm::onUpdateUserConfig
 * @param deleteData
 * @param arImConfig
 */
void SessionFrm::onUpdateUserConfig(const std::map<std::string, std::string> &deleteData,
                                    const std::vector<QTalk::Entity::ImConfig> &arImConfig)
{
    QMutexLocker locker(&_mutex);

    for (const auto &it : deleteData)
    {
        QTalk::Entity::UID uid(it.first);

        //
        if (it.second == "kStickJidDic")
            _mapStick.erase(it.first);
        else if (it.second == "kNoticeStickJidDic")
            _mapNotice.erase(it.first);
        else if (it.second == "kStarContact")
        {
            auto itFind = std::find(_arSatr.begin(), _arSatr.end(), it.first);

            if (itFind != _arSatr.end())
                _arSatr.erase(itFind);
        }
        else if (it.second == "kBlackList")
        {
            auto itFind = std::find(_arBlackList.begin(), _arBlackList.end(), it.first);

            if (itFind != _arBlackList.end())
                _arBlackList.erase(itFind);
        }

        //
        emit sgUserConfigChanged(uid);
    }

    //
    for (const auto &conf : arImConfig)
    {
        QTalk::Entity::UID uid(conf.ConfigSubKey);

        if (conf.ConfigKey == "kStickJidDic")
            _mapStick[conf.ConfigSubKey] = conf.ConfigValue;
        else if (conf.ConfigKey == "kNoticeStickJidDic")
            _mapNotice[conf.ConfigSubKey] = conf.ConfigValue;
        else if (conf.ConfigKey == "kStarContact")
            _arSatr.push_back(conf.ConfigSubKey);
        else if (conf.ConfigKey == "kBlackList")
            _arBlackList.push_back(conf.ConfigSubKey);
        else
        {
        }

        //
        emit sgUserConfigChanged(uid);
    }
}

/**
 * @brief SessionFrm::onChangeHeadRet
 * @param ret
 * @param xmppId
 * @param localHead
 */
void SessionFrm::onGotUserCards(const std::vector<QTalk::StUserCard> &cards)
{
    QVector<QString> users;

    for (auto &card : cards)
        users.push_back(card.xmppId.data());

    if (!users.empty())
        emit sgUpdateUserInfo(users);
}

void SessionFrm::onGotUserCard(const QString &user)
{
    emit sgUpdateUserInfo({user});
}


/**
 * @brief SessionFrm::updateTatalReadCount
 */
void SessionFrm::updateTatalReadCount()
{
    emit updateTotalUnreadCount(_totalUnReadCount);
}
