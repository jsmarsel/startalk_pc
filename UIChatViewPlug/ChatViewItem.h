//
// Created by cc on 2019/12/25.
//

#ifndef QTALK_V2_CHATVIEWITEM_H
#define QTALK_V2_CHATVIEWITEM_H

#include <QFrame>
#include <QPushButton>
#include <QHBoxLayout>
#include <set>
#include <QLabel>
#include "../entity/UID.h"
#include "../entity/im_message.h"
#include "../entity/im_transfer.h"
#include "../include/CommonDefine.h"
#include "../include/im_enum.h"
#include "NativeChatStruct.h"

#define SYSTEM_XMPPID "SystemMessage"
#define RBT_SYSTEM "rbt-system"
#define RBT_NOTICE "rbt-notice"
#define SYSTEM_CONVERSIONID "SystemConversionId"
#define compare_doule_Equal(x, y) (abs(x - y) < 0.00001)

class QSplitter;
class ShareMessageFrm;
class LocalSearchMainWgt;
class StatusWgt;
class ChatMainWgt;
class GroupChatSidebar;
class ToolWgt;
class InputWgt;
class QHBoxLayout;
class QStackedLayout;
class ChatViewMainPanel;
class QMediaPlayer;
class SendCodeWnd;
class CodeShowWnd;
class ChatViewItem : public QFrame
{
    Q_OBJECT
public:
    ChatViewItem();
    ~ChatViewItem() override;

Q_SIGNALS:
    void sgRemoveGroupMember(const std::string&);
    void sgUpdateUserStatus(const QString& sts);
    void sgDeleteLater();
    //
    void sgShowMessage(StNetMessageResult, int);
    void sgLoadingMovie(bool show);

public:
    QTalk::Entity::UID getPeerId() const;
    QString conversionId();
    void onRecvAddGroupMember(const std::string& memberId, const std::string& nick, int affiliation);
    void onRecvRemoveGroupMember(const std::string& memberId);
    void onShowSearchWnd();

public:
    void switchSession(const QUInt8& chatType, const QString& userName, const QTalk::Entity::UID& uid);
    void showMessage(const QTalk::Entity::ImMessageInfo& message, int jumType);

public:
    void setShareMessageState(bool flag);


public:
    void freeView();

private:
    void keyPressEvent(QKeyEvent* e) override;
    void onShowLoading(bool show);

private:
    void initUi();

public:
    QTalk::Entity::UID     _uid;
    QTalk::Enum::ChatType  _chatType{}; // 是否为群组聊天
    ChatMainWgt*      _pChatMainWgt = nullptr;
    GroupChatSidebar* _pGroupSidebar = nullptr;
    InputWgt*         _pInputWgt = nullptr;
    ToolWgt*          _pToolWgt = nullptr;
    StatusWgt*        _pStatusWgt = nullptr;
    QSplitter*        splitter = nullptr;

    LocalSearchMainWgt*    _pSearchMainWgt = nullptr;
    ShareMessageFrm*   _pShareMessageFrm = nullptr;

    QFrame *sendBtnFrm{};

public:
    QString            _name;

private:
    QString            _strConversionId;
    QVBoxLayout*       _leftLay  = nullptr;
    QHBoxLayout*       _sendBtnLay = nullptr;
    QPushButton*       _sendBtn = nullptr;
    QFrame             *_pInputFrm = nullptr;

private:
    QHBoxLayout *pMidLayout{};

    QLabel* _pLoading{};
};



#endif //QTALK_V2_CHATVIEWITEM_H
