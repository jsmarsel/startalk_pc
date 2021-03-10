#ifndef CHATVIEWMIANPANEL_H
#define CHATVIEWMIANPANEL_H

#include <QFrame>
#include <vector>
#include <QVector>
#include <QMap>
#include <QMutexLocker>
#include <QLabel>
#include <set>
#include <QMediaPlayer>
#include <QRcode/QRcode.h>
#include <QPointer>
#include <QFuture>
#include "search/FileRecordWnd.h"
#include "../UICom/UIEntity.h"
#include "MessageManager.h"
#include "NativeChatStruct.h"
#include "../Message/ChatMessage.h"
#include "../include/im_enum.h"
#include "../QtUtil/lib/ini/ConfigLoader.h"
#include "../Message/GroupMessage.h"
#include "VideoPlayWnd.h"
#include "../entity/UID.h"
#include "../entity/im_transfer.h"
#include "GIFManager.h"
#include "../include/STLazyQueue.h"
#include "../WebService/AudioVideo.h"
#include "search/MessageRecordManager.h"
#include "ChatViewItem.h"
#include "../include/FrequencyMap.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtConcurrent>

#include "sound/amr/WavOutput.h"
#include "DropWnd.h"

#define QT_CONCURRENT_FUNC QtConcurrent::run

//
class SnipScreenTool;
class SelectUserWnd;
class MessageItemBase;
class MessagePrompt;
class VoiceMessageItem;
class ChatViewMainPanel : public QFrame
{
    Q_OBJECT

public:
    explicit ChatViewMainPanel(QWidget *parent = 0);
    ~ChatViewMainPanel() override;

public slots:
    void onChatUserChanged(const StSessionInfo &info);
    void onAppDeactivated();
    void packAndSendLog(const QString &describe);
    void systemShortCut();
    void onRemoveSession(const QString &);
    void onAppActive();
    void onMsgSoundChanged();
    void setAutoReplyFlag(bool);
    void onShowSearchResult(const QString &key, const QString &xmppId);
    void onShowSearchFileWnd(const QString &key);

Q_SIGNALS:
    void showUserCardSignal(const QString &);
    void showGroupCardSignal(const QString &);
    void sgOpenNewSession(const StSessionInfo &);
    void creatGroup(const QString &);
    void addGroupMember(const QString &);
    void showChatPicture(const QString &, const QString &, int);
    void sgShowPicture(const QString &, const QString &);
    void showMessagePromptSignal(const QTalk::Entity::ImMessageInfo &);
    void recvMessageSignal();
    void sgSwitchCurFun(int);
    void sgJumpToStructre(const QString &);
    void sgShortCutSwitchSession(int); // 快捷键切换会话
    void sgForwardMessage(const QString &messageId);
    void sgReloadChatView();
    void sgDisconnected();
    void sgShowLiteMessageBox(bool isSuccess, const QString &message);
    void sgShowInfoMessageBox(const QString &message);
    void sgPlaySound();
    void sgShowVidioWnd(const std::string &caller, const std::string &callee, bool);
    void sgWakeUpWindow();
    void sgOperator(const QString &desc);
    void showQRcode();
    void sgUserSendMessage();
    void sgShowNotify(const QTalk::StNotificationParam &);
    void sgShowDraft(const QTalk::Entity::UID &, const QString &);
    void sgShowUserCard(const QString &);
    void sgUpdateGroupMember(const std::string &, const std::vector<QTalk::StUserCard> &);

public:
    void setConnectStatus(bool isConnect);
    bool getConnectStatus();
    std::string getSelfUserId();

public:
    void onRecvMessage(R_Message &e, bool isCarbon);
    void onRecvFileProcessMessage(const FileProcessMessage &e);
    void updateGroupMember(const GroupMemberMessage &e);
    void updateHead();
    void updateUserHeadPath(const std::vector<QTalk::StUserCard> &users);
    void updateGroupTopic(const std::string &groupId, const std::string &topic);
    void updateGroupMemberInfo(const std::string &groupId, const std::vector<QTalk::StUserCard> &userCards);
    void getHistoryMsg(const QInt64 &t, const QUInt8 &chatType, const QTalk::Entity::UID &uid);
    void gotReadState(const QTalk::Entity::UID &uid, const std::map<std::string, QInt32> &msgMasks);
    void gotMState(const QTalk::Entity::UID &uid, const std::string &messageId, const QInt64 &time);
    void updateRevokeMessage(const QTalk::Entity::UID &uid, const std::string &fromId, const std::string &messageId, const long long time);
    void onDestroyGroup(const std::string &groupId, bool isDestroy);
    void recvBlackMessage(const std::string &fromId, const std::string &messageId);
    void addConllection(const QString &key, const QString &path);
    void updateGroupInfo(const std::shared_ptr<QTalk::StGroupInfo> &groupinfo);
    void onRemoveGroupMember(const std::string &groupId, const std::string &memberId);
    void onLogReportMessageRet(bool isSuccess, const std::string &message);
    void recvUserStatus(const std::string &user, const std::string &userstatus);
    //	void onRecvVideo(const std::string& peerId);
    void onRecvAddGroupMember(const std::string &groupId, const std::string &memberId, const std::string &nick, int affiliation);
    void onRecvRemoveGroupMember(const std::string &groupId, const std::string &memberId);
    void onChangeHeadSuccess(const std::string &localHead);
    void onUpdateMoodSuccess(const std::string &userId, const std::string &mood);
    void onDisConnected();
    void onRecvWebRtcCommand(int msgType, const std::string &peerId, const std::string &cmd, bool isCarbon);
    void onRecvLoginProcessMessage(const char *message);

public:
    void setFileMsgLocalPath(const std::string &key, const std::string &val);
    std::string getFileMsgLocalPath(const std::string &key);
    std::string uploadFile(const std::string &localFile);

public:
    void insertAt(const QString &groupId, const QString &userId);
    void clapSomebody(const QString &groupId, const QString &userId);
    void showUserCard(const QString &userId);
    void removePrompt(MessagePrompt *prompt);
    //
    void updateUserConfig(const std::vector<QTalk::Entity::ImConfig> &configs);
    void updateUserConfig(const std::map<std::string, std::string> &deleteData,
                          const std::vector<QTalk::Entity::ImConfig> &arImConfig);
    void updateUserMaskName();
    void onShowChatWnd(QUInt8, QString, QString, QString);
    void showShowCodeWnd(const QString &type, const QString &language, const QString &content);
    void showSendCodeWnd(const QTalk::Entity::UID &uid);
    void sendCodeMessage(const QTalk::Entity::UID &uid, const std::string &text,
                         const std::string &codeType, const std::string &codeLanguage);
    void sendShareMessage(const QString &, int type, const QString &);
    // search share
    void searchLocalSession(int type, const std::string &key);
    void forwardMessage(const std::string &messageId);
    void playVoice(const std::string &localFile, VoiceMessageItem *msgItem);
    //
    void resendMessage(MessageItemBase *item);
    //
    void scanQRcCodeImage(const QString &imgPath);
    //
    static void postInterface(const std::string &url, const std::string &params);
    //
    static void sendGetRequest(const std::string &url);
    //
    void updateMessageExtendInfo(const std::string &msgId, const std::string &info);

public:
    // 发消息前处理 -> 入库
    void preSendMessage(const QTalk::Entity::UID &uid,
                        int chatType,
                        int msgType, const QString &info, const std::string &messageId);
    void sendTextMessage(const QTalk::Entity::UID &uid,
                         int chatType,
                         const std::string &text,
                         const std::map<std::string, std::string> &mapAt = std::map<std::string, std::string>(),
                         const std::string &messageId = std::string());
    void sendCodeMessage(
        const QTalk::Entity::UID &uid,
        int chatType,
        const std::string &text,
        const std::string &codeType,
        const std::string &codeLanguage,
        const std::string &messageId = std::string());
    void sendFileMessage(const QTalk::Entity::UID &uid,
                         int chatType,
                         const QString &filePath,
                         const QString &fileName,
                         const QString &fileSize,
                         const std::string &messageId = std::string());
    void sendAudioVideoMessage(const QTalk::Entity::UID &uid, int chatType, bool isVideo = false, const QString &json = QString());
    void sendShockMessage(const QTalk::Entity::UID &uid, int chatType);
    void sendEmoticonMessage(const QString &id, const QString &messageContent, bool isShowAll, const std::string &messageId = std::string());
    void sendCollectionMessage(const QString &id, const QString &imagePath, const QString &imgLink);

    //
    void onUserMadelChanged(const std::vector<QTalk::Entity::ImUserStatusMedal> &userMedals);

public:
    //
    void getUserMedal(const std::string &xmppId, std::set<QTalk::StUserMedal> &medal);

public:
    void showDropWnd(const QPixmap &mask, const QString &name);
    void cancelDrop();

protected:
    void dropEvent(QDropEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;

protected:
    void initPlug(); // 加载截屏 表情插件
    void showPrompt(const QTalk::Entity::ImMessageInfo &msg);
    void showEmptyLabel();
    void onSnapFinish(const QString &);
    void onSnapCancel(const QString &);
    void reloadChatView();
    void showLiteMessageBox(bool isSuccess, const QString &message);
    void showInfoMessageBox(const QString &message);
    void initSound();
    void playSound();
    void onShowVideoWnd(const std::string &caller, const std::string &callee, bool isVideo);
    void sendAutoPeplyMessage(const QTalk::Entity::UID &uid, const std::string &messageId);

public:
    void makeFileLink(const QString &filePath, const QString &fileMd5);
    QString getFileLink(const QString &fileMd5);

public:
    void setSearchStyle(const QString &qss);

public:
    QString getRealText(const QString &json, const QString &msgId, bool &success, std::map<std::string, std::string> &mapAt);

public:
    void start2Talk_old(const std::string &peerId, bool isVideo, bool isCall);
    void startGroupTalk(const QString &id, const QString &name);
    void onSendSignal(const QString &json, const QString &id);

public:
    void downloadFileWithProcess(const QString &url, const QString &path, const QString &key, QWidget *item);

private slots:
    void onHttpError(QNetworkReply::NetworkError err);
    void onSSLError(const QList<QSslError> &errors);

public:
    GIFManager *gifManager;

private:
    std::string _strSelfId; // 自己id
    bool _isConnected = true;

private:
    ChatViewItem *_pViewItem{};
    FrequencyMap<QTalk::Entity::UID, ChatViewItem *> _mapItems; // items

private:
    QStackedLayout *_pStackedLayout;
    QMap<QTalk::Entity::UID, QString> _his_input_data;
    QMap<QString, QString> _mapHeadPath; //头像路径
    QVector<MessagePrompt *> _arMessagePrompt;
    QLabel *_pEmptyLabel{};
    //	QLabel*                     _pLoadingLabel{};
    SnipScreenTool *_pSnipScreenTool{};
    CodeShowWnd *_pCodeShowWnd{};
    SendCodeWnd *_pSendCodeWnd{};
    SelectUserWnd *_pSelectUserWnd{};

    QRcode *_pQRcode{};

    DropWnd *_dropWnd{};

private:
    QMutex _mutex;

private:
    std::map<std::string, std::string> _mapNotice; //

private:
    QTalk::ConfigLoader *_pFileMsgConfig;

private:
    QMediaPlayer *_pPlayer{};
#ifndef _WINDOWS
    QMediaPlayer *_pVoicePlayer{};
#endif // !_WINDOWS

private:
    bool _autoReply = false;

public:
    QString _qss;

private:
    AudioVideoManager *_pAudioVideoManager{};

private:
    std::map<std::string, std::set<QTalk::StUserMedal>> _user_medals;

private:
    MessageRecordManager *_pMsgRecordManager{};
    FileRecordWnd *_pFileRecordWnd{};

private:
    QMutex _sendingMutex;
    std::set<std::string> _sending;

public:
    void addSending(const std::string &msgId);
    void eraseSending(const std::string &msgId);
    bool isSending(const std::string &msgId);

private:
    QNetworkAccessManager *_netManager{};
    QThread _downloadImageThread;

public:
    inline QThread &downloadImageThread() { return _downloadImageThread; }
};

#endif // CHATVIEWMIANPANEL_H
