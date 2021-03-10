﻿#include "GroupMember.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QImage>
#include <QFileInfo>
#include <QSettings>
#include <QScrollBar>
#include <QApplication>
#include "../Platform/Platform.h"
#include "../QtUtil/Entity/JID.h"
#include "ChatViewMainPanel.h"
#include "GroupItemDelegate.h"
#include "MessageManager.h"
#include "../Platform/dbPlatForm.h"
#include "../QtUtil/Utils/Log.h"
#include "../CustomUi/QtMessageBox.h"
#include <QFileDialog>
#include <QDesktopServices>

#define HEAD_RADIUS 40

using namespace QTalk;
extern ChatViewMainPanel *g_pMainPanel;

GroupMember::GroupMember(QString& groupId, QWidget *parent)
        : QFrame(parent),
          _selfIsCreator(false) ,
          _selfIsAdmin(false),
          _groupId(groupId){
    initUi();

}

/**
  * @函数名   addMember
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void
GroupMember::addMember(const std::string &xmppid,
        const std::string &userName,
        const QString &headSrc,
        QInt8 userType,
        bool isOnline,
        const QString& searchKey) {
//    static int index = 0;
//    if(++index == 5)
//    {
//        QApplication::processEvents(QEventLoop::AllEvents, 100);
//        index = 0;
//    }
    if (_pMemberList && !_mapMemberItem.contains(xmppid)) {

        auto *item = new QStandardItem();
        QString strId = QString::fromStdString(xmppid);
        item->setData(strId, EM_ITEMDATA_TYPE_XMPPID);
        item->setData(strId.mid(0, strId.lastIndexOf("@")), EM_ITEMDATA_TYPE_USERID);
        item->setData(QString::fromStdString(userName), EM_ITEMDATA_TYPE_USERNAME);
        item->setData(searchKey, EM_ITEMDATA_TYPE_SEARCHKEY);
        item->setData(isOnline, EM_ITEMDATA_TYPE_ISONLINE);
        item->setData(
                QString::fromStdString(DB_PLAT.getMaskName(xmppid)),
                EM_ITEMDATA_TYPE_MASKNAME);
        item->setData(userType, EM_ITEMDATA_TYPE_USERTYPE);
        if (strId.toStdString() == PLAT.getSelfXmppId()) {
            if (userType == 1) {
                _selfIsCreator = true;
                _selfIsAdmin = false;
            } else {
                _selfIsCreator = false;
                _selfIsAdmin = (userType == 2);
            }
        }
        QFileInfo head(headSrc);
        if (!head.exists() || !head.isFile()) {
#ifdef _STARTALK
            QString headPath = ":/QTalk/image1/StarTalk_defaultHead.png";
#else
            QString headPath = ":/QTalk/image1/headPortrait.png";
#endif
            item->setData(headPath, EM_ITEMDATA_TYPE_HEADPATH);
        } else {
            QString tmphead(head.absoluteFilePath());
            item->setData(tmphead, EM_ITEMDATA_TYPE_HEADPATH);
        }

        {
            _srcModel->appendRow(item);
            std::lock_guard<QTalk::util::spin_mutex> lock(_sm);
            _mapMemberItem[xmppid] = item;
        }

        _pModel->sort(0);
    }
}

/**
  * @函数名   deleteMember
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void GroupMember::deleteMember(const std::string &xmppid) {

    qInfo() << "deleteMember" << xmppid.data() << _groupId.data();
    {
        std::lock_guard<QTalk::util::spin_mutex> lock(_sm);
        //
        if (_mapMemberItem.contains(xmppid))
        {
            auto *pItem = _mapMemberItem[xmppid];
            bool isOnline = pItem->data().toBool();
            _mapMemberItem.remove(xmppid);
            _srcModel->removeRow(pItem->row());
            setMemberCount(_srcModel->rowCount(), isOnline ? --groupMemberOnlineCount : groupMemberOnlineCount);
        }
        _pModel->sort(0);
    }
}

void GroupMember::updateGroupMember(const std::string& memberJid, const std::string& nick, int affiliation) {

    std::string userId = Entity::JID(memberJid).basename();
    std::shared_ptr<QTalk::Entity::ImUserInfo> userInfo = DB_PLAT.getUserInfo(memberJid);
    bool isOnline = PLAT.isOnline(userId);

    std::lock_guard<QTalk::util::spin_mutex> lock(_sm);
    if (_mapMemberItem.contains(memberJid)) {
        if(userInfo)
        {
            std::string headSrc = GetHeadPathByUrl(userInfo->HeaderSrc);
            _mapMemberItem[memberJid]->setData(QString(headSrc.data()), EM_ITEMDATA_TYPE_HEADPATH);
        }
        _mapMemberItem[memberJid]->setData(affiliation, EM_ITEMDATA_TYPE_USERTYPE);
        _mapMemberItem[memberJid]->setData(isOnline, EM_ITEMDATA_TYPE_ISONLINE);

    } else {
        if(userInfo)
        {
            std::string headSrc = GetHeadPathByUrl(userInfo->HeaderSrc);
            emit addMemberSignal(memberJid, nick, QString::fromStdString(headSrc), affiliation, isOnline,
                    QString::fromStdString(userInfo->SearchIndex));
        }
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void GroupMember::updateMemberInfo(const std::vector<StUserCard> &users) {

    std::lock_guard<QTalk::util::spin_mutex> lock(_sm);
    for (const auto &user : users) {
        if (_mapMemberItem.contains(user.xmppId)) {
            std::string headSrc = GetHeadPathByUrl(user.headerSrc);
            QFileInfo head(QString(headSrc.c_str()));
            if (!head.exists() || !head.isFile()) {
#ifdef _STARTALK
                QString headPath = ":/QTalk/image1/StarTalk_defaultHead.png";
#else
                QString headPath = ":/QTalk/image1/headPortrait.png";
#endif
                _mapMemberItem[user.xmppId]->setData(headPath, EM_ITEMDATA_TYPE_HEADPATH);
            } else {
                QString tmphead(head.absoluteFilePath());
                _mapMemberItem[user.xmppId]->setData(tmphead, EM_ITEMDATA_TYPE_HEADPATH);
            }
            std::string name = user.nickName;
            if(name.empty())
                name = user.userName;
            if(name.empty())
                name = QTalk::Entity::JID(user.xmppId).username();

            _mapMemberItem[user.xmppId]->setData(QString::fromStdString(name), EM_ITEMDATA_TYPE_USERNAME);
            _mapMemberItem[user.xmppId]->setData(
                    QString::fromStdString(DB_PLAT.getMaskName(user.xmppId)),
                    EM_ITEMDATA_TYPE_MASKNAME);
        }
    }
    _pModel->sort(0);
}

/**
  * @函数名   initUi
  * @函数名   initUi
  * @功能描述 初始化画面
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void GroupMember::initUi() {
    setFrameShape(QFrame::NoFrame);
    setObjectName("GroupMember");
    //

    _pSearchLineEdit = new QLineEdit(this);
    _pSearchBtn = new QPushButton(this);

    _pMemberList = new QListView(this);
    _pMemberList->verticalScrollBar()->setVisible(false);
    _pMemberList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pMemberList->verticalScrollBar()->setSingleStep(10);
    _srcModel = new QStandardItemModel(this);
    _pModel = new GroupMemberSortModel(this);
    _pModel->setSourceModel(_srcModel);
    _pMemberList->setModel(_pModel);
    _pMemberList->setItemDelegate(new GroupItemDelegate(this));

    _pMemberList->setFrameShape(QFrame::NoFrame);
    _pMemberList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    _pSearchLineEdit->setPlaceholderText(tr("点此搜索"));
    _pSearchLineEdit->setObjectName("GroupMemberSearchLineEdit");
    _pSearchLineEdit->installEventFilter(this);

    _pSearchBtn->setFixedSize(20, 20);
    _pSearchBtn->setObjectName("GroupMemberSerchBtn");
    _pMemberList->setObjectName("GroupMemberListWgt");

    _pMemberList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 10, 0);
    topLayout->addWidget(_pSearchLineEdit);
    topLayout->addWidget(_pSearchBtn);

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(13, 12, 0, 0);
    layout->addLayout(topLayout);
    layout->addWidget(_pMemberList);
    setLayout(layout);

    //
    _pContextMenu = new QMenu(_pMemberList);
    _pContextMenu->setAttribute(Qt::WA_TranslucentBackground, true);

//    QAction *sendMsgAct = new QAction("发送消息", _pContextMenu);
//    _pContextMenu->addAction(sendMsgAct);

    QAction *showCardAct = new QAction(tr("资料卡片"), _pContextMenu);
    _pContextMenu->addAction(showCardAct);

    _setAdminAction = new QAction(tr("提升管理员"), _pContextMenu);
    _pContextMenu->addAction(_setAdminAction);

    _removeGroupAction = new QAction(tr("移出群组"), _pContextMenu);
    _pContextMenu->addAction(_removeGroupAction);
    _pMemberList->installEventFilter(this);

    connect(_pSearchBtn, &QPushButton::clicked, this, &GroupMember::onSearchBtnClick);
    connect(_pSearchLineEdit, &QLineEdit::textChanged, [this](const QString &text) {
        QString strContent = _pSearchLineEdit->text();
        _pModel->setFilterRegExp(strContent.toLower());
    });

    connect(_pMemberList, &QListView::doubleClicked, [](const QModelIndex &index) {

        if(!index.isValid())
            return;

        QString strUserName(index.data(EM_ITEMDATA_TYPE_USERNAME).toByteArray());
        QString strUserHead(index.data(EM_ITEMDATA_TYPE_HEADPATH).toByteArray());
        QString strUserId(index.data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
        StSessionInfo stSession(QTalk::Enum::TwoPersonChat, strUserId, strUserName);
        stSession.headPhoto = strUserHead;
        emit g_pMainPanel->sgOpenNewSession(stSession);
    });
    connect(showCardAct, &QAction::triggered, [this](bool) {
        QModelIndex index = _pMemberList->currentIndex();
        if(index.isValid())
        {
            QString strUserId(index.data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
            g_pMainPanel->showUserCard(strUserId);
        }
    });
    connect(_setAdminAction, &QAction::triggered, [this](bool) {
        QModelIndex index = _pMemberList->currentIndex();
        if(index.isValid()) {
            std::string xmppId(index.data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
            std::string nick(index.data(EM_ITEMDATA_TYPE_USERNAME).toByteArray());
            int type = index.data(EM_ITEMDATA_TYPE_USERTYPE).toInt();
            if (type == 3) {
                setAdminByJid(nick, xmppId);
            }
            else if(type == 2) {
                removeAdminByJid(nick, xmppId);
            }
            else
            {
                error_log("set admin error id:{0} nick:{1} type{2}", xmppId, nick, type);
            }
        }
    });
    connect(_removeGroupAction, &QAction::triggered, [this](bool) {
        QModelIndex index = _pMemberList->currentIndex();
        if(index.isValid()) {
            std::string xmppId(index.data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
            std::string nick(index.data(EM_ITEMDATA_TYPE_USERNAME).toByteArray());
            removeGroupByJid(nick,xmppId);
        }
    });
    //
    connect(this, &GroupMember::addMemberSignal, this, &GroupMember::addMember);
    connect(this, &GroupMember::sgUpdateMemberCount, this, &GroupMember::setMemberCount, Qt::QueuedConnection);
    connect(this, &GroupMember::sgUpdateUserRole, this, &GroupMember::updateUserRole, Qt::QueuedConnection);
}

void GroupMember::setAdminByJid(const std::string& nick,const std::string& xmppId) {
    ChatMsgManager::setGroupAdmin(_groupId.toStdString(), nick,xmppId, false);
    _pModel->sort(0);
}

void GroupMember::removeAdminByJid(const std::string& nick,const std::string& xmppId) {
    ChatMsgManager::setGroupAdmin(_groupId.toStdString(), nick,xmppId, true);
    _pModel->sort(0);
}

void GroupMember::removeGroupByJid(const std::string& nick,const std::string& xmppId) {
    ChatMsgManager::removeGroupMember(_groupId.toStdString(), nick, xmppId);
    _pModel->sort(0);
}

/**
  * @函数名   onSearchBtnClick
  * @功能描述 检索按钮
  * @参数
  * @author   cc
  * @date     2018/10/15
  */
void GroupMember::onSearchBtnClick() {
//    QString strContent = _pSearchLineEdit->text();
//    _pModel->setFilterRegExp(strContent.toLower());
    _pSearchLineEdit->setFocus();
    //
    QString strContent = _pSearchLineEdit->text();
    if("导出群成员" == strContent)
    {
        int ret = QtMessageBox::question(g_pMainPanel, tr("提示"), tr("是否导出群成员?"));
        if(ret == QtMessageBox::EM_BUTTON_YES)
        {
            _pSearchLineEdit->clear();

            QString historyDir = QString("%1/%2.txt").arg(PLAT.getHistoryDir().data()).arg(_groupId);
            QString path = QFileDialog::getSaveFileName(g_pMainPanel, tr("请选择导出目录"), historyDir);
            if(!path.isEmpty())
            {
                QString data;

                for(const auto &memberId : _mapMemberItem.keys())
                {
                    data.append(QString(memberId.data()).section("@", 0, 0));
                    data.append("; \n");
                }

                QFile file(path);
                if(file.open(QIODevice::WriteOnly))
                {
                    file.write(data.toUtf8());
                    file.close();
                }

                QDesktopServices::openUrl(QUrl::fromLocalFile(path));
            }

        }
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/15
  */
bool GroupMember::eventFilter(QObject *o, QEvent *e) {
    if (o == _pMemberList)
    {
        if(e->type() == QEvent::ContextMenu)
        {
            QModelIndex index = _pMemberList->currentIndex();
            if(!index.isValid())
                return false;

            unsigned int affline = index.data(EM_ITEMDATA_TYPE_USERTYPE).toUInt();
            std::string xmppId(index.data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
            if (xmppId == PLAT.getSelfXmppId()) {
                _setAdminAction->setVisible(false);
                _removeGroupAction->setVisible(false);
            } else {
                if (_selfIsCreator) {
                    if (affline == 2) {
                        _setAdminAction->setText(tr("解除管理员"));
                        _setAdminAction->setVisible(true);
                        _removeGroupAction->setVisible(false);
                    } else {
                        _setAdminAction->setText(tr("提升管理员"));
                        _setAdminAction->setVisible(true);
                        _removeGroupAction->setVisible(true);
                    }
                } else if (_selfIsAdmin) {
                    if (affline == 1) {
                        _setAdminAction->setText(tr("无法操作"));
                        _setAdminAction->setVisible(false);
                        _removeGroupAction->setVisible(false);
                    } else {
                        _setAdminAction->setText(tr("提升管理员"));
                        _setAdminAction->setVisible(false);
                        _removeGroupAction->setVisible(affline != 2);
                    }
                } else {
                    _setAdminAction->setText(tr("提升管理员"));
                    _setAdminAction->setVisible(false);
                    _removeGroupAction->setVisible(false);
                }
                _pModel->sort(0);
            }
            _pContextMenu->exec(QCursor::pos());
        }
        else if(e->type() == QEvent::Enter)
        {
            _pMemberList->verticalScrollBar()->setVisible(true);
        }
        else if(e->type() == QEvent::Leave)
        {
            _pMemberList->verticalScrollBar()->setVisible(false);
        }
    }

    return QFrame::eventFilter(o, e);
}

//
void GroupMember::updateHead() {

    {
        unsigned int onlineCount = 0;
        for (const auto item : _mapMemberItem) {
            std::string xmppId(item->data(EM_ITEMDATA_TYPE_XMPPID).toByteArray());
            bool isOnline = PLAT.isOnline(xmppId);
            if(isOnline)
                onlineCount++;

            item->setData(isOnline, EM_ITEMDATA_TYPE_ISONLINE);
        }
        emit sgUpdateMemberCount(_mapMemberItem.size(), onlineCount);
    }
    _pModel->sort(0);
}

/**
 *
 * @param allCount
 * @param onlineCount
 */
void GroupMember::setMemberCount(unsigned int allCount, unsigned int onlineCount)
{
    if(_pSearchLineEdit)
    {
        groupMemberCount = allCount;
        groupMemberOnlineCount = onlineCount;
        _pSearchLineEdit->setPlaceholderText(QString(tr("成员 %1/ %2")).arg(onlineCount).arg(allCount));
    }
}


void GroupMember::clearData() {

//    setMemberCount(0, 0);
//    //
//    std::lock_guard<QTalk::util::spin_mutex> lock(_sm);
//    groupMemberCount = groupMemberOnlineCount = 0;
//    _mapMemberItem.clear();
//    _srcModel->clear();
}

//
void GroupMember::updateUserRole(const QString& xmppId, int role) {
    auto id = xmppId.toStdString();
    if(_mapMemberItem.contains(id) && _mapMemberItem[id])
    {
        _mapMemberItem[id]->setData(role, EM_ITEMDATA_TYPE_USERTYPE);
    }
    _pModel->sort(0);
}
