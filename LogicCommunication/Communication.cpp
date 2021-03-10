#include <utility>
#include <iostream>
#include <algorithm>
#include "Communication.h"
#include "../entity/IM_Session.h"
#include "../LogicManager/LogicManager.h"
#include "../QtUtil/Entity/JID.h"
#include "../include/im_enum.h"
#include "FileHelper.h"
#include "OfflineMessageManager.h"
#include "../Platform/Platform.h"
#include "UserManager.h"
#include "GroupManager.h"
#include "../QtUtil/Utils/utils.h"
#include "../QtUtil/nJson/nJson.h"
#include "../Message/GroupMessage.h"
#include "../QtUtil/Utils/Log.h"
#include "OnLineManager.h"
#include "../include/CommonStrcut.h"
#include "SearchManager.h"
#include "UserConfig.h"
#include "HotLinesConfig.h"
#include "../Platform/NavigationManager.h"
#include "../Platform/dbPlatForm.h"
#include "../include/perfcounter.h"
#include <time.h>
#include "../interface/logic/ILogicBase.h"

#if defined(_MACOS)
    #define DEM_CLIENT_TYPE ClientTypeMac
    #include <unistd.h>
#elif defined(_LINUX)
    #define DEM_CLIENT_TYPE ClientTypeLinux
    #include <unistd.h>
#else
    #include <windows.h>
    #define DEM_CLIENT_TYPE ClientTypePC
#endif
//

#define QTALK_2_0_CONFIG "QTALK_2_0_CONFIG"

#define LOGIN_TYPE_NEW_PASSWORD "newpassword"

using namespace QTalk;

Communication::Communication()
    : _port(0)
{
    //    _pMsgManager = new CommMsgManager;
    _pMsgListener = new CommMsgListener(this);
    _pFileHelper = new FileHelper(this);
    _pUserManager = new UserManager(this);
    _pUserGroupManager = new GroupManager(this);
    _pOnLineManager = new OnLineManager(this);
    _pSearchManager = new SearchManager(this);
    _pOfflineMessageManager = new OfflineMessageManager(this);
    _pUserConfig = new UserConfig(this);
    _pHotLinesConfig = new HotLinesConfig(this);

    for (int i = 0; i < _threadPoolCount; ++i)
    {
        auto name = SFormat("Communication's http pool {0}", i);
        auto *item = new ThreadPool(name);
        info_log("{0} -> {1}", item, name);
        _httpPool.push_back(item);
    }

    std::function<int(STLazyQueue<std::pair<std::string, UserCardMapParam>> *)> usercardFun = [this](STLazyQueue<std::pair<std::string, UserCardMapParam>> *queue) -> int
    {
        int runningCount = 0;

        if (queue != nullptr && !queue->empty())
        {
            std::map<std::string, UserCardMapParam> params;

            while (!queue->empty())
            {
                auto tmpParam = queue->front();
                const std::string &groupId = tmpParam.first;
                auto tmp = tmpParam.second;

                for (const auto &item : tmp)
                    params[groupId][item.first] = item.second;

                queue->pop();
                runningCount++;
            }

            // 插库
            if (_pUserManager)
            {
                for (const auto &it : params)
                {
                    std::vector<QTalk::StUserCard> arUserInfo;
                    _pUserManager->getUserCard(it.second, arUserInfo);
                    // 下载头像
                    downloadUserHeadByStUserCard(arUserInfo);
                    CommMsgManager::updateGroupMemberInfo(it.first, arUserInfo);
                }
            }
        }

        return runningCount;
    };
    userCardQueue = new STLazyQueue<std::pair<std::string, UserCardMapParam>>(50, usercardFun);
    //
    //    _pUpdaterManager = new UpdaterManager(this);
}

Communication::~Communication()
{
    if (nullptr != _pMsgListener)
    {
        delete _pMsgListener;
        _pMsgListener = nullptr;
    }

    if (nullptr != _pFileHelper)
    {
        delete _pFileHelper;
        _pFileHelper = nullptr;
    }

    if (_pUserManager)
    {
        delete _pUserManager;
        _pUserManager = nullptr;
    }

    if (_pOnLineManager)
    {
        delete _pOnLineManager;
        _pOnLineManager = nullptr;
    }

    if (nullptr != _pOfflineMessageManager)
    {
        delete _pOfflineMessageManager;
        _pOfflineMessageManager = nullptr;
    }
}

/**
 * 登录处理
 * @param userName
 * @param password
 * @return
 */
bool Communication::OnLogin(const std::string &userName, const std::string &password)
{
    // 设置当前登录的userid
    PLAT.setSelfUserId(userName);

    // 下载公钥
    if (!_pFileHelper->DownloadPubKey())
        return false;

    const std::string host = NavigationManager::instance().getXmppHost();
    const int port = NavigationManager::instance().getProbufPort();
    std::string domain = NavigationManager::instance().getDomain();
    info_log("login at host:{0} domain:{1} port: {2}", host, domain, port);

    if (port == 0 || domain.empty() || host.empty())
    {
        //
        warn_log("nav info error (port == 0 || domain.empty())");
        CommMsgManager::sendLoginErrMessage("获取服务器地址失败!");
        return false;
    }

    //增加新登录逻辑
    std::string loginType = NavigationManager::instance().getLoginType();

    if (!loginType.empty() && LOGIN_TYPE_NEW_PASSWORD == loginType)
    {
        std::map<std::string, std::string> map;
        getNewLoginToken(userName, password, map);
        std::string u = map["u"];
        std::string p = map["t"];

        if (!u.empty() && !p.empty())
        {
            // 设置当前登录的userid
            PLAT.setSelfUserId(u);
            nJson nauth;
            nJson gObj;
            gObj["u"] = userName;
            gObj["p"] = p;
            gObj["mk"] = QTalk::utils::getMessageId();
            nauth["nauth"] = gObj;
            std::string pp = nauth.dump();

            if (!AsyncConnect(u + "@" + domain, pp, host, port))
            {
                CommMsgManager::sendLoginErrMessage("连接服务器失败!");
                return false;
            }
        }
        else
        {
            CommMsgManager::sendLoginErrMessage("账户验证失败!");
            return false;
        }
    }
    else
    {
        std::string loginName = userName + "@" + domain;

        if (!AsyncConnect(loginName, password, host, port))
        {
            CommMsgManager::sendLoginErrMessage("连接服务器失败!");
            return false;
        }
    }

    return true;
}

//
bool Communication::AsyncConnect(const std::string &userName, const std::string &password, const std::string &host,
                                 int port)
{
    info_log("start login: user:{0}, password length:{1}, host:{2}, port:{3}", userName, password.length(), host, port);
    _userName = userName;
    _password = password;
    _host = host;
    _port = port;
    return tryConnectToServer();
}

//
bool Communication::tryConnectToServer()
{
    bool isNewLogin = (LOGIN_TYPE_NEW_PASSWORD == NavigationManager::instance().getLoginType());
    return LogicManager::instance()->getLogicBase()->tryConnectToServer(_userName, _password, _host, _port, isNewLogin);
}

/**
 * qchat断线重连 需要根据qvt重新获取token
 */
bool Communication::tryConnectToServerByQVT()
{
    std::string qvt = PLAT.getQvt();
    std::map<std::string, std::string> userMap;
    getQchatTokenByQVT(qvt, userMap);
    return LogicManager::instance()->getLogicBase()->tryConnectToServer(_userName, userMap["password"], _host, _port, false);
}

/**
 * 获取导航信息
 * @param navAddr
 */
bool Communication::getNavInfo(const std::string &navAddr, QTalk::StNav &nav)
{
    std::string url = navAddr;

    if (navAddr.find('?') == std::string::npos)
        url += "?p=pc";
    else
        url += "&p=pc";

    if (navAddr.find("nauth=") == std::string::npos)
        url += "&nauth=true";

    bool ret = false;
    auto func = [url, &ret, &nav](int code, const std::string & resData)
    {
        if (code == 200)
        {
            nJson data = Json::parse(resData);

            if (nullptr == data)
            {
                error_log("nav data parse error");
                return;
            }

            nav.version = Json::get<long long>(data, "version", 0);
            // base address
            nJson baseAddress = Json::get<nJson>(data, "baseaddess");
            nav.xmppHost = Json::get<std::string>(baseAddress, "xmpp");
            nav.domain = Json::get<std::string>(baseAddress, "domain");
            nav.protobufPcPort = Json::get<int>(baseAddress, "protobufPcPort");
            nav.apiurl = Json::get<std::string>(baseAddress, "apiurl");
            nav.javaurl = Json::get<std::string>(baseAddress, "javaurl");
            nav.httpurl = Json::get<std::string>(baseAddress, "httpurl");
            nav.pubkey = Json::get<std::string>(baseAddress, "pubkey");
            nav.fileurl = Json::get<std::string>(baseAddress, "fileurl");
            nav.mobileurl = Json::get<std::string>(baseAddress, "mobileurl");
            nav.leaderUrl = Json::get<std::string>(baseAddress, "leaderurl");
            nav.shareUrl = Json::get<std::string>(baseAddress, "shareurl");
            nav.videoUrl = Json::get<std::string>(baseAddress, "videourl");
            nav.videoConference = Json::get<std::string>(baseAddress, "videoConference");
            nav.checkUpdateUrl = Json::get<std::string>(baseAddress, "checkupdate");

            //imcofig
            if (data.contains("imConfig"))
            {
                nJson imcofig = Json::get<nJson>(data, "imConfig");
                nav.rsaEncodeType = Json::get<int>(imcofig, "RsaEncodeType");
                nav.uploadLog = Json::get<std::string>(imcofig, "uploadLog");
                nav.mailSuffix = Json::get<std::string>(imcofig, "mail");
                nav.foundConfigUrl = Json::get<std::string>(imcofig, "foundConfigUrl");
            }

            // ops
            if (data.contains("ops"))
            {
                nJson ops = Json::get<nJson>(data, "ops");
                nav.opsHost = Json::get<std::string>(ops, "host");
            }

            // ability
            if (data.contains("ability"))
            {
                nJson ability = Json::get<nJson>(data, "ability");
                nav.qCloudHost = Json::get<std::string>(ability, "qCloudHost");
                nav.searchUrl = Json::get<std::string>(ability, "new_searchurl");
                nav.showmsgstat = Json::get<bool>(ability, "showmsgstat", false);
                nav.qcGrabOrder = Json::get<std::string>(ability, "qcGrabOrder");
            }

            //qcadmin
            if (data.contains("qcadmin"))
            {
                nJson qcadmin = Json::get<nJson>(data, "qcadmin");
                nav.qcadminHost = Json::get<std::string>(qcadmin, "host");
            }

            //Login
            if (data.contains("Login"))
            {
                nJson login = Json::get<nJson>(data, "Login");

                if (login.contains("loginType"))
                    nav.loginType = Json::get<std::string>(login, "loginType");
            }

            // client
            if (data.contains("client"))
            {
                nJson login = Json::get<nJson>(data, "client");

                if (login.contains("rollback"))
                    nav.rollback = Json::get<bool>(login, "rollback");
            }

            ret = true;
        }
    };

    try
    {
        QTalk::HttpRequest req(url);
        addHttpRequest(req, func);
    }
    catch (const std::exception &e)
    {
        error_log(e.what());
    }

    return ret;
}

/**
 *
 * @param nav
 */
void Communication::setLoginNav(const QTalk::StNav &nav)
{
    NavigationManager::instance().setVersion(nav.version);
    NavigationManager::instance().setXmppHost(nav.xmppHost);
    NavigationManager::instance().setDomain(nav.domain);
    PLAT.setSelfDomain(nav.domain);
    NavigationManager::instance().setProbufPort(nav.protobufPcPort);
    NavigationManager::instance().setApiHost(nav.apiurl);
    NavigationManager::instance().setJavaHost(nav.javaurl);
    NavigationManager::instance().setHttpHost(nav.httpurl);
    NavigationManager::instance().setPubKey(nav.pubkey);
    NavigationManager::instance().setFileHttpHost(nav.fileurl);
    NavigationManager::instance().setPhoneNumAddr(nav.mobileurl);
    NavigationManager::instance().setOpsApiHost(nav.opsHost);
    NavigationManager::instance().setQCHost(nav.qCloudHost);
    NavigationManager::instance().setQcadminHost(nav.qcadminHost);
    NavigationManager::instance().setRsaEncodeType(nav.rsaEncodeType);
    NavigationManager::instance().setSearchUrl(nav.searchUrl);
    NavigationManager::instance().setUploadLog(nav.uploadLog);
    NavigationManager::instance().setMailSuffix(nav.mailSuffix);
    NavigationManager::instance().setLeaderUrl(nav.leaderUrl);
    NavigationManager::instance().setShareUrl(nav.shareUrl);
    NavigationManager::instance().setFoundConfigUrl(nav.foundConfigUrl);
    NavigationManager::instance().setShowmsgstat(nav.showmsgstat);
    NavigationManager::instance().setQcGrabOrder(nav.qcGrabOrder);
    NavigationManager::instance().setLoginType(nav.loginType);
    // checkupadte
    NavigationManager::instance().setCheckUpteUrl(nav.checkUpdateUrl);
    // video
    NavigationManager::instance().setVideoUrl(nav.videoUrl);
    NavigationManager::instance().setvideoConference(nav.videoConference);
    //
    NavigationManager::instance().setRollbackFlag(nav.rollback);
}

/**
  * @函数名
  * @功能描述 登陆成功后同步各服务器数据 同步完成后发送同步信息完成event
  * @参数
  * @date 2018.9.28
  */
void Communication::synSeverData()
{
    try
    {
        // 发发送一次心跳
        LogicManager::instance()->getLogicBase()->sendHeartbeat();
        //        sendHeartbeat();
        // 获取组织架构
        debug_log("获取组织架构");
        CommMsgManager::sendLoginProcessMessage("getting user information");

        if (_pUserManager)
        {
            bool ret = _pUserManager->getNewStructure();
            CommMsgManager::sendGotStructure(ret);
        }

        // 获取群信息
        CommMsgManager::sendLoginProcessMessage("getting group information");

        if (_pUserGroupManager)
        {
            debug_log("获取群信息");
            MapGroupCard groups;

            if (_pUserGroupManager->getUserGroupInfo(groups))
            {
                //                _pUserGroupManager->getGroupCard(groups);
                _pUserGroupManager->getUserIncrementMucVcard();
                CommMsgManager::sendGotGroupCard();
            }
        }

        // 获取单人配置
        debug_log("获取单人配置");
        CommMsgManager::sendLoginProcessMessage("initializing configuration");
        _pUserConfig->getUserConfigFromServer(false);
        // 初始化配置
        debug_log("初始化配置");
        std::string strPlatform = PLAT.getPlatformStr();
        std::string userConfig;
        LogicManager::instance()->getDatabase()->getConfig(QTALK_2_0_CONFIG, strPlatform, userConfig);
        AppSetting::instance().initAppSetting(userConfig);
        // 获取好友列表
        //        debug_log("获取好友列表");
        //        getFriendList();
        // 获取单人离线消息
        debug_log("获取单人消息");
        CommMsgManager::sendLoginProcessMessage("getting user message");
        bool isok = _pOfflineMessageManager->updateChatOfflineMessage();

        //
        if (!isok)
        {
            CommMsgManager::sendGetHistoryError();
            return;
        }

        CommMsgManager::sendLoginProcessMessage("updating message read mask");
        _pOfflineMessageManager->updateChatMasks();
        // 获取群离线消息
        debug_log("获取群离线消息");
        CommMsgManager::sendLoginProcessMessage("getting group message");
        isok = _pOfflineMessageManager->updateGroupOfflineMessage();

        if (!isok)
        {
            CommMsgManager::sendGetHistoryError();
            return;
        }

        //
        debug_log("更新群阅读状态");
        _pOfflineMessageManager->updateGroupMasks();
        // 获取通知消息
        debug_log("获取通知消息");
        CommMsgManager::sendLoginProcessMessage("getting notice message");
        isok = _pOfflineMessageManager->updateNoticeOfflineMessage();

        if (!isok)
        {
            CommMsgManager::sendGetHistoryError();
            return;
        }

        //获取热线账号
#ifndef _STARTALK
        debug_log("获取热线账号信息");
        _pHotLinesConfig->getVirtualUserRole();
#endif
        // 根据离线消息生成sessionList
        //LogicManager::instance()->GetDatabase()->QueryImSessionInfos();
        // 获取最新版本号
        //        initAppNetVersion();
        // 增量拉取勋章信息
        getMedalList();
        getUserMedal();
        //
        CommMsgManager::sendLoginProcessMessage("getting user state");
        CommMsgManager::sendSynOfflineSuccess();
        // 同步完离线后 开启在线信息查询定时器
        synUsersUserStatus();
        CommMsgManager::sendLoginProcessMessage("login success");
    }
    catch (const std::exception &e)
    {
        error_log(e.what());
        throw std::runtime_error(e.what());
    }
}

void Communication::synUsersUserStatus()
{
    if (_pOnLineManager)
    {
        std::set<std::string> users;
        LogicManager::instance()->getDatabase()->getCareUsers(users);
        std::set<std::string> tmpUsers;

        for (const auto &u : users)
        {
            tmpUsers.insert(u);

            if (tmpUsers.size() >= 200)
            {
                if (_pOnLineManager)
                    _pOnLineManager->getOnLineUser(tmpUsers);

                tmpUsers.clear();
            }
        }

        if (_pOnLineManager && !tmpUsers.empty())
            _pOnLineManager->getOnLineUser(tmpUsers);

        CommMsgManager::sendOnlineUpdate();
    }
}

/**
  * @函数名   getUserHistoryMessage
  * @功能描述 获取对应人小于对应时间的历史消息
  * @参数     time 时间 userName 人
     void
  * @author   cc
  * @date     2018/09/28
  */
void Communication::getUserHistoryMessage(const QInt64 &time, const QUInt8 &chatType, const QTalk::Entity::UID &uid,
        std::vector<QTalk::Entity::ImMessageInfo> &msgList)
{
    bool ret = LogicManager::instance()->getDatabase()->getUserMessage(time, uid.usrId(), uid.realId(), msgList);

    if (!ret || msgList.empty())
    {
        switch (chatType)
        {
            case QTalk::Enum::TwoPersonChat:
            case QTalk::Enum::Consult:
                _pOfflineMessageManager->getUserMessage(time, uid.usrId());
                break;

            case QTalk::Enum::GroupChat:
                _pOfflineMessageManager->getGroupMessage(time, uid.usrId());
                break;

            case QTalk::Enum::System:
                _pOfflineMessageManager->getSystemMessage(time, uid.usrId());
                break;

            case QTalk::Enum::ConsultServer:
                _pOfflineMessageManager->getConsultServerMessage(time, uid.usrId(), uid.realId());
                break;

            default:
                break;
        }

        LogicManager::instance()->getDatabase()->getUserMessage(time, uid.usrId(), uid.realId(), msgList);
    }
}

void Communication::getNetHistoryMessage(const QInt64 &time, const QUInt8 &chatType,
        const QTalk::Entity::UID &uid,
        const std::string &direction,
        std::vector<QTalk::Entity::ImMessageInfo> &msgList)
{
    switch (chatType)
    {
        case QTalk::Enum::TwoPersonChat:
        case QTalk::Enum::Consult:
            msgList = _pOfflineMessageManager->getUserMessage(time, uid.usrId(), direction);
            break;

        case QTalk::Enum::GroupChat:
            msgList = _pOfflineMessageManager->getGroupMessage(time, uid.usrId(), direction);
            break;

        case QTalk::Enum::System:
            msgList = _pOfflineMessageManager->getSystemMessage(time, uid.usrId(), direction);
            break;

        case QTalk::Enum::ConsultServer:
            msgList = _pOfflineMessageManager->getConsultServerMessage(time, uid.usrId(), uid.realId(), direction);
            break;

        default:
            break;
    }
}

/**
  * @函数名   onCreateGroupCompleted
  * @功能描述 收到创建群通知
  * @参数
  * @author   may
  * @date     2018/11/10
  */
void Communication::onCreateGroupComplete(const std::string &groupId, bool ret)
{
    if (ret && _pUserGroupManager)
    {
        QTalk::StGroupInfo groupInfo;
        groupInfo.desc = "";
        groupInfo.groupId = groupId;

        if (_mapGroupIdName.find(groupId) != _mapGroupIdName.end() && !_mapGroupIdName[groupId].empty())
            groupInfo.name = _mapGroupIdName[groupId];
        else
            groupInfo.name = "新建群聊...";

        groupInfo.title = "";
        QTalk::Entity::ImGroupInfo im_gInfo;
        im_gInfo.GroupId = groupInfo.groupId;
        LogicManager::instance()->getDatabase()->insertGroupInfo(im_gInfo);

        if (_pUserGroupManager->upateGroupInfo(groupInfo))
            _mapGroupIdName.erase(groupId);
    }
}

/**
  * @函数名   onRecvGroupMembers
  * @功能描述 收到群成员信息
  * @参数
  * @author   cc
  * @date     2018/10/08
  */
void Communication::onRecvGroupMembers(const std::string &groupId, const std::map<std::string, QUInt8> &mapUserRole)
{
    std::map<std::string, QTalk::StUserCard> mapStUsers;

    // 存储数据库
    if (!mapUserRole.empty())
    {
        LogicManager::instance()->getDatabase()->bulkDeleteGroupMember({groupId});
        LogicManager::instance()->getDatabase()->bulkInsertGroupMember(groupId, mapUserRole);
    }
    else
        return;

    for (const auto &item : mapUserRole)
    {
        auto card = QTalk::StUserCard();
        card.xmppId = item.first;
        mapStUsers[item.first] = card;
    }

    LogicManager::instance()->getDatabase()->getGroupMemberInfo(mapStUsers);

    //
    if (!mapStUsers.empty())
    {
        // 下载头像
        //            downloadUserHeadByStUserCard(arStUsers);
        GroupMemberMessage e;
        e.groupId = groupId;
        e.userRole = mapUserRole;
        e.members = mapStUsers;
        CommMsgManager::gotGroupMember(e);
        // 获取群成员名片
        std::set<std::string> users;
        UserCardMapParam param;

        for (auto &itm : mapStUsers)
        {
            const std::string &strXmppId = itm.first;
            auto pos = strXmppId.find('@');

            if (pos != std::string::npos)
            {
                users.insert(strXmppId);
                std::string userId = strXmppId.substr(0, pos);
                std::string domain = strXmppId.substr(pos + 1);
                param[domain][userId] = itm.second.version;
            }
        }

        userCardQueue->push(std::pair<std::string, UserCardMapParam>(groupId, param));
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/26
  */
void Communication::downloadUserHeadByStUserCard(const std::vector<QTalk::StUserCard> &arUserInfo)
{
    std::vector<std::string> urls;

    for (const QTalk::StUserCard &user : arUserInfo)
    {
        if (user.headerSrc.empty())
            continue;

        urls.push_back(user.headerSrc);
    }

    if (_pFileHelper)
    {
        if (urls.empty())
            return;

        _pFileHelper->batchDownloadHead(urls);
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/09/30
  */
void Communication::batchUpdateHead(const std::vector<std::string> &arXmppids)
{
    if (arXmppids.empty())
        return;

    if (_pUserManager)
    {
        std::map<std::string, std::map<std::string, int>> params;

        for (const auto &arXmppid : arXmppids)
        {
            auto pos = arXmppid.find('@');

            if (pos != std::string::npos)
            {
                std::string user = arXmppid.substr(0, pos);
                std::string domain = arXmppid.substr(pos + 1);
                params[domain][user] = 0;
            }
        }

        std::thread([this, params]()
        {
            std::vector<QTalk::StUserCard> arUserInfo;
            _pUserManager->getUserCard(params, arUserInfo);
            downloadUserHeadByStUserCard(arUserInfo);
            CommMsgManager::sendDownloadHeadSuccess();
        }).detach();
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/10/03
  */
void Communication::getGroupMemberById(const std::string &groupId)
{
    // 获取群公告
    std::string groupTopic;
    LogicManager::instance()->getDatabase()->getGroupTopic(groupId, groupTopic);
    debug_log("群公告 群Id:{0} 群公告:{1}", groupId, groupTopic);

    if (!groupTopic.empty())
        CommMsgManager::gotGroupTopic(groupId, groupTopic);

    // 获取本地群成员
    std::map<std::string, QTalk::StUserCard> arGroupMembers;
    std::map<std::string, QUInt8> userRole;
    LogicManager::instance()->getDatabase()->getGroupMemberById(groupId, arGroupMembers, userRole);

    if (!arGroupMembers.empty())
    {
        GroupMemberMessage e;
        e.groupId = groupId;
        e.userRole = userRole;
        e.members = arGroupMembers;
        CommMsgManager::gotGroupMember(e);
    }

    // 从网络获取数据
    LogicManager::instance()->getLogicBase()->getGroupMemberById(groupId);
}

/**
 * bind成功后处理
 * @param evt
 */
void Communication::dealBindMsg()
{
    CommMsgManager::sendLoginProcessMessage("opening database");
    std::string path = PLAT.getAppdataRoamingUserPath();
    path += "/qtalk.db";
    //FileHelper::creatDir(path);
    std::string errorMsg;

    if (!LogicManager::instance()->getDatabase()->OpenDB(path, errorMsg))
        throw std::runtime_error(errorMsg);

    //将自己的id插入cache_data表 触发器使用
    LogicManager::instance()->getDatabase()->insertUserId(PLAT.getSelfXmppId());
    //设置群阅读指针时间
    std::string last_rmt = LogicManager::instance()->getDatabase()->getLoginBeforeGroupReadMarkTime();

    if (last_rmt == "0")
    {
        std::string current_rmt = LogicManager::instance()->getDatabase()->getGroupReadMarkTime();
        LogicManager::instance()->getDatabase()->saveLoginBeforeGroupReadMarkTime(current_rmt);
    }

    //
    updateTimeStamp();
    //
    LogicManager::instance()->getLogicBase()->startAutoReconnectToServer();
}

/**
  * @函数名   inviteGroupMembers
  * @功能描述 邀请群成员
  * @参数
  * @author   cc
  * @date     2018/10/25
  */
void Communication::inviteGroupMembers(std::vector<std::string> &users, const std::string &groupId)
{
    LogicManager::instance()->getLogicBase()->inviteGroupMembers(users, groupId);
}

/**
  * @函数名   createGroup
  * @功能描述 创建群
  * @参数
  * @author   may
  * @date     2018/11/10
  */
void Communication::createGroup(const std::string &groupId, const std::string &groupName)
{
    _mapGroupIdName[groupId] = groupName;
    LogicManager::instance()->getLogicBase()->createGroup(groupId);
}

/**
  * @函数名   getNetEmoticon
  * @功能描述 获取网络
  * @参数
  * @author   cc
  * @date     2018/10/21
  */
void Communication::getNetEmoticon(GetNetEmoticon &e)
{
    std::ostringstream url;
    url << NavigationManager::instance().getFileHttpHost()
        << "/file/v1/emo/d/e/config?"
        << "p=qtalk";
    auto callback = [&e](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson msgList = Json::parse(responseData);

            if (msgList == nullptr)
            {
                error_log("json paring error");
                return;
            }

            ArStNetEmoticon netEm0os;

            for (auto &item : msgList)
            {
                std::shared_ptr<StNetEmoticon> emo(new StNetEmoticon);
                emo->pkgid = Json::get<std::string>(item, "pkgid");
                emo->emoName = Json::get<std::string>(item, "name");
                emo->emoFile = Json::get<std::string>(item, "file");
                emo->desc = Json::get<std::string>(item, "desc");
                emo->iconPath = Json::get<std::string>(item, "thumb");
                emo->filesize = Json::get<int>(item, "file_size");
                emo->md5 = Json::get<std::string>(item, "md5");
                netEm0os.push_back(emo);
            }

            e.arEmoInfo = netEm0os;
        }
        else
        {
        }
    };
    //
    QTalk::HttpRequest req(url.str());
    addHttpRequest(req, callback);

    if (_pFileHelper)
    {
        auto it = e.arEmoInfo.begin();

        for (; it != e.arEmoInfo.end(); it++)
        {
            std::shared_ptr<StNetEmoticon> emo = *it;
            emo->iconPath = _pFileHelper->downloadEmoticonIcon(emo->iconPath, emo->pkgid);
        }
    }
}

void Communication::getStructure(std::vector<std::shared_ptr<QTalk::Entity::ImUserInfo>> &structure)
{
    LogicManager::instance()->getDatabase()->getStructure(structure);
}

void Communication::onInviteGroupMembers(const std::string &groupId)
{
    if (PLAT.isMainThread())
    {
        std::thread([this, groupId]()
        {
            onInviteGroupMembers(groupId);
        }).detach();
        return;
    }

    getGroupMemberById(groupId);

    if (_mapGroupIdName.find(groupId) != _mapGroupIdName.end() && !_mapGroupIdName[groupId].empty())
    {
        info_log("update group name ");
        QTalk::StGroupInfo groupInfo;
        groupInfo.desc = "";
        groupInfo.groupId = groupId;
        groupInfo.name = _mapGroupIdName[groupId];
        _mapGroupIdName.erase(groupId);
        groupInfo.title = "";
        QTalk::Entity::ImGroupInfo im_gInfo;
        im_gInfo.GroupId = groupInfo.groupId;
        LogicManager::instance()->getDatabase()->insertGroupInfo(im_gInfo);
        auto retry = 3; // while update failed , retry

        while (retry > 0)
        {
            bool ret = _pUserGroupManager->upateGroupInfo(groupInfo);

            if (ret)
                break;
            else
            {
                // sleep for retry
                const std::chrono::milliseconds ms(3000);
                std::this_thread::sleep_for(ms);
                retry--;
            }
        }
    }
}

// 发送http 请求
void Communication::addHttpRequest(const QTalk::HttpRequest &req,
                                   const std::function<void(int, const std::string &)> &callback,
                                   bool showCastWarn)
{
    try
    {
        if (PLAT.isMainThread())
            warn_log("main thread http request {0}", req.url);

        int currentThread = 0;

        if (!req.url.empty() && req.url.size() > 4 && req.url.substr(0, 4) == "http")
        {
            std::string url = req.url;
            std::size_t hash = std::hash<std::string> {}(url);
            currentThread = static_cast<int>(hash % _threadPoolCount);
            auto http = _httpPool[currentThread]->enqueue([req, callback, currentThread, showCastWarn]()
            {
                debug_log("在第{1}个http坑 开始请求: {0}", req.url, currentThread);
                std::string url(req.url);

                if (!AppSetting::instance().with_ssl)
                {
                    if (req.url.substr(0, 5) == "https")
                        url = std::string("http") + req.url.substr(5);
                }

                QTalk::QtHttpRequest request(url, req.timeout);
                //method
                request.setRequestMethod((RequestMethod)req.method);
                // header
                auto itr = req.header.begin();

                for (; itr != req.header.end(); itr++)
                {
                    debug_log("请求header:{0} = {1}", itr->first, itr->second);
                    request.addRequestHeader(itr->first.c_str(), itr->second.c_str());
                }

                std::string requestHeaders = std::string("q_ckey=") + PLAT.getClientAuthKey();
                request.addRequestHeader("Cookie", requestHeaders.c_str());

                // body
                if ((RequestMethod)req.method == QTalk::RequestMethod::POST)
                {
                    debug_log("请求body:{0}", req.body);
                    request.appendPostData(req.body.c_str(), req.body.length());
                }

                // form
                if (!req.formFile.empty())
                    request.addFromFile(req.formFile);

                // process callback
                if (req.addProcessCallback)
                {
                    // 参数 总下载量 当前下载量 总上传量 当前上传量 速度 剩余时间
                    std::function<void(StProcessParam)> processCallback;
                    processCallback = [req](const StProcessParam & param)
                    {
                        CommMsgManager::updateFileProcess(param.key, param.dt, param.dn,
                                                          param.ut, param.un, param.speed, param.leftTime);
                    };
                    request.setProcessCallback(req.processCallbackKey, processCallback);
                }

                // speed
                if (req.speed > 0)
                    request.setDownloadSpeed(req.speed);

                // proxy
                if (AppSetting::instance().enableProxy())
                    request.enableProxy();

                // start
                {
                    if (showCastWarn)
                    {
                        perf_counter("addHttpRequest {0}\n {1}", req.url, req.body);
                        request.startSynchronous();
                    }
                    else
                        request.startSynchronous();
                }
                debug_log("http request: code: {0}", request.getResponseCode());

                if ((RequestMethod)req.method == QTalk::RequestMethod::POST)
                    debug_log("http request result: data: {0}", *request.getResponseData());

                // callback
                callback(request.getResponseCode(), *request.getResponseData());

                if (request.getResponseCode() != 200)
                    error_log("http request failed:{3} -> {0} \n params:{2} \n {1}", req.url, *request.getResponseData(), req.body, request.getResponseCode());
            });
            // wait
            http.get();
        }
        else
        {
            error_log("invalid url {0}", req.url);
            callback(-1, "");
        }
    }
    catch (const std::exception &e)
    {
        error_log("http exception, {0}", e.what());
    }
}

// 获取session and 个人配置
void Communication::getSessionData()
{
    // 最新个人配置
    updateUserConfigFromDb();

    // 群组列表
    std::vector<QTalk::Entity::ImGroupInfo> groups;
    LogicManager::instance()->getDatabase()->getAllGroup(groups);
    CommMsgManager::sendGotGroupList(groups);
}

/**
 * 更新个人配置
 */
void Communication::updateUserConfigFromDb()
{
    std::vector<QTalk::Entity::ImConfig> arConfig;
    LogicManager::instance()->getDatabase()->getAllConfig(arConfig);

    std::map<std::string, std::string> mapConf;
    LogicManager::instance()->getDatabase()->getConfig("kMarkupNames", mapConf);
    DB_PLAT.setMaskNames(mapConf);

    //
    if (!arConfig.empty())
        CommMsgManager::updateUserConfigs(arConfig);
}

/**
 *
 * @param keyName
 * @param count
 */
void Communication::getStructureCount(const std::string &keyName, int &count)
{
    LogicManager::instance()->getDatabase()->getStructureCount(keyName, count);
}

/**
 *
 * @param keyName
 * @param arMember
 */
void Communication::getStructureMember(const std::string &keyName, std::vector<std::string> &arMember)
{
    LogicManager::instance()->getDatabase()->getStructureMember(keyName, arMember);
}

/**
 * 下载收藏表情
 * @param arDownloads
 */
void Communication::downloadCollection(const std::vector<std::string> &arDownloads)
{
    auto it = arDownloads.cbegin();

    for (; it != arDownloads.cend(); it++)
        _pFileHelper->getLocalImgFilePath(*it, "/emoticon/collection/");
}

/**
 * 动态获取oa部分 ui组件
 */
bool Communication::geiOaUiData(std::vector<QTalk::StOAUIData> &arOAUIData)
{
    bool ret = false;
    std::ostringstream url;
    url << NavigationManager::instance().getFoundConfigUrl();
    nJson gObj;
    gObj["qtalk"] = PLAT.getSelfUserId().c_str();
    gObj["platform"] = "PC";
    gObj["version"] = PLAT.getClientNumVerison();
    std::string postData = gObj.dump();
    //
    std::vector<std::string> arIcons;
    //
    auto callback = [&arOAUIData, &ret, &arIcons](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson retDta = Json::parse(responseData);

            if (nullptr == retDta)
            {
                error_log("json load error");
                return;
            }

            int errCode = Json::get<int>(retDta, "errorCode");

            //
            if (errCode == 0)
            {
                nJson data = Json::get<nJson>(retDta, "data");

                for (auto &groupItem : data)
                {
                    QTalk::StOAUIData stOAData;
                    stOAData.groupId = Json::get<int>(groupItem, "groupId");
                    stOAData.groupName = Json::get<std::string>(groupItem, "groupName");
                    stOAData.groupIcon = Json::get<std::string>(groupItem, "groupIcon");
                    //
                    arIcons.push_back(stOAData.groupIcon);
                    //
                    nJson memberItems = Json::get<nJson>(groupItem, "members");

                    for (auto &memItem : memberItems)
                    {
                        QTalk::StOAUIData::StMember member;
                        member.memberId = Json::get<int>(memItem, "memberId");

                        if (memItem.contains("actionType"))
                            member.action_type = Json::get<int>(memItem, "actionType");

                        if (memItem.contains("nativeAction"))
                            member.native_action_type = Json::get<int>(memItem, "nativeAction");

                        member.memberName = Json::get<std::string>(memItem, "memberName");
                        member.memberAction = Json::get<std::string>(memItem, "memberAction");
                        member.memberIcon = Json::get<std::string>(memItem, "memberIcon");
                        //
                        arIcons.push_back(member.memberIcon);
                        stOAData.members.push_back(member);
                    }

                    arOAUIData.push_back(stOAData);
                }

                ret = true;
            }
            else
            {
                if (retDta.contains("errorMsg"))
                    error_log(Json::get<std::string>(retDta, "errorMsg"));
            }
        }
        else
            warn_log("get oa data error");
    };
    QTalk::HttpRequest req(url.str(), QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/json;";
    req.body = postData;
    addHttpRequest(req, callback);

    if (ret)
    {
        auto it = arIcons.cbegin();

        for (; it != arIcons.cend(); it++)
            _pFileHelper->getLocalImgFilePath(*it, "/oaIcon/");
    }

    return ret;
}

std::string Communication::getQchatQvt(const std::string &userName, const std::string &password)
{
    std::string qvtStr;
    std::ostringstream url;
    url << NavigationManager::instance().getApiUrl()
        << "/get_power";
    std::string type = "username";
    regex regPhone("^1\\d{10}$");
    smatch smatchPhone;

    if (regex_match(userName, smatchPhone, regPhone))
        type = "mobile";

    regex regEmail("^[\\w-]+(\\.[\\w-]+)*@[\\w-]+(\\.[\\w-]+)+$");
    smatch smatchEmail;

    if (regex_match(userName, smatchEmail, regEmail))
        type = "email";

    std::string pwd = QTalk::utils::UrlEncode(password);
    std::string postData = "strid=" + userName + "&password=" + pwd + "&type=" + type + "&terminalType=01&osType=12&port=5222&loginsource=1";
    auto callback = [&qvtStr](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson retDta = Json::parse(responseData);

            if (nullptr == retDta)
            {
                error_log("json json load error");
                return;
            }

            int errCode = Json::get<int>(retDta, "errcode");

            if (errCode == 200)
            {
                nJson array = Json::get<nJson>(retDta, "data");
                nJson QVTJson = Json::convert<nJson>(array[0]);
                std::string qcookie = Json::get<std::string>(QVTJson, "qcookie");
                std::string vcookie = Json::get<std::string>(QVTJson, "vcookie");
                std::string tcookie = Json::get<std::string>(QVTJson, "tcookie");
                nJson obj;
                nJson &data = obj["data"];
                data["qcookie"] = qcookie;
                data["vcookie"] = vcookie;
                data["tcookie"] = tcookie;
                qvtStr = obj.dump();
                PLAT.setQvt(qvtStr);
            }
        }
    };
    QTalk::HttpRequest req(url.str(), QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/x-www-form-urlencoded;";
    req.body = postData;
    addHttpRequest(req, callback);

    if (!qvtStr.empty())
        _pFileHelper->writeQvtToFile(qvtStr);

    return qvtStr;
}

void Communication::getQchatTokenByQVT(const std::string &qvt, std::map<std::string, std::string> &userMap)
{
    //    std::map<std::string,std::string> userMap;
    std::ostringstream url;
    url << NavigationManager::instance().getApiUrl()
        << "/http_gettk";
    auto uuid = utils::getMessageId();
    nJson gObj;
    gObj["macCode"] = uuid;
    gObj["plat"] = "pc";
    std::string postData = gObj.dump();
    auto callback = [&userMap, uuid](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson retDta = Json::parse(responseData);

            if (nullptr == retDta)
            {
                error_log("json json load error");
                return;
            }

            int errCode = Json::get<int>(retDta, "errcode");

            if (errCode == 0)
            {
                nJson data = Json::get<nJson>(retDta, "data");
                std::string userName = Json::get<std::string>(data, "username");
                std::string token = Json::get<std::string>(data, "token");
                std::string password = "{\"token\":{\"plat\":\"pc\", \"macCode\":\"" + uuid + "\", \"token\":\"" + token + "\"}}";
                userMap["name"] = userName;
                userMap["password"] = password;
            }
            else
                error_log(Json::get<std::string>(retDta, "errmsg"));
        }
    };
    QTalk::HttpRequest req(url.str(), QTalk::RequestMethod::POST);

    if (qvt.empty())
        return;

    nJson qvtJson = Json::get<nJson>(Json::parse(qvt), "data");

    if (nullptr == qvtJson)
        return;

    std::string qcookie = Json::get<std::string>(qvtJson, "qcookie");
    std::string vcookie = Json::get<std::string>(qvtJson, "vcookie");
    std::string tcookie = Json::get<std::string>(qvtJson, "tcookie");
    std::string requestHeaders = std::string("_q=") + qcookie + ";_v=" + vcookie + ";_t=" + tcookie;
    req.header["Content-Type"] = "application/json;";
    req.header["Cookie"] = requestHeaders;
    req.body = postData;
    addHttpRequest(req, callback);
    //    return userMap;
}

//
void Communication::getNewLoginToken(const std::string &u, const std::string &p, std::map<std::string, std::string> &info)
{
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/nck/qtlogin.qunar";
    std::string plaint = LogicManager::instance()->getLogicBase()->normalRsaEncrypt(p);
    nJson gObj;
    gObj["u"] = u;
    gObj["p"] = plaint;
    gObj["h"] = NavigationManager::instance().getDomain();
    gObj["mk"] = utils::getMessageId();
    std::string postData = gObj.dump();
    auto callback = [&info](int code, const std::string & responseData)
    {
        if (code == 200)
        {
            nJson retDta = Json::parse(responseData);

            if (nullptr == retDta)
                return;

            int errCode = Json::get<int>(retDta, "errcode");

            if (errCode == 0)
            {
                nJson data = Json::get<nJson>(retDta, "data");
                std::string u = Json::get<std::string>(data, "u");
                std::string t = Json::get<std::string>(data, "t");
                info["u"] = u;
                info["t"] = t;
            }
            else
                error_log(Json::get<std::string>(retDta, "errmsg"));
        }
    };
    QTalk::HttpRequest req(url.str(), QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/json;";
    req.body = postData;
    addHttpRequest(req, callback);
}

/**
 *
 * @param e
 */
void Communication::getGroupCardInfo(std::shared_ptr<QTalk::Entity::ImGroupInfo> &group)
{
    if (_pUserGroupManager)
    {
        std::string groupId = group->GroupId;
        // 获取最新
        QTalk::Entity::JID jid(groupId);
        std::string domian = jid.domainname();
        MapGroupCard mapGroupCard;
        mapGroupCard[domian].push_back(*group);
        bool ret = _pUserGroupManager->getGroupCard(mapGroupCard);

        if (ret)
        {
            LogicManager::instance()->getDatabase()->getGroupCardById(group);

            if (!group->HeaderSrc.empty())
            {
                std::string localHead = QTalk::GetHeadPathByUrl(group->HeaderSrc);

                if (!FileHelper::fileExist(localHead))
                    _pFileHelper->downloadFile(group->HeaderSrc, localHead, false);
            }

            std::shared_ptr<QTalk::StGroupInfo> info(new QTalk::StGroupInfo);
            info->groupId = groupId;
            info->name = group->Name;
            info->headSrc = group->HeaderSrc;
            info->title = group->Topic;
            info->desc = group->Introduce;
            CommMsgManager::onUpdateGroupInfo(info);
        }
    }
}

// 设置群管理员
void Communication::setGroupAdmin(const std::string &groupId, const std::string &nickName,
                                  const std::string &memberJid, bool isAdmin)
{
    LogicManager::instance()->getLogicBase()->setGroupAdmin(groupId, nickName, memberJid, isAdmin);
}

void Communication::removeGroupMember(const std::string &groupId,
                                      const std::string &nickName,
                                      const std::string &memberJid)
{
    LogicManager::instance()->getLogicBase()->removeGroupMember(groupId, nickName, memberJid);
}

/**
 * 退出群
 * @param groupId
 */
void Communication::quitGroup(const std::string &groupId)
{
    //
    LogicManager::instance()->getLogicBase()->quitGroup(groupId);
    // 移除置顶
    _pUserConfig->updateUserSetting(UserSettingMsg::EM_OPERATOR_CANCEL,
                                    "kStickJidDic",
                                    QTalk::Entity::UID(groupId).toStdString(),
                                    "{\"topType\":0,\"chatType\":1}");
}

/**
 * 销毁群处理
 * @param groupId
 */
void Communication::destroyGroup(const std::string &groupId)
{
    LogicManager::instance()->getLogicBase()->destroyGroup(groupId);
}

void Communication::reportDump(const std::string &ip, const std::string &id, const std::string &dumpPath, QInt64 crashTime)
{
    auto fun = [ip, id, dumpPath, crashTime](const std::string & url, const std::string &)
    {
        if (!url.empty())
        {
            // format timestamp
            struct tm *timeinfo;
            char buffer[80];
            time_t rawtime = crashTime / 1000;
            timeinfo = localtime(&rawtime);
            strftime(buffer, 80, "%F %T", timeinfo);
            std::string strTime(buffer);

            {
                // 发邮件
                std::string strSub = string("QTalk 2.0 崩溃信息 ");
                std::ostringstream strBody;
                strBody << "登录用户: " << PLAT.getSelfXmppId()
                        << " " << PLAT.getSelfName() << "<br/>"
                        << "版本号: " << PLAT.getClientVersion() << "<br/>"
                        << "版本id : " << id << "<br/>"
                        << "使用平台: " << PLAT.getPlatformStr() << "<br/>"
                        << "OS 信息: " << PLAT.getOSInfo() << "<br/>"
                        << "ip: " << ip << "<br/>"
                        << "crash at: " << strTime << "<br/>"
                        << "exec: " << PLAT.getExecuteName() << "<br/>"
                        << "dump文件: " << url;
                std::string error;
                std::vector<std::string> tos;
                {
                    // todo add email address
                }
                bool isSuccess = LogicManager::instance()->getLogicBase()->sendReportMail(
                                     tos, strSub, strBody.str(), true, error);
                UN_USED(isSuccess)
            }
        };
    };

    if (_pFileHelper)
        _pFileHelper->uploadLogFile(dumpPath, fun);
}

/**
 *
 * @param logPath
 */
void Communication::reportLog(const std::string &desc, const std::string &logPath)
{
    if (_pFileHelper)
    {
        auto fun = [desc, logPath](const std::string & url, const std::string &)
        {
            // 发送消息
            if (!url.empty())
            {
                {
#if !defined(_STARTALK)
                    // 发消息
                    std::ostringstream msgCont;
                    msgCont << "qtalk pc2.0 日志反馈 \n\n"
                            << "登录用户: " << PLAT.getSelfXmppId()
                            << " " << PLAT.getSelfName() << "\n"
                            << "版本号: " << PLAT.getClientVersion() << "\n"
                            << "编译时间: " << PLAT.get_build_date_time() << "\n"
                            << "使用平台: " << PLAT.getPlatformStr() << "\n"
                            << "OS 信息: " << PLAT.getOSInfo() << "\n"
                            << "描述信息: " << desc << " \n"
                            << "日志文件: " << url;
                    //
                    std::string error;
                    bool isSuccess = LogicManager::instance()->getLogicBase()->sendReportMessage(msgCont.str(), error);
                    debug_log("send message :{0}", isSuccess);
#endif
                }
                {
                    // 发邮件
                    std::string strSub = string("QTalk 2.0 反馈日志 ") + PLAT.getSelfXmppId();
                    std::ostringstream strBody;
                    strBody << "登录用户: " << PLAT.getSelfXmppId()
                            << " " << PLAT.getSelfName() << "<br/>"
                            << "版本号: " << PLAT.getClientVersion() << "<br/>"
                            << "使用平台: " << PLAT.getPlatformStr() << "<br/>"
                            << "OS 信息: " << PLAT.getOSInfo() << "<br/>"
                            << "描述信息: " << desc << " <br/>"
                            << "日志文件: " << url;
                    std::vector<std::string> tos;
                    {
                        //                        tos.push_back("xxx@xxx.com");
                        // todo add email address
                    }
                    std::string error;
                    bool isSuccess = LogicManager::instance()->getLogicBase()->sendReportMail(tos,
                                     strSub,
                                     strBody.str(),
                                     true,
                                     error);
                    bool isNotify = (desc != "@@#@@");

                    if (isNotify)
                        CommMsgManager::sendLogReportRet(isSuccess, isSuccess ? "日志反馈成功" : error);
                }
                //#endif
            }
        };

        if (_pFileHelper)
            _pFileHelper->uploadLogFile(logPath, fun);
    }
}

/**
 *
 */
void Communication::saveUserConfig()
{
    if (_pUserConfig)
    {
        std::string val = AppSetting::instance().saveAppSetting();
        //_pUserConfig->updateUserSettting(UserSettingMsg::EM_OPERATOR_SET, QTALK_2_0_CONFIG, PLAT.getPlatformStr(), val);
        LogicManager::instance()->getDatabase()->insertConfig(QTALK_2_0_CONFIG,
                PLAT.getPlatformStr(), val);
    }
}

void Communication::clearSystemCache()
{
    // 清理数据库
    LogicManager::instance()->getDatabase()->ClearDBData();

    // 清理配置文件
    if (_pFileHelper)
        _pFileHelper->clearConfig();
}

void Communication::removeSession(const string &peerId)
{
    std::vector<std::string> peers;
    peers.push_back(peerId);
    LogicManager::instance()->getDatabase()->bulkDeleteSessions(peers);
}

/**
 *
 * @param head
 */
void Communication::changeUserHead(const string &head)
{
    std::thread([this, head]()
    {
#ifdef _MACOS
        pthread_setname_np("communication changeUserHead thread");
#endif

        if (_pFileHelper && _pUserManager)
        {
            std::string headUrl = _pFileHelper->getNetImgFilePath(head);

            if (headUrl.empty()) //todo return error message
                return;

            bool ret = _pUserManager->changeUserHead(headUrl);
            std::string localHead;

            if (ret)
                localHead = _pFileHelper->getLocalImgFilePath(headUrl, "/image/headphoto/");

            CommMsgManager::changeHeadRetMessage(ret, localHead);
        }
    }).detach();
}

/**
 *
 * @param loginTime
 * @param logoutTime
 */
void Communication::sendUserOnlineState(const QInt64 &loginTime, const QInt64 &logoutTime, const std::string &ip)
{
    std::thread([this, loginTime, logoutTime, ip]()
    {
#ifdef _MACOS
        pthread_setname_np("communication sendUserOnlineState thread");
#endif
        std::ostringstream url;
        url << NavigationManager::instance().getHttpHost()
            << "/statics/pc_statics.qunar"
            << "?v=" << PLAT.getClientVersion()
            << "&p=" << PLAT.getPlatformStr()
            << "&u=" << PLAT.getSelfUserId()
            << "&k=" << PLAT.getServerAuthKey()
            << "&d=" << PLAT.getSelfDomain();
        nJson obj;
        obj["username"] = PLAT.getSelfUserId();
        obj["host"] = PLAT.getSelfDomain();
        obj["resource"] = PLAT.getSelfResource();
        obj["platform"] = PLAT.getPlatformStr();
        obj["login_time"] = std::to_string(loginTime);
        obj["logout_at"] = std::to_string(logoutTime);
        obj["ip"] = ip;
        std::string postData = obj.dump();
        auto callback = [](int code, const std::string & responsData)
        {
            if (code == 200)
                debug_log("sendUserOnlineState success {0}", responsData);
            else
                error_log("sendUserOnlineState error {0}", responsData);
        };
        HttpRequest req(url.str(), RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        addHttpRequest(req, callback);
    }).detach();
}

/**
 *
 * @param ip
 * @param desc
 * @param reportTime
 */
void Communication::sendOperatorStatistics(const std::string &, const std::vector<QTalk::StActLog> &operators)
{
    std::string uploadLog = NavigationManager::instance().getUploadLog();

    if (uploadLog.empty())
        return;

    std::vector<QTalk::StActLog> logs(operators);

    do
    {
        nJson obj;
        // user
        nJson user;
        user["uid"] = PLAT.getSelfUserId();
        user["domain"] = PLAT.getSelfDomain();
        user["nav"] = PLAT.getLoginNav();
        obj["user"] = user;
        // device
        nJson device;
#if defined(_STARTALK)
        device["plat"] = "starTalk";
#else
        device["plat"] = "qtalk";
#endif
        device["os"] = PLAT.getPlatformStr();
        device["versionCode"] = PLAT.getClientNumVerison();
        device["versionName"] = PLAT.getGlobalVersion();
        device["osModel"] = PLAT.getOSInfo();
        device["osBrand"] = PLAT.getOSProductType();
        device["osVersion"] = PLAT.getOSVersion();
        obj["device"] = device;
        nJson infos;
        int i = 0;

        while (i++ <= 100 && !logs.empty())
        {
            const auto &log = logs.back();
            nJson info;
            info["describtion"] = log.desc;
            info["costTime"] = 0;
            info["isMainThread"] = true;
            info["reportTime"] = std::to_string(log.operatorTime);
            info["sql"] = "[]";
            info["threadId"] = 1;
            info["subType"] = "click";
            info["threadName"] = "main";
            info["type"] = "ACT";
            logs.pop_back();
            infos.push_back(info);
        }

        obj["infos"] = infos;
        std::string postData = obj.dump();
        auto callback = [](int code, const std::string & responsData)
        {
            if (code == 200)
                debug_log("sendOperatorStatistics success {0}", responsData);
            else
                error_log("sendOperatorStatistics error {0}", responsData);
        };
        HttpRequest req(uploadLog, RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        addHttpRequest(req, callback);
    }
    while (!logs.empty());
}

/**
 *
 * @param info
 */
void Communication::getUserCard(std::shared_ptr<QTalk::Entity::ImUserInfo> &info)
{
    if (info && _pUserManager)
    {
        QTalk::Entity::JID jid(info->XmppId);
        UserCardParam params;
        params[jid.domainname()][jid.username()] = 0;
        std::vector<QTalk::StUserCard> userCards;
        _pUserManager->getUserCard(params, userCards);

        if (userCards.size() == 1)
        {
            auto card = userCards[0];
            info->Name = card.userName;
            info->HeaderSrc = card.headerSrc;
            info->NickName = card.nickName;
            //
            LogicManager::instance()->getDatabase()->setUserCardInfo(userCards);
        }
    }
}

/**
 * 最近session
 * @param sessions
 */
void Communication::getRecntSession(std::vector<QTalk::StShareSession> &sessions)
{
    try
    {
        LogicManager::instance()->getDatabase()->getRecentSession(sessions);
    }
    catch (const std::exception &e)
    {
        error_log(e.what());
    }
}

/**
 *
 * @param sessions
 */
void Communication::geContactsSession(std::vector<QTalk::StShareSession> &sessions)
{
    try
    {
        LogicManager::instance()->getDatabase()->geContactsSession(sessions);
    }
    catch (const std::exception &e)
    {
        error_log(e.what());
    }
}

/**
 *
 */
void Communication::updateTimeStamp()
{
    try
    {
        QInt64 timeStamp = 0;
        std::string configTimeStamp;
        //
        auto *database = LogicManager::instance()->getDatabase();

        if (!database)
            return;

        database->getConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_TWOPERSONCHAT, configTimeStamp);

        if (configTimeStamp.empty())
            timeStamp = 0;
        else
            timeStamp = std::stoll(configTimeStamp);

        if (timeStamp == 0)
        {
            timeStamp = LogicManager::instance()->getDatabase()->getMaxTimeStampByChatType(QTalk::Enum::TwoPersonChat);
            LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_TWOPERSONCHAT,
                    std::to_string(timeStamp));
            info_log("{0} insert new timestamp {1} str:{2}", DEM_TWOPERSONCHAT, timeStamp, configTimeStamp);
        }
        else
            info_log("{0} has old timestamp {1} str:{2}", DEM_TWOPERSONCHAT, timeStamp, configTimeStamp);

        //
        configTimeStamp = "";
        LogicManager::instance()->getDatabase()->getConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_GROUPCHAT, configTimeStamp);

        if (configTimeStamp.empty())
            timeStamp = 0;
        else
            timeStamp = std::stoll(configTimeStamp);

        if (timeStamp == 0)
        {
            timeStamp = LogicManager::instance()->getDatabase()->getMaxTimeStampByChatType(QTalk::Enum::GroupChat);
            LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_GROUPCHAT,
                    std::to_string(timeStamp));
            warn_log("{0} insert new timestamp {1} str:{2}", DEM_GROUPCHAT, timeStamp, configTimeStamp);
        }
        else
            warn_log("{0} has old timestamp {1} str:{2}", DEM_GROUPCHAT, timeStamp, configTimeStamp);

        //
        configTimeStamp = "";
        LogicManager::instance()->getDatabase()->getConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_SYSTEM, configTimeStamp);

        if (configTimeStamp.empty())
            timeStamp = 0;
        else
            timeStamp = std::stoll(configTimeStamp);

        if (timeStamp == 0)
        {
            timeStamp = LogicManager::instance()->getDatabase()->getMaxTimeStampByChatType(QTalk::Enum::System);
            LogicManager::instance()->getDatabase()->insertConfig(DEM_MESSAGE_MAXTIMESTAMP, DEM_SYSTEM,
                    std::to_string(timeStamp));
            info_log("{0} insert new timestamp {1} str:{2}", DEM_SYSTEM, timeStamp, configTimeStamp);
        }
        else
            info_log("{0} has old timestamp {1} str:{2}", DEM_SYSTEM, timeStamp, configTimeStamp);
    }
    catch (const std::exception &e)
    {
        error_log("exception {0}", e.what());
    }
}

/**
 *
 * @param status
 */

void Communication::setServiceSeat(const int sid, const int seat)
{
    if (_pHotLinesConfig)
        _pHotLinesConfig->setServiceSeat(sid, seat);
}

void Communication::serverCloseSession(const std::string &username, const std::string &virtualname)
{
    if (_pHotLinesConfig)
        _pHotLinesConfig->serverCloseSession(username, virtualname);
}

void Communication::getSeatList(const QTalk::Entity::UID &uid)
{

}

void Communication::sendProduct(const std::string &username, const std::string &virtualname,
                                const std::string &product, const std::string &type)
{
    if (_pHotLinesConfig)
        _pHotLinesConfig->sendProduct(username, virtualname, product, type);
}

void Communication::sessionTransfer(const QTalk::Entity::UID &uid, const std::string &newCser,
                                    const std::string &reason)
{
    if (_pHotLinesConfig)
        _pHotLinesConfig->transferCsr(uid, newCser, reason);
}

void Communication::sendWechat(const QTalk::Entity::UID &uid)
{
    if (_pHotLinesConfig)
        _pHotLinesConfig->sendWechat(uid);
}

/**
 *
 * @param groupId
 * @param memberId
 * @param affiliation
 */
void Communication::onUserJoinGroup(const std::string &groupId, const std::string &memberId, int affiliation)
{
    std::shared_ptr<QTalk::StGroupMember> member(new QTalk::StGroupMember);
    member->groupId = groupId;
    //
    const std::string &jid = memberId;

    // 自己被拉入群 拉取群资料
    if (jid == PLAT.getSelfXmppId())
    {
        MapGroupCard mapGroups;
        _pUserGroupManager->getUserGroupInfo(mapGroups);
        _pUserGroupManager->getGroupCard(mapGroups);
        std::shared_ptr<QTalk::Entity::ImGroupInfo> group(new QTalk::Entity::ImGroupInfo);
        group->GroupId = groupId;

        if (LogicManager::instance()->getDatabase()->getGroupCardById(group))
        {
            std::shared_ptr<QTalk::StGroupInfo> info(new QTalk::StGroupInfo);
            info->groupId = groupId;
            info->name = group->Name;
            info->headSrc = group->HeaderSrc;
            info->title = group->Topic;
            info->desc = group->Introduce;
            CommMsgManager::onUpdateGroupInfo(info);
        }
    }

    member->memberJid = jid;
    std::map<std::string, QUInt8> members;
    members[jid] = static_cast<unsigned char>(affiliation);
    LogicManager::instance()->getDatabase()->bulkInsertGroupMember(groupId, members);
    std::shared_ptr<QTalk::Entity::ImUserInfo> userInfo = LogicManager::instance()->getDatabase()->getUserInfoByXmppId(
                jid);

    if (userInfo && !userInfo->Name.empty())
        member->nick = userInfo->Name;

    member->domain = QTalk::Entity::JID(memberId).domainname();
    member->affiliation = affiliation;
    CommMsgManager::onGroupJoinMember(member);
}

void Communication::onStaffChanged()
{
    if (_pUserManager)
        _pUserManager->getNewStructure(true);
}

//
void Communication::checkUpdater(int ver)
{
	// check update
    // todo
    // after download new setupfile and call : CommMsgManager::onCheckUpdate(!file_link.empty(), force);

    return;
}

//
void Communication::getMedalList()
{
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/medal/medalList.qunar";
    std::string strUrl = url.str();
    //
    std::string strVer;
    LogicManager::instance()->getDatabase()->getConfig("MEDAL_LIST", "VERSION", strVer);
    auto version = atoi(strVer.data());
    nJson obj;
    obj["version"] = version;
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImMedalList> medalList;
    std::set<std::string> imgs;
    int newVersion = 0;
    auto call_back = [&medalList, &newVersion, &imgs](int code, const std::string & resData)
    {
        if (resData.empty())
            return;

        nJson json = Json::parse(resData);

        if (nullptr == json)
        {
            error_log("json Parse error {0}", resData);
            return;
        }

        if (code == 200)
        {
            //
            bool ret = Json::get<bool>(json, "ret");

            if (ret)
            {
                nJson data = Json::get<nJson>(json, "data");
                newVersion = Json::get<int>(data, "version");
                //
                nJson list = Json::get<nJson>(data, "medalList");

                for (auto &temp : list)
                {
                    QTalk::Entity::ImMedalList medal;
                    medal.medalId = Json::get<int>(temp, "id");
                    medal.medalName = Json::get<std::string>(temp, "medalName");
                    medal.obtainCondition = Json::get<std::string>(temp, "obtainCondition");
                    medal.status = Json::get<int>(temp, "status");
                    nJson icon = Json::get<nJson>(temp, "icon");
                    medal.smallIcon = Json::get<std::string>(icon, "small");
                    medal.bigLightIcon = Json::get<std::string>(icon, "bigLight");
                    //                    medal.bigGrayIcon = Json::get<std::string >(icon, "bigGray");
                    medal.bigLockIcon = Json::get<std::string>(icon, "bigLock");
                    medalList.push_back(medal);
                    imgs.insert(medal.smallIcon);
                    imgs.insert(medal.bigLightIcon);
                    imgs.insert(medal.bigLockIcon);
                };
            }
            else
            {
                std::string errorMsg = Json::get<std::string>(json, "errmsg");
                error_log("getMedalList error {0}", errorMsg);
            }
        }
        else
        {
        }
    };
    QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/json;";
    req.body = postData;
    addHttpRequest(req, call_back);

    if (newVersion >= version && !medalList.empty())
    {
        try
        {
            //
            LogicManager::instance()->getDatabase()->insertMedalList(medalList);
            //
            LogicManager::instance()->getDatabase()->insertConfig("MEDAL_LIST", "VERSION", std::to_string(newVersion));
        }
        catch (const std::exception &e)
        {
            error_log("update medal list error {0}", e.what());
        }
    }

    //
    std::vector<QTalk::Entity::ImMedalList> medals;
    LogicManager::instance()->getDatabase()->getMedalList(medals);

    if (!medals.empty())
        DB_PLAT.setMedals(medals);

    if (!imgs.empty() && _pFileHelper)
    {
        for (const auto &img : imgs)
            _pFileHelper->getLocalImgFilePath(img, "/image/medal/", false);
    }
}

void Communication::getUserMedal(bool presence)
{
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/medal/userMedalList.qunar";
    std::string strUrl = url.str();
    //
    std::string strVer;
    LogicManager::instance()->getDatabase()->getConfig("USER_MEDAL", "VERSION", strVer);
    auto version = atoi(strVer.data());
    nJson obj;
    obj["version"] = version;
    //    obj["userId"] = PLAT.getSelfUserId();
    //    obj["host"] = PLAT.getSelfDomain();
    std::string postData = obj.dump();
    //
    std::vector<QTalk::Entity::ImUserStatusMedal> userMedals;
    int newVersion = 0;
    auto call_back = [&userMedals, &newVersion](int code, const std::string & resData)
    {
        if (resData.empty())
            return;

        nJson json = Json::parse(resData);

        if (nullptr == json)
        {
            error_log("json Parse error {0}", resData);
            return;
        }

        if (code == 200)
        {
            //
            bool ret = Json::get<bool>(json, "ret");

            if (ret)
            {
                nJson data = Json::get<nJson>(json, "data");
                newVersion = Json::get<int>(data, "version");
                //
                nJson list = Json::get<nJson>(data, "userMedals");

                for (auto &temp : list)
                {
                    QTalk::Entity::ImUserStatusMedal medal;
                    medal.medalId = Json::get<int>(temp, "medalId");
                    medal.userId = Json::get<std::string>(temp, "userId");
                    medal.host = Json::get<std::string>(temp, "host");
                    medal.medalStatus = Json::get<int>(temp, "medalStatus");
                    medal.mappingVersion = Json::get<int>(temp, "mappingVersion");
                    medal.updateTime = Json::get<long long>(temp, "updateTime");
                    userMedals.push_back(medal);
                };
            }
            else
            {
                std::string errorMsg = Json::get<std::string>(json, "errmsg");
                error_log("getMedalList error {0}", errorMsg);
            }
        }
        else
        {
        }
    };
    QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/json;";
    req.body = postData;
    addHttpRequest(req, call_back);

    if (newVersion > version && !userMedals.empty())
    {
        try
        {
            //
            LogicManager::instance()->getDatabase()->insertMedals(userMedals);
            //
            LogicManager::instance()->getDatabase()->insertConfig("USER_MEDAL", "VERSION", std::to_string(newVersion));
            CommMsgManager::onUserMadelChanged(userMedals);
        }
        catch (const std::exception &e)
        {
            error_log("update medal list error {0}", e.what());
        }
    }
}

void Communication::getMedalUser(int medalId, std::vector<QTalk::StMedalUser> &metalUsers)
{
    try
    {
        LogicManager::instance()->getDatabase()->getMedalUsers(medalId, metalUsers);
    }
    catch (const std::exception &e)
    {
        error_log("getMedalUser error", e.what());
    }
}

bool Communication::modifyUserMedalStatus(int medalId, bool wear)
{
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/medal/userMedalStatusModify.qunar";
    std::string strUrl = url.str();
    //
    nJson obj;
    obj["medalStatus"] = wear ? 3 : 1;
    obj["userId"] = PLAT.getSelfUserId();
    obj["host"] = PLAT.getSelfDomain();
    obj["medalId"] = medalId;
    std::string postData = obj.dump();
    //
    bool ret = false;
    auto call_back = [&ret](int code, const std::string & resData)
    {
        if (resData.empty())
            return;

        nJson json = Json::parse(resData);

        if (nullptr == json)
        {
            error_log("json Parse error {0}", resData);
            return;
        }

        ret = Json::get<bool>(json, "ret");

        if (code != 200 || !ret)
        {
            std::string errorMsg = Json::get<std::string>(json, "errmsg");
            error_log("userMedalStatusModify error {0}", errorMsg);
        }
    };
    QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
    req.header["Content-Type"] = "application/json;";
    req.body = postData;
    addHttpRequest(req, call_back);

    if (ret)
        LogicManager::instance()->getDatabase()->modifyUserMedalStatus(PLAT.getSelfXmppId(), medalId, wear ? 3 : 1);

    return ret;
}

void Communication::reportLogin()
{
    //
    auto now = time(nullptr);
    //
    std::ostringstream sId;
    sId << GLOBAL_INTERNAL_VERSION << "_" << build_time();
    nJson obj;
    obj["symbolFileId"] = sId.str();
    obj["resource"] = PLAT.getClientVersion();
    obj["platform"] = PLAT.getPlatformStr();
    obj["username"] = PLAT.getSelfUserId();
    obj["exec"] = PLAT.getExecuteName();
    obj["loginTime"] = now * 1000;
    std::string postData = obj.dump();
    //
    {
        std::ostringstream url;
        url << NavigationManager::instance().getHttpHost()
            << "/qtalkDump/upload_login_data.qunar";
        std::string strUrl = url.str();
        bool ret = false;
        auto call_back = [&ret](int code, const std::string & resData)
        {
            if (resData.empty())
                return;

            nJson json = Json::parse(resData);

            if (nullptr == json)
            {
                error_log("json Parse error {0}", resData);
                return;
            }

            ret = Json::get<bool>(json, "ret");

            if (code != 200 || !ret)
            {
                std::string errorMsg = Json::get<std::string>(json, "errmsg");
                error_log("userMedalStatusModify error {0}", errorMsg);
            }
        };
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        addHttpRequest(req, call_back);
    }
}
