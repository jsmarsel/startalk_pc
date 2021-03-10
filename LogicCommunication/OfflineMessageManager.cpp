#include "OfflineMessageManager.h"
#include "../Platform/Platform.h"
#include "../Platform/NavigationManager.h"
#include "../entity/im_message.h"
#include "../entity/IM_Session.h"
#include "../LogicManager/LogicManager.h"
#include "../interface/logic/IDatabasePlug.h"
#include "Communication.h"
#include "../QtUtil/Utils/Log.h"
#include "../QtUtil/nJson/nJson.h"
#include <time.h>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

vector<string> split(const string &strtem, char a)
{
    vector<string> strvec;
    string::size_type pos1, pos2;
    pos2 = strtem.find(a);
    pos1 = 0;

    while (string::npos != pos2)
    {
        strvec.push_back(strtem.substr(pos1, pos2 - pos1));
        pos1 = pos2 + 1;
        pos2 = strtem.find(a, pos1);
    }

    strvec.push_back(strtem.substr(pos1));
    return strvec;
}

OfflineMessageManager::OfflineMessageManager(Communication *comm)
    : _pComm(comm)
{
}

bool OfflineMessageManager::updateChatOfflineMessage()
{
    int pageCount = 500;
    int retryCount = 3;
    bool complete = false;
    std::string errMsg;
    QInt64 timeStamp = getTimeStamp(QTalk::Enum::TwoPersonChat);
    int index = 0;

    while (true)
    {
        std::vector<QTalk::Entity::ImMessageInfo> msgList;
        std::vector<QTalk::Entity::ImSessionInfo> sessionList;
        getOfflineChatMessageJson(timeStamp, pageCount, complete, errMsg, msgList, sessionList);

        if (complete)
        {
            if (!msgList.empty())
            {
                LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
                QTalk::Entity::ImMessageInfo lastMsgInfo = msgList.back();
                timeStamp = lastMsgInfo.LastUpdateTime;
                retryCount = 3;
            }
            else
            {
                // 获取消息完成
                break;
            }
        }
        else
        {
            // 获取消息失败重试
            if (retryCount < 0)
            {
                error_log("pull history failed and retry failed");
                break;
            }
            else
            {
                retryCount--;
                warn_log("pull chat history failed: retry {0} times", retryCount);
            }
        }

        if(_pComm)
        {
            std::string msg = SFormat("getting user message: {0}", ++index);
            CommMsgManager::sendLoginProcessMessage(msg);
        }
    }

    // 成功才更新时间戳
    return retryCount >= 0;
}

void OfflineMessageManager::updateChatMasks()
{
    using namespace QTalk;
    QInt64 timeStamp = getTimeStamp(QTalk::Enum::TwoPersonChat);
    //
    std::ostringstream url;
    url << NavigationManager::instance().getJavaHost()
        << "/qtapi/getreadflag.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();
    std::string strUrl = url.str();
    nJson obj;
    obj["time"] = timeStamp;
    obj["domain"] = PLAT.getSelfDomain();
    std::string postData = obj.dump();
    bool _ret = false;
    std::map<std::string, int> readFlags;
    auto callBack = [&_ret, &readFlags](int code, const std::string & responseData)
    {
        info_log("{0}  {1}", code, responseData);

        if (code == 200)
        {
            nJson obj = Json::parse(responseData);

            if (obj == nullptr)
            {
                error_log("json paring error");
                return;
            }

            //
            bool ret = Json::get<bool >(obj, "ret");

            if(ret)
            {
                nJson data = Json::get<nJson >(obj, "data");

                for( auto &tempMsg : data)
                {
                    auto flag = Json::get<int >(tempMsg, "readflag");
                    std::string msgId = Json::get<std::string >(tempMsg, "msgid");
                    readFlags[msgId] = flag;
                }
            }

            //
            _ret = true;
        }
        else
        {
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", strUrl, postData);
        _pComm->addHttpRequest(req, callBack);

        //
        if(_ret)
        {
            // 更新阅读状态
            LogicManager::instance()->getDatabase()->updateMessageReadFlags(readFlags);
            info_log("-- got user message and read flag -> update timestamp");
            // 更新时间戳
            LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_TWOPERSONCHAT,
                    std::to_string(0));
        }
    }
}


bool OfflineMessageManager::updateGroupOfflineMessage()
{
    int pageCount = 500;
    int retryCount = 3;
    bool complete = false;
    std::string errMsg;
    QInt64 timeStamp = getTimeStamp(QTalk::Enum::GroupChat);
    int index = 0;

    while (true)
    {
        std::vector<QTalk::Entity::ImMessageInfo> msgList;
        std::vector<QTalk::Entity::ImSessionInfo> sessionList;
        getOfflineGroupMessageJson(timeStamp, pageCount, complete, errMsg, msgList, sessionList);

        if (complete)
        {
            if (!msgList.empty())
            {
                LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
                QTalk::Entity::ImMessageInfo lastMsgInfo = msgList.back();
                timeStamp = lastMsgInfo.LastUpdateTime;
                retryCount = 3;
            }
            else
            {
                // 获取消息完成
                break;
            }
        }
        else
        {
            // 获取消息失败重试
            if (retryCount < 0)
            {
                error_log("pull history failed and retry failed");
                break;
            }
            else
            {
                retryCount--;
                warn_log("pull group history failed: retry {0} times", retryCount);
            }
        }

        if(_pComm)
        {
            std::string msg = SFormat("getting group message: {0}", ++index);
            CommMsgManager::sendLoginProcessMessage(msg);
        }
    }

    // 成功才更新时间戳
    if(retryCount >= 0)
    {
        info_log("-- got group message -> update timestamp");
        LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_GROUPCHAT,
                std::to_string(0));
    }

    return retryCount >= 0;
}

bool OfflineMessageManager::updateNoticeOfflineMessage()
{
    int pageCount = 500;
    int retryCount = 3;
    bool complete = false;
    std::string errMsg;
    QInt64 timeStamp = getTimeStamp(QTalk::Enum::System);
    int index = 0;

    while (true)
    {
        std::vector<QTalk::Entity::ImMessageInfo> msgList;
        std::vector<QTalk::Entity::ImSessionInfo> sessionList;
        getOfflineNoticeMessageJson(timeStamp, pageCount, complete, errMsg, msgList, sessionList);

        if (complete)
        {
            if (!msgList.empty())
            {
                LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
                //LogicManager::instance()->GetDatabase()->bulkInsertSessionInfo(sessionList);
                QTalk::Entity::ImMessageInfo lastMsgInfo = msgList.back();
                timeStamp = lastMsgInfo.LastUpdateTime;
                retryCount = 3;
            }
            else
            {
                // 获取消息完成
                break;
            }
        }
        else
        {
            // 获取消息失败重试
            if (retryCount < 0)
            {
                error_log("pull history failed and retry failed");
                break;
            }
            else
            {
                retryCount--;
                warn_log("pull system history failed: retry {0} times", retryCount);
            }
        }

        if(_pComm)
        {
            std::string msg = SFormat("getting system message: {0}", ++index);
            CommMsgManager::sendLoginProcessMessage(msg);
        }
    }

    // 成功才更新时间戳
    if(retryCount >= 0)
    {
        info_log("-- got system message -> update timestamp");
        LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_SYSTEM, std::to_string(0));
    }

    return retryCount >= 0;
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/26
  */
QInt64 OfflineMessageManager::getTimeStamp(QTalk::Enum::ChatType chatType)
{
    QInt64 timeStamp = 0;
    std::string subKey;

    switch (chatType)
    {
        case QTalk::Enum::TwoPersonChat:
        case QTalk::Enum::Consult:
        case QTalk::Enum::ConsultServer:
        case QTalk::Enum::Robot:
        case QTalk::Enum::Collection:
            subKey = DEM_TWOPERSONCHAT;
            break;

        case QTalk::Enum::GroupChat:
            subKey = DEM_GROUPCHAT;
            break;

        case QTalk::Enum::System:
            subKey = DEM_SYSTEM;
            break;

        default:
            break;
    }

    std::string strTime;

    if (LogicManager::instance()->getDatabase()->getConfig(DEM_MESSAGE_MAXTIMESTAMP, subKey, strTime))
        timeStamp = atoll(strTime.data());

    debug_log("getconfig time {0}", timeStamp);

//    if (timeStamp <= 0) {
//        timeStamp = LogicManager::instance()->GetDatabase()->getMaxTimeStampByChatType(chatType);
//    }
//    debug_log("getMaxTimeStampByChatType time{0}", timeStamp);
    if (timeStamp <= 0)
    {
        time_t now = time(0);
        timeStamp = (now - PLAT.getServerDiffTime() - 3600 * 48) * 1000;
    }

    debug_log("get now time{0}", timeStamp);
    return timeStamp;
}

void OfflineMessageManager::updateGroupMasks()
{
    try
    {
        std::map<std::string, QInt64> readMarkList;
        getGroupReadMark(readMarkList);
        bool ret = LogicManager::instance()->getDatabase()->updateReadMask(readMarkList);

        if (ret)
        {
            //登录后阅读指针更新成功将时间至0
            LogicManager::instance()->getDatabase()->saveLoginBeforeGroupReadMarkTime("0");
        }
    }
    catch (std::exception &e)
    {
        error_log("updateGroupMasks exception {0}", e.what());
    }
}

void OfflineMessageManager::getOfflineChatMessageJson(long long chatTimestamp, int count, bool &complete,
        std::string &errMsg,
        std::vector<QTalk::Entity::ImMessageInfo> &outMsgList,
        std::vector<QTalk::Entity::ImSessionInfo> &outSessionList)
{
    debug_log("开始获取单人消息");
    std::string httpHost = NavigationManager::instance().getJavaHost();
    std::string method = "/qtapi/gethistory.qunar";
    std::string params = "server=" + PLAT.getSelfDomain()
                         + "&c=qtalk&u=" + PLAT.getSelfUserId()
                         + "&p=" + PLAT.getPlatformStr()
                         + "&v=" + PLAT.getClientVersion();
    std::string selfJid = PLAT.getSelfUserId();
    std::string url = httpHost + method + "?" + params;
    nJson obj;
    obj["user"] = PLAT.getSelfUserId();
    obj["domain"] = PLAT.getSelfDomain();
    obj["host"] = PLAT.getSelfDomain();
    obj["time"] = chatTimestamp;
    obj["num"] = count;
    obj["f"] = "t";
    std::string postData = obj.dump();
    //
    auto callback = [ selfJid, &complete, &errMsg, &outMsgList, &outSessionList]
                    (int code, const std::string & responseData)
    {
        info_log("{0}  {1}", code, responseData);
        std::map<std::string, QTalk::Entity::ImSessionInfo> sessionMap;

        if (code == 200)
        {
            nJson obj = Json::parse(responseData);

            if (obj == nullptr)
            {
                error_log("json paring error");
                return;
            }

            int ret = Json::get<bool >(obj, "ret");

            if (ret)
            {
                nJson msgList = Json::get<nJson >(obj, "data");
                complete = true;
                
                if(!msgList.is_array())
                {
                    complete = false;
                    return;
                }
                
                for (auto &item : msgList)
                {
                    std::string from = Json::get<std::string>(item, "from");
                    std::string fromDomain = Json::get<std::string>(item, "from_host");
                    std::string fromJid = from + "@" + fromDomain;
                    std::string to = Json::get<std::string>(item, "to");
                    std::string toDomain = Json::get<std::string>(item, "to_host");
                    std::string toJid = to + "@" + toDomain;
                    int readFlag = Json::get<int>(item, "read_flag");
                    std::string xmppId;
                    std::string realJid;
                    int chatType = QTalk::Enum::TwoPersonChat;

                    if (item.contains("body") && item.contains("message"))
                    {
                        nJson message = Json::get<nJson >(item, "message");
                        nJson body    = Json::get<nJson >(item, "body");
                        std::string type = Json::get<std::string>(message, "type");
                        std::string msgId = Json::get<std::string>(body, "id");

                        if (msgId.empty())
                            continue;

                        std::string chatId = Json::get<std::string>(message, "qchatid");

                        if (chatId.empty())
                            chatId = Json::get<std::string>(message, "qchatid");

                        if (chatId.empty())
                            chatId = "4";

                        bool isConsult = false;
                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        std::string channelInfo = Json::get<std::string>(message, "channelid");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");

                        if (type == "consult")
                            isConsult = true;
                        else if (type != "chat" && type != "revoke" && type != "subscription")
                            continue;

                        if (type == "subscription")
                            chatType = QTalk::Enum::Robot;

                        int direction = 0;

                        if (fromJid == (selfJid + "@" + fromDomain))
                        {
                            if (isConsult)
                            {
                                xmppId = toJid;

                                if (chatId == "4")
                                {
                                    realJid = toJid;
                                    chatType = QTalk::Enum::Consult;
                                }
                                else
                                {
                                    std::string realTo = Json::get<std::string>(message, "realto");
                                    realJid = split(realTo, '/').front();
                                    chatType = QTalk::Enum::ConsultServer;
                                }
                            }
                            else
                            {
                                xmppId = toJid;
                                realJid = toJid;
                            }

                            direction = 1;

                            if (msgType == 1003 || msgType == 1004 || msgType == 1002 || msgType == 1001)
                                continue;
                        }
                        else
                        {
                            direction = 0;

                            if (isConsult)
                            {
                                xmppId = fromJid;

                                if (chatId == "4")
                                {
                                    std::string realfrom = Json::get<std::string>(message, "realfrom");
                                    realJid = split(realfrom, '/').front();
                                    chatType = QTalk::Enum::ConsultServer;
                                }
                                else
                                {
                                    realJid = fromJid;
                                    chatType = QTalk::Enum::Consult;
                                }
                            }
                            else
                            {
                                xmppId = fromJid;
                                realJid = fromJid;
                            }

                            if (msgType == 1004 || msgType == 1003 || msgType == 1002 || msgType == 1001)
                                continue;
                        }

                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = direction;
                        msgInfo.From = fromJid;
                        msgInfo.To = toJid;
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1; // 从服务器拉回来的消息 -> 消息已送达服务器
                        msgInfo.ReadedTag = readFlag;
                        msgInfo.LastUpdateTime = msec_times;
                        outMsgList.push_back(msgInfo);
                        std::string sessionKey = xmppId + "-" + realJid;
                        auto iter = sessionMap.find(sessionKey);

                        if (sessionMap.end() != iter)
                        {
                            QTalk::Entity::ImSessionInfo sessionInfo = iter->second;

                            if (msec_times > sessionInfo.LastUpdateTime)
                            {
                                sessionInfo.LastMessageId = msgId;
                                sessionInfo.LastUpdateTime = msec_times;
                                sessionMap[sessionKey] = sessionInfo;
                            }
                        }
                        else
                        {
                            QTalk::Entity::ImSessionInfo sessionInfo;
                            sessionInfo.XmppId = xmppId;
                            sessionInfo.RealJid = realJid;
                            sessionInfo.ChatType = chatType;
                            sessionInfo.LastMessageId = msgId;
                            sessionInfo.UnreadCount = 0;
                            sessionInfo.LastUpdateTime = msec_times;
                            sessionInfo.MessageState = 0;
                            sessionMap.insert(
                                map<std::string, QTalk::Entity::ImSessionInfo>::value_type(sessionKey,
                                        sessionInfo));
//                        outSessionList->push_back(sessionInfo);
                        }
                    }
                }

                //组装会话列表
                auto iter = sessionMap.begin();

                for (; iter != sessionMap.end(); ++iter)
                {
                    QTalk::Entity::ImSessionInfo sessionInfo = iter->second;
                    outSessionList.push_back(sessionInfo);
                }
            }
            else
            {
                complete = false;
                errMsg = Json::get<std::string>(obj, "errmsg");
            }
        }
        else
        {
            complete = false;
            errMsg = "请求失败";
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(url, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", url, postData);
        _pComm->addHttpRequest(req, callback);
    }
}

void OfflineMessageManager::getGroupReadMark(std::map<std::string, QInt64> &readMarkList)
{
    std::string httpHost = NavigationManager::instance().getJavaHost();
    std::string method = "/qtapi/get_muc_readmark1.qunar";
    std::string params = "server=" + PLAT.getSelfDomain()
                         + "&c=qtalk&u=" + PLAT.getSelfUserId()
                         + "&p=" + PLAT.getPlatformStr()
                         + "&v=" + PLAT.getClientVersion();
    std::string timeStamp = LogicManager::instance()->getDatabase()->getLoginBeforeGroupReadMarkTime();
    timeStamp = std::to_string(std::strtoll(timeStamp.data(), nullptr, 0));
    //
    std::string selfJid = PLAT.getSelfUserId() + "@" + PLAT.getSelfDomain();
    std::string url = httpHost + method + "?" + params;
    nJson  obj;
    obj["user"] = PLAT.getSelfUserId();
    obj["host"] = PLAT.getSelfDomain();
    obj["time"] = timeStamp;
    std::string postData = obj.dump();
    std::map<std::string, QTalk::Entity::ImSessionInfo> sessionMap;
    auto callback = [ url, &readMarkList](int code, const string & responseData)
    {
        info_log("{0}  {1}", code, responseData);

        if (code == 200)
        {
            nJson obj = Json::parse(responseData);

            if (obj == nullptr)
            {
                error_log("json paring error");
                return;
            }

            bool ret = Json::get<bool >(obj, "ret");

            if (ret)
            {
                nJson data = Json::get<nJson >(obj, "data");
                std::map<std::string, QInt64> readMark;

                for (auto &item : data)
                {
                    std::string mucName = Json::get<std::string>(item, "muc_name");
                    std::string domain = Json::get<std::string>(item, "domain");
                    QInt64 date = atoll(Json::get<std::string>(item, "date").data());
                    std::string groupId = mucName + "@" + domain;
                    readMarkList.insert(std::map<std::string, QInt64>::value_type(groupId, date));
                }
            }
            else
            {
                std::ostringstream os("error code is :");
                os << ret
                   << ", url is :"
                   << url;
                error_log(os.str());
            }
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(url, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", url, postData);
        _pComm->addHttpRequest(req, callback);
    }
}

void OfflineMessageManager::getOfflineGroupMessageJson(long long chatTimestamp, int count, bool &complete,
        std::string &errMsg,
        std::vector<QTalk::Entity::ImMessageInfo> &outMsgList,
        std::vector<QTalk::Entity::ImSessionInfo> &outSessionList)
{
    try
    {
        std::string httpHost = NavigationManager::instance().getJavaHost();
        std::string method = "/qtapi/getmuchistory.qunar";
        std::string params = "server=" + PLAT.getSelfDomain()
                             + "&c=qtalk&u=" + PLAT.getSelfUserId()
                             //                       + "&k=" + Platform::GetPlatform()->getServerAuthKey()
                             + "&p=" + PLAT.getPlatformStr()
                             + "&v=" + PLAT.getClientVersion();
        std::string selfJid = PLAT.getSelfUserId() + "@" + PLAT.getSelfDomain();
        std::string httpUrl = httpHost + method + "?" + params;
        nJson obj;
        obj["user"] = PLAT.getSelfUserId();
        obj["domain"] = PLAT.getSelfDomain();
        obj["host"] = PLAT.getSelfDomain();
        obj["time"] = chatTimestamp;
        obj["num"] = count;
        obj["f"] = "t";
        std::string postData = obj.dump();
        auto callback = [ selfJid, &complete, &errMsg, &outMsgList, &outSessionList]
                        (int code, const string & responseData)
        {
            info_log("{0}  {1}", code, responseData);
            std::map<std::string, QTalk::Entity::ImSessionInfo> sessionMap;

            if (code == 200)
            {
                nJson obj = Json::parse(responseData);

                if (obj == nullptr)
                {
                    error_log("json paring error");
                    return;
                }

                bool ret = Json::get<bool >(obj, "ret");

                if (ret)
                {
                    complete = true;
                    nJson msgList = Json::get<nJson >(obj, "data");

                    for (auto &item : msgList)
                    {
                        std::string nickName = Json::get<std::string>(item, "nick");
                        int chatType = QTalk::Enum::GroupChat;
                        nJson message = Json::get<nJson >(item, "message");
                        std::string xmppId = Json::get<std::string>(message, "to");

                        if (xmppId.empty())
                            continue;

                        const std::string &realJid = xmppId;

                        if (item.contains("body"))
                        {
                            nJson body = Json::get<nJson >(item, "body");
                            std::string msgId = Json::get<std::string>(body, "id");

                            if (msgId.empty())
                                continue;

                            QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                            std::string msg = Json::get<std::string>(body, "content");
                            int platform = atoi(Json::get<std::string>(body, "maType").data());
                            int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                            std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                            std::string backupinfo = Json::get<std::string>(body, "backupinfo");
                            std::string realFrom = Json::get<std::string>(message, "sendjid");
                            int direction = 0;
                            int readState = 0;

                            if (realFrom == selfJid)
                            {
                                direction = 1;
                                readState = 1;//已发送
                            }
                            else
                            {
                                direction = 0;
                                readState = 0;//未读
                            }

                            QTalk::Entity::ImMessageInfo msgInfo;
                            msgInfo.MsgId = msgId;
                            msgInfo.XmppId = xmppId;
                            msgInfo.RealJid = realJid;
                            msgInfo.ChatType = chatType;
                            msgInfo.Direction = direction;
                            msgInfo.From = realFrom;
                            msgInfo.To = "";
                            msgInfo.Content = msg;
                            msgInfo.ExtendedInfo = extendInfo;
                            msgInfo.Platform = platform;
                            msgInfo.Type = msgType;
                            msgInfo.State = 1;
                            msgInfo.ReadedTag = readState;
                            msgInfo.LastUpdateTime = msec_times;
                            msgInfo.BackupInfo = backupinfo;
                            outMsgList.push_back(msgInfo);
                            std::string sessionKey = xmppId + "-" + realJid;
                            auto iter = sessionMap.find(
                                            sessionKey);

                            if (sessionMap.end() != iter)
                            {
                                QTalk::Entity::ImSessionInfo sessionInfo = iter->second;

                                if (msec_times > sessionInfo.LastUpdateTime)
                                {
                                    sessionInfo.LastMessageId = msgId;
                                    sessionInfo.LastUpdateTime = msec_times;
                                    sessionMap[sessionKey] = sessionInfo;
                                }
                            }
                            else
                            {
                                QTalk::Entity::ImSessionInfo sessionInfo;
                                sessionInfo.XmppId = xmppId;
                                sessionInfo.RealJid = realJid;
                                sessionInfo.ChatType = chatType;
                                sessionInfo.LastMessageId = msgId;
                                sessionInfo.UnreadCount = 0;
                                sessionInfo.LastUpdateTime = msec_times;
                                sessionInfo.MessageState = 0;
                                sessionMap.insert(map<std::string, QTalk::Entity::ImSessionInfo>::value_type(sessionKey,
                                                  sessionInfo));
//                        outSessionList->push_back(sessionInfo);
                            }
                        }
                    }

                    //组装会话列表
                    auto iter = sessionMap.begin();

                    for (; iter != sessionMap.end(); ++iter)
                    {
                        QTalk::Entity::ImSessionInfo sessionInfo = iter->second;
                        outSessionList.push_back(sessionInfo);
                    }
                }
                else
                {
                    complete = false;
                    errMsg = Json::get<nJson >(obj, "errmsg");
                    error_log(errMsg);
                    return;
                }
            }
            else
            {
                complete = false;
                errMsg = "请求失败";
            }
        };

        if (_pComm)
        {
            QTalk::HttpRequest req(httpUrl, QTalk::RequestMethod::POST);
            req.header["Content-Type"] = "application/json;";
            req.body = postData;
            info_log("{0} \n {1}", httpUrl, postData);
            _pComm->addHttpRequest(req, callback);
        }
    }
    catch (std::exception &e)
    {
        std::string what = e.what();
        warn_log(what);
    }
}

void OfflineMessageManager::getOfflineNoticeMessageJson(long long noticeTimestamp, int count, bool &complete,
        std::string &errMsg,
        std::vector<QTalk::Entity::ImMessageInfo> &outMsgList,
        std::vector<QTalk::Entity::ImSessionInfo> &outSessionList)
{
    std::string httpHost = NavigationManager::instance().getJavaHost();
    std::string method = "/qtapi/get_system_history.qunar";
    std::string params = "server=" + PLAT.getSelfDomain()
                         + "&c=qtalk&u=" + PLAT.getSelfUserId()
                         //                       + "&k=" + Platform::GetPlatform()->getServerAuthKey()
                         + "&p=" + PLAT.getPlatformStr()
                         + "&v=" + PLAT.getClientVersion();
    std::string selfJid = PLAT.getSelfUserId() + "@" + PLAT.getSelfDomain();
    std::string url = httpHost + method + "?" + params;
    nJson obj;
    obj["user"] = PLAT.getSelfUserId();
    obj["domain"] = PLAT.getSelfDomain();
    obj["host"] = PLAT.getSelfDomain();
    obj["time"] = noticeTimestamp;
    obj["num"] = count;
    obj["f"] = "t";
    std::string postData = obj.dump();
    auto callback = [ selfJid, &complete, &errMsg, &outMsgList, &outSessionList]
                    (int code, string responseData)
    {
        info_log("{0}  {1}", code, responseData);
        std::map<std::string, QTalk::Entity::ImSessionInfo> sessionMap;

        if (code == 200)
        {
            nJson data = Json::parse(responseData);

            if (data == nullptr)
            {
                error_log("json paring error");
                return;
            }

            bool ret = Json::get<bool >(data, "ret");

            if (ret)
            {
                complete = true;
                nJson msgList = Json::get<nJson >(data, "data");

//            std::cout << "msgList json" << cJSON_Print(msgList) << std::endl;
                for (auto &item : msgList)
                {
                    std::string nickName = Json::get<std::string>(item, "nick");
                    int chatType = QTalk::Enum::System;
                    nJson message = Json::get<nJson >(item, "message");
                    std::string xmppId = "SystemMessage@" + PLAT.getSelfDomain();
                    std::string type = Json::get<std::string>(message, "type");
                    const std::string &realJid = xmppId;

                    if (item.contains("body"))
                    {
                        nJson body = Json::get<nJson >(item, "body");
                        std::string msgId = Json::get<std::string>(body, "id");

                        if (msgId.empty())
                            continue;

                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
//                    int msgType = Json::get<int>(body,"msgType");
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");
                        std::string realFrom = Json::get<std::string>(message, "sendjid");
                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = 0;
                        msgInfo.From = realFrom;
                        msgInfo.To = "";
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1;
                        msgInfo.ReadedTag = 0;
                        msgInfo.LastUpdateTime = msec_times;
                        outMsgList.push_back(msgInfo);
                        std::string sessionKey = xmppId + "-" + realJid;
                        auto iter = sessionMap.find(sessionKey);

                        if (sessionMap.end() != iter)
                        {
                            QTalk::Entity::ImSessionInfo sessionInfo = iter->second;
                            sessionInfo.LastMessageId = msgId;
                            sessionInfo.LastUpdateTime = msec_times;
                        }
                        else
                        {
                            QTalk::Entity::ImSessionInfo sessionInfo;
                            sessionInfo.XmppId = xmppId;
                            sessionInfo.RealJid = realJid;
                            sessionInfo.ChatType = chatType;
                            sessionInfo.LastMessageId = msgId;
                            sessionInfo.UnreadCount = 0;
                            sessionInfo.LastUpdateTime = msec_times;
                            sessionInfo.MessageState = 0;
                            sessionMap.insert(
                                map<std::string, QTalk::Entity::ImSessionInfo>::value_type(sessionKey,
                                        sessionInfo));
                            outSessionList.push_back(sessionInfo);
                        }
                    }
                }
            }
            else
            {
                complete = false;
                errMsg = Json::get<std::string >(data, "errmsg");
            }
        }
        else
        {
            complete = false;
            errMsg = "请求失败";
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(url, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", url, postData);
        _pComm->addHttpRequest(req, callback);
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/12/13
  */
VectorMessage OfflineMessageManager::getUserMessage(const QInt64 &time, const std::string &userId,
        const std::string &direction, bool saveDb)
{
    //
    std::ostringstream url;
    url << NavigationManager::instance().getJavaHost()
        << "/qtapi/getmsgs.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();
    std::string strUrl = url.str();
    QTalk::Entity::JID jid(userId);
    nJson obj;
    obj["from"] = PLAT.getSelfUserId();
    obj["fhost"] = PLAT.getSelfDomain();
    obj["to"] = jid.username();
    obj["thost"] = jid.domainname();
    obj["direction"] = direction.data();
    obj["time"] = time;
    obj["domain"] = PLAT.getSelfDomain();
    obj["num"] = 20;
    obj["f"] = "t";
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImMessageInfo> msgList;
    auto callBack = [ userId, &msgList](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson resData = Json::parse(responseData);

            if (resData == nullptr)
            {
                error_log("json paring error");
                return;
            }

            bool ret = Json::get<bool >(resData, "ret");

            if (ret)
            {
                std::string selfJid = PLAT.getSelfUserId();
                nJson data = Json::get<nJson >(resData, "data");

                for (auto &item : data)
                {
                    std::string from = Json::get<std::string>(item, "from");
                    std::string fromDomain = Json::get<std::string>(item, "from_host");
                    std::string fromJid = from + "@" + fromDomain;
                    std::string to = Json::get<std::string>(item, "to");
                    std::string toDomain = Json::get<std::string>(item, "to_host");
                    std::string toJid = to + "@" + toDomain;
                    int readFlag = Json::get<int>(item, "read_flag");
                    std::string xmppId;
                    std::string realJid;
                    QUInt8 chatType = QTalk::Enum::TwoPersonChat;

                    if (item.contains("body") && item.contains("message"))
                    {
                        nJson message = Json::get<nJson >(item, "message");
                        nJson body = Json::get<nJson >(item, "body");
                        std::string type = Json::get<std::string>(message, "type");
                        std::string msgId = Json::get<std::string>(body, "id");

                        //
                        if (msgId.empty()) continue;

                        std::string chatId = Json::get<std::string>(message, "qchatid");

                        if (chatId.empty())
                            chatId = Json::get<std::string>(body, "qchatid");

                        if (chatId.empty())
                            chatId = "4";

                        bool isConsult = false;
                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        std::string channelInfo = Json::get<std::string>(message, "channelid");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");

                        if (type == "note")
                            msgType = -11;
                        else if (type == "consult")
                            isConsult = true;
                        else if (type != "chat" && type != "revoke" && type != "subscription")
                            continue;

                        if (type == "subscription")
                            chatType = QTalk::Enum::Robot;

                        int direction = 0;

                        if (fromJid == (selfJid + "@" + fromDomain))
                        {
                            if (isConsult)
                            {
                                xmppId = toJid;

                                if (chatId == "4")
                                {
                                    realJid = toJid;
                                    chatType = QTalk::Enum::Consult;
                                }
                                else
                                {
                                    std::string realTo = Json::get<std::string>(message, "realto");
                                    realJid = split(realTo, '/').front();
                                    chatType = QTalk::Enum::ConsultServer;
                                }
                            }
                            else
                            {
                                xmppId = toJid;
                                realJid = toJid;
                            }

                            direction = 1;

                            if (msgType == 1003 || msgType == 1004 || msgType == 1002 || msgType == 1001)
                                continue;
                        }
                        else
                        {
                            direction = 0;

                            if (isConsult)
                            {
                                xmppId = fromJid;

                                if (chatId == "4")
                                {
                                    std::string realfrom = Json::get<std::string>(message, "realfrom");
                                    realJid = split(realfrom, '/').front();
                                    chatType = QTalk::Enum::ConsultServer;
                                }
                                else
                                {
                                    realJid = fromJid;
                                    chatType = QTalk::Enum::Consult;
                                }
                            }
                            else
                            {
                                xmppId = fromJid;
                                realJid = fromJid;
                            }

                            if (msgType == 1004 || msgType == 1003 || msgType == 1002 || msgType == 1001)
                                continue;
                        }

                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = direction;
                        msgInfo.From = fromJid;
                        msgInfo.To = toJid;
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1; // 从服务器拉回来的消息 -> 消息已送达服务器
                        msgInfo.ReadedTag = readFlag;
                        msgInfo.LastUpdateTime = msec_times;
                        msgList.push_back(msgInfo);
                    }
                }
            }
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", strUrl, postData);
        _pComm->addHttpRequest(req, callBack);

        if (saveDb && !msgList.empty())
            LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
    }

    return msgList;
}

/**
 * 获取客服测 consult会话上翻历史消息
 * @param time
 * @param userId
 * @param realJid
 */
VectorMessage OfflineMessageManager::getConsultServerMessage(const QInt64 &time, const std::string &userId,
        const std::string &realJid, const std::string &direction,
        bool saveDb)
{
    if(realJid.empty())
        return VectorMessage();

    std::ostringstream url;
    url << NavigationManager::instance().getJavaHost()
        << "/qtapi/getconsultmsgs.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();
    std::string strUrl = url.str();
    QTalk::Entity::JID jid(userId);
    QTalk::Entity::JID relId(realJid);
    nJson obj;
    obj["from"] = PLAT.getSelfUserId();
    obj["fhost"] = PLAT.getSelfDomain();
    obj["to"] = relId.username();
    obj["virtual"] = jid.username();
    obj["thost"] = jid.domainname();
    obj["direction"] = direction.data();
    obj["time"] = time;
    obj["domain"] = PLAT.getSelfDomain();
    obj["num"] = 20;
    obj["f"] = "t";
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImMessageInfo> msgList;
    auto callBack = [ userId, &msgList](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson resData = Json::parse(responseData);

            if (resData == nullptr)
            {
                error_log("json paring error");
                return;
            }

            bool ret = Json::get<bool >(resData, "ret");

            if (ret)
            {
                std::string selfJid = PLAT.getSelfUserId();
                nJson data = Json::get<nJson >(resData, "data");

                for (auto &item : data)
                {
                    std::string from = Json::get<std::string>(item, "from");
                    std::string fromDomain = Json::get<std::string>(item, "from_host");
                    std::string fromJid = from + "@" + fromDomain;
                    std::string to = Json::get<std::string>(item, "to");
                    std::string toDomain = Json::get<std::string>(item, "to_host");
                    std::string toJid = to + "@" + toDomain;
                    int readFlag = Json::get<int>(item, "read_flag");
                    std::string xmppId;
                    std::string realJid;
                    QUInt8 chatType = QTalk::Enum::TwoPersonChat;

                    if (item.contains("body") && item.contains("message"))
                    {
                        nJson message = Json::get<nJson >(item, "message");
                        nJson body = Json::get<nJson >(item, "body");
                        std::string type = Json::get<std::string>(message, "type");
                        std::string msgId = Json::get<std::string>(body, "id");

                        //
                        if (msgId.empty()) continue;

                        std::string chatId = Json::get<std::string>(message, "qchatid");

                        if (chatId.empty())
                            chatId = Json::get<std::string>(body, "qchatid");

                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        std::string channelInfo = Json::get<std::string>(message, "channelid");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");
                        int direction = 0;

                        if (fromJid == (selfJid + "@" + fromDomain))
                        {
                            xmppId = toJid;

                            if (chatId == "4")
                            {
                                realJid = toJid;
                                chatType = QTalk::Enum::Consult;
                            }
                            else
                            {
                                std::string realTo = Json::get<std::string>(message, "realto");
                                realJid = split(realTo, '/').front();
                                chatType = QTalk::Enum::ConsultServer;
                            }

                            direction = 1;

                            if (msgType == 1003 || msgType == 1004 || msgType == 1002 || msgType == 1001)
                                continue;
                        }
                        else
                        {
                            direction = 0;
                            xmppId = fromJid;

                            if (chatId == "4")
                            {
                                std::string realfrom = Json::get<std::string>(message, "realfrom");
                                realJid = split(realfrom, '/').front();
                                chatType = QTalk::Enum::ConsultServer;
                            }
                            else
                            {
                                realJid = fromJid;
                                chatType = QTalk::Enum::Consult;
                            }

                            if (msgType == 1004 || msgType == 1003 || msgType == 1002 || msgType == 1001)
                                continue;
                        }

                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = direction;
                        msgInfo.From = fromJid;
                        msgInfo.To = toJid;
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1; // 从服务器拉回来的消息 -> 消息已送达服务器
                        msgInfo.ReadedTag = readFlag;
                        msgInfo.LastUpdateTime = msec_times;
                        msgList.push_back(msgInfo);
                    }
                }
            }
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", strUrl, postData);
        _pComm->addHttpRequest(req, callBack);

        if (saveDb && !msgList.empty())
            LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
    }

    return msgList;
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/12/13
  */
VectorMessage OfflineMessageManager::getGroupMessage(const QInt64 &time,
        const std::string &userId,
        const std::string &direction,
        bool saveDb)
{
    std::ostringstream url;
    url << NavigationManager::instance().getJavaHost()
        << "/qtapi/getmucmsgs.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();
    std::string strUrl = url.str();
    QTalk::Entity::JID jid(userId);
    nJson obj;
    obj["muc"] = jid.username();
    obj["direction"] = direction.data();
    obj["num"] = 20;
    obj["time"] = time;
    obj["domain"] = jid.domainname();
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImMessageInfo> msgList;
    auto callBack = [strUrl, postData, userId, &msgList](int code, const std::string & responseData)
    {
        info_log("{0} \n {1} \n{2}", strUrl, postData, responseData);

        if (code == 200)
        {
            nJson resdata = Json::parse(responseData);

            if (resdata == nullptr)
            {
                error_log("json paring error");
                return;
            }

            int ret = Json::get<nJson >(resdata, "ret");

            if (ret)
            {
                std::string selfJid = PLAT.getSelfXmppId();
                nJson data = Json::get<nJson >(resdata, "data");

                for (auto &item : data)
                {
                    std::string nickName = Json::get<std::string>(item, "nick");
                    int chatType = QTalk::Enum::GroupChat;
                    nJson message = Json::get<nJson >(item, "message");
                    std::string xmppId = Json::get<std::string>(message, "to");

                    if (xmppId.empty())
                        continue;

                    const std::string &realJid = xmppId;

                    if (item.contains("body"))
                    {
                        nJson body = Json::get<nJson >(item, "body");
                        std::string msgId = Json::get<std::string>(body, "id");

                        if (msgId.empty())
                            continue;

                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
                        //                    int msgType = Json::get<int>(body,"msgType");
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");
                        std::string realFrom = Json::get<std::string>(message, "sendjid");
                        int direction = 0;
                        int readState = 0;

                        if (realFrom == selfJid)
                        {
                            direction = 1;
                            readState = 1;//已发送
                        }
                        else
                        {
                            direction = 0;
                            readState = 3;//未读
                        }

                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = direction;
                        msgInfo.From = realFrom;
                        msgInfo.To = "";
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1;
                        msgInfo.ReadedTag = readState;
                        msgInfo.LastUpdateTime = msec_times;
                        msgList.push_back(msgInfo);
                    }
                }
            }
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callBack);

        if (saveDb && !msgList.empty())
            LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
    }

    return msgList;
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/12/13
  */
VectorMessage OfflineMessageManager::getSystemMessage(const QInt64 &time, const std::string &userId,
        const std::string &direction, bool saveDb)
{
    std::ostringstream url;
    url << NavigationManager::instance().getJavaHost()
        << "/qtapi/get_system_msgs.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();
    std::string strUrl = url.str();
    QTalk::Entity::JID jid(userId);
    nJson obj;
    obj["from"] = PLAT.getSelfUserId();
    obj["fhost"] = PLAT.getSelfDomain();
    obj["to"] = jid.username();
    obj["thost"] = jid.domainname();
    obj["direction"] = direction.data();
    obj["time"] = time;
    obj["domain"] = PLAT.getSelfDomain();
    obj["num"] = 20;
    obj["f"] = "t";
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImMessageInfo> msgList;
    auto callBack = [ userId, &msgList](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson resData = Json::parse(responseData);

            if (resData == nullptr)
            {
                error_log("json paring error");
                return;
            }

            int ret = Json::get<nJson >(resData, "ret");

            if (ret)
            {
                std::string selfJid = PLAT.getSelfUserId();
                nJson data = Json::get<nJson >(resData, "data");

                for (auto &item : data)
                {
                    int chatType = QTalk::Enum::System;
                    nJson message = Json::get<nJson >(item, "message");
                    std::string xmppId = userId;
                    std::string type = Json::get<std::string>(message, "type");
                    const std::string &realJid = xmppId;

                    if (item.contains("body"))
                    {
                        nJson body = Json::get<nJson >(item, "body");
                        std::string msgId = Json::get<std::string>(body, "id");

                        if (msgId.empty())
                            continue;

                        QInt64 msec_times = atoll(Json::get<std::string>(message, "msec_times").data());
                        std::string msg = Json::get<std::string>(body, "content");
                        int platform = atoi(Json::get<std::string>(body, "maType").data());
                        int msgType = atoi(Json::get<std::string>(body, "msgType").data());
                        std::string extendInfo = Json::get<std::string>(body, "extendInfo");
                        std::string backupinfo = Json::get<std::string>(body, "backupinfo");
                        std::string realFrom = Json::get<std::string>(message, "sendjid");
                        QTalk::Entity::ImMessageInfo msgInfo;
                        msgInfo.MsgId = msgId;
                        msgInfo.XmppId = xmppId;
                        msgInfo.RealJid = realJid;
                        msgInfo.ChatType = chatType;
                        msgInfo.Direction = 0;
                        msgInfo.From = realFrom;
                        msgInfo.To = "";
                        msgInfo.Content = msg;
                        msgInfo.ExtendedInfo = extendInfo;
                        msgInfo.BackupInfo = backupinfo;
                        msgInfo.Platform = platform;
                        msgInfo.Type = msgType;
                        msgInfo.State = 1;
                        msgInfo.ReadedTag = 3;
                        msgInfo.LastUpdateTime = msec_times;
                        msgList.push_back(msgInfo);
                    }
                }
            }
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        info_log("{0} \n {1}", strUrl, postData);
        _pComm->addHttpRequest(req, callBack);

        if (saveDb && !msgList.empty())
            LogicManager::instance()->getDatabase()->bulkInsertMessageInfo(msgList);
    }

    return msgList;
}
