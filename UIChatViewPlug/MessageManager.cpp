﻿#include "MessageManager.h"
#include "../Message/ChatMessage.h"
#include "../EventBus/EventBus.h"
#include "ChatViewMainPanel.h"
#include <QDateTime>
#include <thread>
#include "../Message/GroupMessage.h"
#include "../QtUtil/Utils/Log.h"
#include "../Message/LogicBaseMessage.h"

//
extern ChatViewMainPanel *g_pMainPanel;
/**
  * @函数名   getNetFilePath
  * @功能描述 获取文件网络地址
  * @参数     string 本地地址
     string 网络地址
  * @author   cc
  * @date     2018/09/27
  */
std::string ChatMsgManager::getNetFilePath(const std::string &localFilePath)
{
    LocalImgEvt e;
    e.localFilePath = localFilePath;
    EventBus::FireEvent(e);

    return e.netFilePath;
}

/**
  * @函数名   getLocalFilePath
  * @功能描述 根据网络文件路径下载并获取本地路径
  * @参数
  * @author   cc
  * @date     2018/09/27
  */
std::string ChatMsgManager::getLocalFilePath(const std::string &netFilePath)
{
    NetImgEvt e;
    e.netFilePath = netFilePath;
    EventBus::FireEvent(e);

    return e.localFilePath;
}

std::string ChatMsgManager::getSouceImagePath(const std::string &netFilePath)
{
    SourceNetImage e;
    e.netFilePath = netFilePath;
    EventBus::FireEvent(e);

    return e.localFilePath;
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/01
  */
std::string ChatMsgManager::getLocalHeadPath(const std::string &netHeadPath)
{
    NetHeadImgEvt e;
    e.netFilePath = netHeadPath;
    EventBus::FireEvent(e);
    return e.localFilePath;
}

/**
  * @函数名   getUserHistoryMessage
  * @功能描述 获取某人小于给定时间的消息
  * @参数
  * @author   cc
  * @date     2018/09/28
  */
VectorMessage ChatMsgManager::getUserHistoryMessage(const QInt64 &time, const QUInt8 &chatType, const QTalk::Entity::UID &uid)
{
    HistoryMessage e;
    e.time = time;
    e.uid = uid;
    e.chatType = chatType;
    EventBus::FireEvent(e);

    return e.msgList;
}

/**
 * 本地历史记录
 * @param time
 * @param uid
 * @return
 */
VectorMessage ChatMsgManager::getUserLocalHistoryMessage(const QInt64 &time, const QTalk::Entity::UID &uid)
{
    LocalHistoryMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    EventBus::FireEvent(e);

    return e.msgList;
}

/**
  * @函数名   getGroupInfo
  * @功能描述 获取群资料 群成员
  * @参数
  * @author   cc
  * @date     2018/10/01
  */
void ChatMsgManager::getGroupInfo(const std::string &groupId)
{
    GetGroupMessage e;
    e.groupId = groupId;
    EventBus::FireEvent(e);
}

/**
  * @函数名   
  * @功能描述 发送message
  * @参数
  * @author   cc
  * @date     2018/10/15
  */
void ChatMsgManager::sendMessage(S_Message &e)
{

    g_pMainPanel->addSending(e.message.MsgId);

    QT_CONCURRENT_FUNC([e]() {
        S_Message ne(e);
        EventBus::FireEvent(ne);
    });
}

void ChatMsgManager::preSendMessage(const QTalk::Entity::ImMessageInfo &message)
{
    g_pMainPanel->addSending(message.MsgId);

    PreSendMessageEvt evt;
    evt.message = message;
    EventBus::FireEvent(evt);
}

/**
  * @函数名
  * @功能描述 下载文件
  * @参数
  * @date 2018.10.22
  */
void ChatMsgManager::sendDownLoadFile(const std::string &strLocalPath, const std::string &strUri, const std::string &processKey)
{
    DownloadFileWithProcess event;
    event.strLocalPath = strLocalPath;
    event.strUri = strUri;
    event.processKey = processKey;
    EventBus::FireEvent(event);
}

//
void ChatMsgManager::sendRevokeMessage(const QTalk::Entity::UID &uid, const std::string &from, const std::string &messageId, const QInt8 &chatType)
{
    S_RevokeMessage e(uid, from, messageId, chatType);
    EventBus::FireEvent(e);
}

void ChatMsgManager::setUserSetting(bool isSetting, const std::string &key, const std::string &subKey,
                                    const std::string &val)
{

    UserSettingMsg e;
    e.operatorType = isSetting ? UserSettingMsg::EM_OPERATOR_CANCEL : UserSettingMsg::EM_OPERATOR_SET;
    e.key = key;
    e.subKey = subKey;
    e.value = val;
    EventBus::FireEvent(e);
}

/**
 *
 * @param logPath
 */
void ChatMsgManager::sendLogReport(const std::string &desc, const std::string &logPath)
{
    LogReportMessage e(desc, logPath);
    EventBus::FireEvent(e);
}

void ChatMsgManager::setGroupAdmin(const std::string &groupId, const std::string &nick, const std::string &memberJid, bool isAdmin)
{
    SetGroupAdmin e;
    e.groupId = groupId;
    e.nick = nick;
    e.memberJid = memberJid;
    e.isAdmin = isAdmin;
    EventBus::FireEvent(e);
}

void ChatMsgManager::removeGroupMember(const std::string &groupId, const std::string &nick, const std::string &memberJid)
{
    RemoveGroupMember e;
    e.groupId = groupId;
    e.nick = nick;
    e.memberJid = memberJid;
    EventBus::FireEvent(e);
}

void ChatMsgManager::getUserInfo(std::shared_ptr<QTalk::Entity::ImUserInfo> &info)
{
    UserCardInfo e(info);
    EventBus::FireEvent(e);
}

std::string ChatMsgManager::uploadFile(const std::string &localFile, bool withprocess, const std::string &processKey)
{
    UploadFileEvt e(localFile, withprocess, processKey);
    EventBus::FireEvent(e);
    return e.fileUrl;
}

void ChatMsgManager::getRecentSession(std::vector<QTalk::StShareSession> &ss)
{
    RecentSessionEvt e(ss);
    EventBus::FireEvent(e);
}

void ChatMsgManager::getContactsSession(std::vector<QTalk::StShareSession> &ss)
{
    ContactsSessionEvt e(ss);
    EventBus::FireEvent(e);
}

/**
 *
 * @param messsageId
 * @param users
 */
void ChatMsgManager::forwardMesssage(const std::string &messsageId, const std::map<std::string, int> &users)
{
    ForwardMessage e(messsageId, users);
    EventBus::FireEvent(e);
}

void ChatMsgManager::addGroupMember(const std::vector<std::string> &members, const std::string &groupId)
{
    AddGroupMember e(members, groupId);
    EventBus::FireEvent(e);
}

void ChatMsgManager::serverCloseSession(const std::string &username, const std::string &virtualname)
{
    ServerCloseSessionEvt e(username, virtualname);
    EventBus::FireEvent(e);
}

void ChatMsgManager::getSeatList(const QTalk::Entity::UID &uid)
{
    GetSeatListEvt e(uid);
    EventBus::FireEvent(e);
}

void ChatMsgManager::sendProduct(const std::string &userQName, const std::string &virtualId, const std::string &product, const std::string &type)
{
    SendProductEvt e(userQName, virtualId, product, type);
    EventBus::FireEvent(e);
}

void ChatMsgManager::sessionTransfer(const QTalk::Entity::UID &uid, const std::string &newCsrName,
                                     const std::string &reason)
{
    SessionTransferEvt e(uid, newCsrName, reason);
    EventBus::FireEvent(e);
}

void ChatMsgManager::sendWechat(const QTalk::Entity::UID &uid)
{
    SendWechatEvt e(uid);
    EventBus::FireEvent(e);
}

void ChatMsgManager::getQuickGroups(std::vector<QTalk::Entity::ImQRgroup> &groups)
{
    QuickGroupEvt evt(groups);
    EventBus::FireEvent(evt);
}

void ChatMsgManager::getQuickContentByGroup(std::vector<QTalk::Entity::IMQRContent> &contents, int id)
{
    QuickContentByGroupEvt evt(contents, id);
    EventBus::FireEvent(evt);
}

VectorMessage ChatMsgManager::getUserFileHistoryMessage(const QInt64 &time, const QTalk::Entity::UID &uid)
{

    FileHistoryMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    EventBus::FireEvent(e);

    return e.msgList;
}

VectorMessage ChatMsgManager::getUserImageHistoryMessage(const QInt64 &time, const QTalk::Entity::UID &uid)
{

    ImageHistoryMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    EventBus::FireEvent(e);

    return e.msgList;
}

VectorMessage ChatMsgManager::getUserLinkHistoryMessage(const QInt64 &time, const QTalk::Entity::UID &uid)
{

    LinkHistoryMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    EventBus::FireEvent(e);

    return e.msgList;
}

VectorMessage
ChatMsgManager::getSearchMessage(const QInt64 &time, const QTalk::Entity::UID &uid, const std::string &text)
{

    SearchHistoryMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    e.searchKey = text;
    EventBus::FireEvent(e);

    return e.msgList;
}

VectorMessage
ChatMsgManager::getAfterMessage(const QInt64 &time, const QTalk::Entity::UID &uid)
{

    AfterMessage e;
    e.time = time;
    e.userid = uid.usrId();
    e.realJid = uid.realId();
    EventBus::FireEvent(e);

    return e.msgList;
}

void ChatMsgManager::hotLineMessageList(const std::string &xmppId)
{
    HotLineMessageListEvt e;
    e.xmppId = xmppId;
    EventBus::FireEvent(e);
}

void ChatMsgManager::postInterface(const std::string &url, const std::string &params)
{
    S_AddHttpQeq req;
    req.request.url = url;
    req.request.method = 1;
    req.request.body = params;
    req.request.header["Content-Type"] = "application/json;";

    req.callback = [](int code, const std::string &responseData) {
        if (code == 200)
        {
        }
        else
            error_log(responseData);
    };

    EventBus::FireEvent(req);
}

std::string ChatMsgManager::sendGetRequest(const std::string &url)
{
    S_AddHttpQeq req;
    req.request.url = url;
    req.request.method = 2;

    int c = 0;
    std::string d;
    req.callback = [&c, &d](int code, const std::string &responseData) {
        c = code;
        d = responseData;
        if (code == 200)
        {
        }
        else
            error_log(responseData);
    };

    EventBus::FireEvent(req);
    if (c == 200)
        return d;
    else
        return "";
}

void ChatMsgManager::updateMessageExtendInfo(const std::string &msgId, const std::string &info)
{

    UpdateMsgExtendInfo evt;
    evt.msgId = msgId;
    evt.extendInfo = info;
    EventBus::FireEvent(evt);
}

void ChatMsgManager::sendWebRtcCommand(int msgType, const std::string &json, const std::string &id)
{

    SWebRtcCommand e;
    e.msgType = msgType;
    e.cmd = json;
    e.jid = id;
    EventBus::FireEvent(e);
}

void ChatMsgManager::getUserMedal(const std::string &xmppId, std::set<QTalk::StUserMedal> &medal)
{
    UserMedalEvt e(xmppId, medal);
    EventBus::FireEvent(e);
}

void ChatMsgManager::sendSearch(SearchInfoEvent &event)
{
    EventBus::FireEvent(event);
}

VectorMessage
ChatMsgManager::getNetHistoryMessage(const QInt64 &time,
                                     int chatType,
                                     const QTalk::Entity::UID &uid,
                                     int direction)
{

    NetHistoryMessage e;
    e.time = time;
    e.uid = uid;
    e.chatType = chatType;
    e.direction = std::to_string(direction);
    EventBus::FireEvent(e);

    return e.msgList;
}

//
void ChatMsgManager::updateGroupTopic(const std::string &groupId, const std::string &groupTopic)
{
    UpdateGroupTopicEvt e;
    e.groupId = groupId;
    e.groupTopic = groupTopic;
    EventBus::FireEvent(e);
}

/*******************************/
ChatMsgListener::ChatMsgListener()
{
    EventBus::AddHandler<R_Message>(*this);
    EventBus::AddHandler<GroupMemberMessage>(*this);
    EventBus::AddHandler<UserCardMessgae>(*this);
    EventBus::AddHandler<GroupTopicEvt>(*this);
    EventBus::AddHandler<UpdateOnlineEvent>(*this);
    EventBus::AddHandler<FileProcessMessage>(*this);
    EventBus::AddHandler<DisconnectToServer>(*this);
    EventBus::AddHandler<LoginSuccessMessage>(*this);
    EventBus::AddHandler<UpdateGroupMember>(*this);
    EventBus::AddHandler<SignalReadState>(*this);
    EventBus::AddHandler<RevokeMessage>(*this);
    EventBus::AddHandler<UpdateUserConfigMsg>(*this);
    EventBus::AddHandler<DestroyGroupRet>(*this);
    EventBus::AddHandler<R_BlackListMessage>(*this);
    EventBus::AddHandler<UpdateGroupInfoRet>(*this);
    EventBus::AddHandler<RemoveGroupMemberRet>(*this);
    EventBus::AddHandler<LogReportMessageRet>(*this);
    EventBus::AddHandler<GetUsersOnlineSucessEvent>(*this);
    EventBus::AddHandler<RecvVideoMessage>(*this);
    EventBus::AddHandler<GroupMemberChangeRet>(*this);
    EventBus::AddHandler<ChangeHeadRetMessage>(*this);
    EventBus::AddHandler<UpdateMoodRet>(*this);
    EventBus::AddHandler<FeedBackLogEvt>(*this);
    EventBus::AddHandler<IncrementConfig>(*this);
    EventBus::AddHandler<MStateEvt>(*this);
    EventBus::AddHandler<WebRtcCommand>(*this);
    EventBus::AddHandler<UserMedalChangedEvt>(*this);
    EventBus::AddHandler<LoginProcessMessage>(*this);
}

ChatMsgListener::~ChatMsgListener()
{
}

void ChatMsgListener::onEvent(R_Message &e)
{
    if (e.getCanceled())
    {
        return;
    }
    if (g_pMainPanel)
    {
        g_pMainPanel->onRecvMessage(e, e.isCarbon);
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/08
  */
void ChatMsgListener::onEvent(GroupMemberMessage &e)
{
    if (e.getCanceled())
    {
        return;
    }
    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateGroupMember(e);
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/08
  */
void ChatMsgListener::onEvent(UserCardMessgae &e)
{
    if (e.getCanceled())
    {
        return;
    }
    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateUserHeadPath(e.userCards);
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数     GroupTopicEvt 群公告
     void
  * @author   cc
  * @date     2018/10/12
  */
void ChatMsgListener::onEvent(GroupTopicEvt &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateGroupTopic(e.groupId, e.groupTopic);
    }
}

/**
  * @函数名   
  * @功能描述 更新在线人数 头像
  * @参数
  * @author   cc
  * @date     2018/10/15
  */
void ChatMsgListener::onEvent(UpdateOnlineEvent &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateHead();
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/19
  */
void ChatMsgListener::onEvent(FileProcessMessage &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->onRecvFileProcessMessage(e);
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/25
  */
void ChatMsgListener::onEvent(DisconnectToServer &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        // 连接状态
        g_pMainPanel->setConnectStatus(false);
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/25
  */
void ChatMsgListener::onEvent(LoginSuccessMessage &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->setConnectStatus(true);
    }
}

/**
  * @函数名   
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/26
  */
void ChatMsgListener::onEvent(UpdateGroupMember &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateGroupMemberInfo(e.groupId, e.userCards);
    }
}

void ChatMsgListener::onEvent(SignalReadState &e)
{

    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->gotReadState(QTalk::Entity::UID(e.userId, e.realJid), e.mapReadState);
    }
}

void ChatMsgListener::onEvent(MStateEvt &e)
{
    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->gotMState(QTalk::Entity::UID(e.userId, e.realJid), e.messageId, e.time);
    }
}

void ChatMsgListener::onEvent(RevokeMessage &e)
{
    if (e.getCanceled())
        return;

    if (nullptr != g_pMainPanel)
    {
        g_pMainPanel->updateRevokeMessage(e.uid, e.messageFrom, e.messageId, e.time);
    }
}

void ChatMsgListener::onEvent(UpdateUserConfigMsg &e)
{
    if (e.getCanceled())
        return;
    if (g_pMainPanel)
    {
        g_pMainPanel->updateUserConfig(e.arConfigs);
    }
}

void ChatMsgListener::onEvent(DestroyGroupRet &e)
{
    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->onDestroyGroup(e.groupId, e.isDestroy);
    }
}

/**
 * 黑名单反馈消息
 * @param e
 */
void ChatMsgListener::onEvent(R_BlackListMessage &e)
{
    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->recvBlackMessage(e.messageFrom, e.messageId);
    }
}

void ChatMsgListener::onEvent(UpdateGroupInfoRet &e)
{
    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->updateGroupInfo(e.groupinfo);
    }
}

/**
 *
 * @param e
 */
void ChatMsgListener::onEvent(RemoveGroupMemberRet &e)
{
    if (e.getCanceled())
        return;
    ;
    if (g_pMainPanel)
    {
        g_pMainPanel->onRemoveGroupMember(e.groupId, e.memberId);
    }
}

void ChatMsgListener::onEvent(LogReportMessageRet &e)
{
    if (e.getCanceled())
        return;
    ;
    if (g_pMainPanel)
    {
        g_pMainPanel->onLogReportMessageRet(e._success, e._strLogRetMsg);
    }
}

void ChatMsgListener::onEvent(GetUsersOnlineSucessEvent &e)
{
    if (e.getCanceled())
        return;
    ;
    if (g_pMainPanel)
    {
        g_pMainPanel->recvUserStatus(e._user, e._status);
    }
}

/**
 *
 * @param e
 */
void ChatMsgListener::onEvent(RecvVideoMessage &e)
{
    if (e.getCanceled())
        return;
    //    if (g_pMainPanel) {
    //        g_pMainPanel->onRecvVideo(e._userId);
    //    }
}

void ChatMsgListener::onEvent(GroupMemberChangeRet &e)
{
    if (e.getCanceled())
        return;
    if (g_pMainPanel)
    {
        if (e.isAdd)
        {
            g_pMainPanel->onRecvAddGroupMember(e.groupId, e.memberJid, e.nick, e.affiliation);
        }
        else if (e.isRemove)
        {
            g_pMainPanel->onRecvRemoveGroupMember(e.groupId, e.memberJid);
        }
    }
}

/**
 *
 * @param e
 */
void ChatMsgListener::onEvent(ChangeHeadRetMessage &e)
{
    if (e.getCanceled())
        return;

    if (e.ret && g_pMainPanel)
    {
        g_pMainPanel->onChangeHeadSuccess(e.localHead);
    }
}

void ChatMsgListener::onEvent(UpdateMoodRet &e)
{
    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->onUpdateMoodSuccess(e.userId, e.mood);
    }
}

void ChatMsgListener::onEvent(FeedBackLogEvt &e)
{
    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->packAndSendLog(QString::fromStdString(e.text));
    }
}

void ChatMsgListener::onEvent(IncrementConfig &e)
{

    if (e.getCanceled())
        return;

    if (g_pMainPanel)
    {
        g_pMainPanel->updateUserConfig(e.deleteData, e.arImConfig);
    }
}

void ChatMsgListener::onEvent(WebRtcCommand &e)
{
    if (g_pMainPanel)
        g_pMainPanel->onRecvWebRtcCommand(e.msgType, e.jid, e.cmd, e.isCarbon);
}

void ChatMsgListener::onEvent(UserMedalChangedEvt &e)
{
    if (g_pMainPanel)
        g_pMainPanel->onUserMadelChanged(e.userMedals);
}

void ChatMsgListener::onEvent(LoginProcessMessage &e)
{
    if (g_pMainPanel)
        g_pMainPanel->onRecvLoginProcessMessage(e.message.data());
}
