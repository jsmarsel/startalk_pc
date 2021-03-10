#if _MSC_VER >= 1600
    #pragma execution_character_set("utf-8")
#endif

#ifndef SESSIONFRM_H
#define SESSIONFRM_H

#include <QFrame>
#include <QListWidget>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QMutexLocker>
#include <QScrollBar>
#include <QStackedWidget>
#include "MessageManager.h"
#include "TcpDisconnect.h"
#include "../include/im_enum.h"
#include "../entity/im_message.h"
#include "../entity/im_config.h"
#include "../entity/UID.h"
#include "../Message/ChatMessage.h"
#include "../Message/LoginMessgae.h"
#include "../entity/im_config.h"
#include "../QtUtil/Entity/JID.h"
#include "../include/STLazyQueue.h"
#include "MessageManager.h"
#include <set>
#include <string>

class ReceiveSession
{
public:
    ReceiveSession()
        : messageRecvTime(0), messtype(-1) {};

    QTalk::Enum::ChatType chatType;
    QString messageId;
    QString messageContent;
    QString xmppId;
    QString realJid;
    QInt64 messageRecvTime;
    QString sendJid;
    int messtype;

    QString headSrc;
};

struct StSessionInfo;

class QMenu;
class QAction;
class SessionSortModel;
class SessionitemDelegate;
class SessionFrm : public QFrame
{
    Q_OBJECT


public:
    explicit SessionFrm(QWidget *parent = nullptr);
    ~SessionFrm() override;

public slots:
    void onloadSessionData();
    void onReceiveSession(const ReceiveSession &mess, bool isSend);
    void onItemSelectChanged(const QModelIndex &index);
    void onDownLoadHeadPhotosFinish();
    void onDownLoadGroupHeadPhotosFinish();
    void onUpdateOnline();
    void onUpdateOnlineUsers(const QString &user);
    void onUpdateGroupInfo(const QTalk::StGroupInfo &gInfo);
    void onUpdateReadedCount(const QTalk::Entity::UID &uid, const int &count);
    void recvRevikeMessage(const QTalk::Entity::UID &uid, const QString &fromId);
    void onToTopAct(bool);
    void onUnNoticeAct(bool);
    void updateUserConfig();
    void onDestroyGroup(const QString &groupId);
    void onUserConfigChanged(const QTalk::Entity::UID &);
    void onShowDraft(const QTalk::Entity::UID &, const QString &draft);
    void onGotMState(const QTalk::Entity::UID &, const QString &messageId, const long long &time);

private:
    void onShowCardAct(bool);
    void onCloseSession(bool);
    void onStarAct(bool);
    void onBlackAct(bool);
    void onClearUnreadAct(bool);
    void onQuitGroupAct(bool);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    bool event(QEvent *e) override;

private:
    void initLayout();
    void connects();
    QString
    GenerateContent(const QString &content, const QUInt8 &chatType, const int &msgType, const QString &userName);
//    void GenerateHeadPhotoName(QString &photosrc);
    void setUserStatus(const QTalk::Entity::UID &, bool check = false);
    QString getUserName(const std::string &xmppId, bool isGroup);
    void sortAndFilterView(int index = -1);

Q_SIGNALS:
    void sgSessionInfo(const StSessionInfo &into);
    void removeSession(const QString &);

    void sgOperator(const QString &desc);
    /// new signal
    void sgReceiveSession(ReceiveSession, bool);
    void sgDownLoadHeadPhotosFinish();
    void sgUpdateOnline();
    void sgUpdateOnlineUsers(const QString &);
    void loadSession();
    void sgUserConfigChanged(const QTalk::Entity::UID &);
    void destoryGroupSignal(const QString &);
    void sgGotMState(const QTalk::Entity::UID &, const QString &, const long long &);
    void sgShortCutSwitchSession(int);
    void sgUpdateUserInfo(const QVector<QString> &);
    void updateGroupInfoSignal(const QTalk::StGroupInfo &);
    void updateReadedCountSignal(const QTalk::Entity::UID &, const int &);
    void recvRevokeMessageSignal(const QTalk::Entity::UID &, const QString &);
    void setDisconnectWgtVisible(bool);
    //
    void showUserCardSignal(const QString &);
    void showGroupCardSignal(const QString &);
    void updateTotalUnreadCount(int);
    void sgShowUnreadMessage(int, const QTalk::Entity::UID &, const QString &, qint64, int);

public: // listener ack messages
    void receiveSession(R_Message mess);
    void sendSession(S_Message mess);
    void onTcpDisconnect();
    void onLoginSuccess();
    void onUpdateUserConfig(const std::vector<QTalk::Entity::ImConfig> &arConfigs);
    void onUpdateUserConfig(const std::map<std::string, std::string> &deleteData,
                            const std::vector<QTalk::Entity::ImConfig> &arImConfig);
//    void onChangeHeadRet(bool ret, const QString &xmppId, const QString &localHead);
    void onGotUserCards(const std::vector<QTalk::StUserCard> &cards);
    void onGotUserCard(const QString &xmppid);
    void updateTatalReadCount();

    void onUpdateUserInfo(const QVector<QString> &);

public slots:
    void onNewSession(const StSessionInfo &into);
    void jumpToNewMessage();
    void onShortCutSwitchSession(int);
    void onAppDeactivated();
    void onAppActive();

private:
    QListView *_pSessionView{};
    QScrollBar *_pSessionScrollBar{};
    QStandardItemModel *_pSrcModel{};
    SessionSortModel *_pModel{};
    SessionitemDelegate *_pItemDelegate{};

private:
    QMenu *_pContextMenu{};
    QAction *_showCardAct{};                               // 名片
    QAction *_closeSessionAct{};                           // 关闭会话
    QAction *_toTopOrCancelTopAct{};                       // 置顶
    QAction *_noticeAct{};                                 // 免打扰
    QAction *_starAct{};                                   // 星标联系人
    QAction *_blackAct{};                                  // 黑名单
    QAction *_clearUnreadAct{};                            // 清除所有未读
    QAction *_quitGroupAct{};                              // 清除所有未读


private:
    int _totalUnReadCount{};

private:
    std::string _strSelfId;
    int _jumpIndex{0};

public:
    QMutex _mutex;
    ImSessions pSessions{nullptr};
    std::map<std::string, std::string> _mapStick;  // 置顶
    std::map<std::string, std::string> _mapNotice; // 面打扰
    std::vector<std::string> _arSatr;              // 星标
    std::vector<std::string> _arBlackList;         // 黑名单
    QTalk::Entity::UID _curUserId;                 // 当前会话
    QMap<QTalk::Entity::UID, QStandardItem *> _sessionMap; //

private:
    QStackedWidget *_pStackWgt{};
    TcpDisconnect *_pTcpDisconnect{};
//    QFrame *_pBackBtnFrm{};
//    QLabel *_pTitleLabel{};
    QFrame *_pSelectFrm{};
    NavigationMsgListener *_messageListener{};

};

#endif // SESSIONFRM_H
