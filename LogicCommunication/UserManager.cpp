#include "UserManager.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "../LogicManager/LogicManager.h"
#include "../Platform/NavigationManager.h"
#include "../Platform/Platform.h"
#include "../QtUtil/nJson/nJson.h"
#include "Communication.h"
#include "../QtUtil/Utils/Log.h"
#include "../Platform/dbPlatForm.h"

using namespace QTalk;

UserManager::UserManager(Communication *pComm)
        : _pComm(pComm) {

}


UserManager::~UserManager() = default;

/**
  * @函数名   getStructure
  * @功能描述 获取组织架构
  * @参数
  * @author   cc
  * @date     2018/09/29
  */
bool UserManager::getNewStructure(bool sendEvt) {
    //{"version":1905}

    // 获取组织架构版本号
    int maxVersion = 0;
    LogicManager::instance()->getDatabase()->getUserVersion(maxVersion);
    //
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/update/getUpdateUsers.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();

    nJson jsonObject;
    jsonObject["version"] = maxVersion;
    std::string postData = jsonObject.dump();
    bool retSts = false;
    std::vector<Entity::ImUserInfo> arUserInfo;
    std::vector<std::string> arDeletes;
    auto callback = [postData, &retSts, &arUserInfo, &arDeletes](int code, const std::string &responseData) {

        info_log("-- {}", responseData) ;

        if (code == 200) {
            nJson resData= Json::parse(responseData);

            if (resData == nullptr) {
                error_log("json paring error"); return;
            }

            bool ret = Json::get<bool>(resData, "ret");

            if (ret) {
                nJson data = Json::get<nJson >(resData, "data");

                int version = Json::get<int >(data, "version");
                bool invisible = Json::get<bool>(data, "invisible");
                PLAT.setShowStaff(!invisible);

                std::string domain = PLAT.getSelfDomain();
                // update
                nJson update= Json::get<nJson >(data, "update");
                for (auto & item : update)
                {
                    Entity::ImUserInfo user;

                    user.UserId = Json::get<std::string >(item, "U");
                    if(!user.UserId.empty() && user.UserId.find('@') != -1){
                        user.XmppId = user.UserId;
                    } else{
                        user.XmppId = user.UserId + "@" + domain;
                    }
                    user.Name = Json::get<std::string >(item, "N");
                    user.DescInfo = Json::get<std::string >(item, "D");
                    user.IncrementVersion = version;
                    user.SearchIndex = Json::get<std::string >(item, "pinyin");
                    user.Gender = Json::get<int >(item, "sex");
                    user.userType = Json::get<std::string >(item, "uType");
                    user.isVisible = Json::get<bool>(item, "visibleFlag", true);

                    arUserInfo.push_back(user);
                }
                // delete
                nJson deleteu= Json::get<nJson >(data, "delete");
                for (auto & item : deleteu) {
                    std::string userId = Json::get<std::string >(item, "U");
                    if (!userId.empty()) {
                        userId += "@" + domain;
                        arDeletes.push_back(userId);
                    }
                }
                retSts = true;
            }
            }
    };


    if (_pComm) {
        HttpRequest req(url.str(), RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (retSts) {

            LogicManager::instance()->getDatabase()->bulkInsertUserInfo(arUserInfo);
            LogicManager::instance()->getDatabase()->bulkDeleteUserInfo(arDeletes);
            //
            if(sendEvt && _pComm) {
                CommMsgManager::gotIncrementUser(arUserInfo, arDeletes);
            }
        }
    }

    return retSts;
}


/**
  * @函数名   getUserCard获取用户列表
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/09/29
  */
bool UserManager::getUserCard(const UserCardParam &param, std::vector<StUserCard> &arUserInfo) {
    //
    //[{"domain": "ejabhost1", "users" : [{"user": "chaocc.wang", "version" : 0}]}]

    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/domain/get_vcard_info.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();

    nJson objs;

    auto itObj = param.cbegin();
    for (; itObj != param.cend(); itObj++) {
        nJson obj;

        obj["domain"] = itObj->first.c_str();

        nJson users;
        for (const auto &itUsers : itObj->second) {
            nJson user;

            user["user"] = itUsers.first.c_str();
            user["version"] = itUsers.second;

            users.push_back(user);
        }

        obj["users"] = users;
        objs.push_back(obj);
    }

    std::string postData = objs.dump();

    std::string strUrl = url.str();
    debug_log("向服务器请求用户数据 url:{0} Json:{1}", strUrl.c_str(), postData);

    bool retSts = false;
    auto callback = [postData, strUrl, &retSts, &arUserInfo](int code, const std::string &responseData) {

        if (code == 200) {
            nJson data = Json::parse(responseData);

            if (data == nullptr) {
                error_log("json paring error"); return;
            }

            bool ret = Json::get<bool>(data, "ret");
            if (ret) {
                nJson msgList= Json::get<nJson >(data, "data");
                for (auto & item : msgList) {

                    std::string strdomain = Json::get<std::string >(item, "domain");

                    nJson users= Json::get<nJson >(item, "users");
                    for (auto user : users) {
                        StUserCard stUserCard;

                        stUserCard.xmppId = Json::get<std::string >(user, "username");
                        stUserCard.xmppId += "@" + strdomain;
                        stUserCard.headerSrc = Json::get<std::string >(user, "imageurl");
                        if(!stUserCard.headerSrc.empty() && stUserCard.headerSrc.find("http") != 0){
                            stUserCard.headerSrc = NavigationManager::instance().getFileHttpHost() + "/" +  stUserCard.headerSrc;
                        }
                        //先取webname 在取nickname
                        std::string webName = Json::get<std::string >(user, "webname");
                        if(!webName.empty()){
                            stUserCard.nickName = webName;
                        } else{
                            stUserCard.nickName = Json::get<std::string >(user, "nickname");
                        }
                        stUserCard.mood = Json::get<std::string >(user, "mood");
                        std::string gender = Json::get<std::string >(user, "gender");
                        stUserCard.gender = std::strtol(gender.data(), nullptr, 0);
                        std::string v = Json::get<std::string >(user, "V");
                        stUserCard.version = std::strtol(v.data(), nullptr, 0);

                        arUserInfo.push_back(stUserCard);
                    }
                }
                debug_log("从服务器请求到用户数据 个数:{0}", arUserInfo.size());
                retSts = true;
            }
            } else {
            warn_log("请求失败  url:{0}", strUrl);
        }
    };

    if (_pComm) {
        HttpRequest req(strUrl, RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (retSts) {
            retSts = LogicManager::instance()->getDatabase()->setUserCardInfo(arUserInfo);
            CommMsgManager::sendGotUserCard(arUserInfo);
        }
    }

    return retSts;
}

void UserManager::getUserFullInfo(std::shared_ptr<QTalk::Entity::ImUserSupplement> &imUserSup,
                                  std::shared_ptr<QTalk::Entity::ImUserInfo>& userInfo) {
    //
    debug_log("请求名片 userId:{0}", imUserSup->XmppId);
    // 强制从服务器获取最新卡片
    QTalk::Entity::JID jid(userInfo->XmppId);
    UserCardParam param;
    std::vector<StUserCard> arUserInfo;
    param[jid.domainname()][jid.username()] = 0;
    getUserCard(param, arUserInfo);
    if(arUserInfo.size() == 1)
    {
        auto info = arUserInfo[0];
        userInfo = DB_PLAT.getUserInfo(userInfo->XmppId);
        if(nullptr == userInfo)
        {
            userInfo = std::make_shared<QTalk::Entity::ImUserInfo>();
            userInfo->XmppId = jid.basename();
        }

        userInfo->Name = info.userName;
        userInfo->HeaderSrc = info.headerSrc;
        userInfo->NickName = info.nickName;
        userInfo->Gender = info.gender;
        userInfo->Mood = info.mood;
    }
    // head
    _pComm->downloadUserHeadByStUserCard(arUserInfo);
    // mood
//    if (!getUserMood(&jid, imUserSup->UserMood, imUserSup->UserMoodVersion)) {
//        imUserSup = nullptr;
//        return;
//    }
    // leader
    if (!getUserSupplement(&jid, imUserSup)) {
        // 不存在的员工
    }

    if(imUserSup->MailAddr.empty())
        imUserSup->MailAddr = jid.username() + "@" + NavigationManager::instance().getMailSuffix();

    // 刷新显示mood
    std::string mood = userInfo->Mood;
    std::string id = userInfo->XmppId;
    std::thread([ id, mood](){
        CommMsgManager::updateMoodRet(id, mood);
    }).detach();
}

// leader and userno
bool
UserManager::getUserSupplement(QTalk::Entity::JID *jid, std::shared_ptr<QTalk::Entity::ImUserSupplement> &imUserSup) {

    debug_log("请求leader userId: {0}", jid->username());

    std::ostringstream url;
    url << NavigationManager::instance().getLeaderUrl();

    nJson obj;
    obj["platform"] = PLAT.getPlatformStr();
    obj["qtalk_id"] = jid->username();
    obj["user_id"] = jid->username();
    obj["ckey"] = PLAT.getClientAuthKey();

    std::string postData = obj.dump();

    std::cout << PLAT.getClientAuthKey() << std::endl;

    bool retSts = false;
    auto callback = [postData, jid, &retSts, &imUserSup](int code, const std::string& responsData) {
        if (code == 200) {
            nJson data = Json::parse(responsData);

            if (data == nullptr) {
                error_log("json paring error"); return;
            }

            int errcde = Json::get<int >(data, "errcode");
            if (errcde == 0) {
                nJson msg= Json::get<nJson >(data, "data");
                imUserSup->LeaderName = Json::get<std::string >(msg, "leader");
                imUserSup->LeaderId = Json::get<std::string >(msg, "qtalk_id");
                imUserSup->MailAddr = jid->username() + "@" + NavigationManager::instance().getMailSuffix();
                imUserSup->UserNo = Json::get<std::string >(msg, "sn");

                debug_log("请求mood success userId:{0}", jid->username());
                retSts = true;

            } else {
                std::string errMsg = Json::get<std::string >(data, "msg");
                debug_log("请求leader error userId:{0} msg: {1}", jid->username().c_str(), errMsg);
            }

            } else {
            //LOG_INFO
            //<< "请求leader error userId:" << jid->username().c_str() << "msg: " << responsData;
        }
    };

    if (_pComm) {
        HttpRequest req(url.str(), RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (retSts) {
            retSts = LogicManager::instance()->getDatabase()->insertOrUpdateUserSuppl(imUserSup);
        }
    }

    return retSts;
}

// get phoneno
bool UserManager::getPhoneNo(const std::string &userId, std::string &phoneNo) {

    debug_log("请求PhoneNo userId: {0}", userId);

    auto *jid = new QTalk::Entity::JID(userId);

    std::ostringstream url;
    url << NavigationManager::instance().getPhoneNumAddr();

    std::string self_id = PLAT.getSelfUserId();

    nJson obj;
    obj["platform"] = PLAT.getPlatformStr().c_str();
    obj["qtalk_id"] = jid->username().c_str();
    obj["user_id"] = self_id.data();
    obj["ckey"] = PLAT.getClientAuthKey().c_str();

    std::string postData = obj.dump();

    bool ret = false;
    std::string errMsg;
    auto callback = [ postData, jid, &ret, &phoneNo, &errMsg](int code, const std::string &responseData) {

        if (code == 200) {
            nJson data = Json::parse(responseData);

            if (data == nullptr) {
                error_log("json paring error"); return;
            }

            int errcode = Json::get<int >(data, "errcode");
            if (errcode == 0) {
                nJson msg= Json::get<nJson >(data, "data");
                phoneNo = Json::get<std::string >(msg, "phone");
                ret = true;
            } else {
                errMsg = Json::get<std::string >(data, "msg");
                debug_log("getPhoneNo error userId:{0} msg: {1}", jid->username(), errMsg);
            }

            } else {
            //LOG_INFO << "请求PhoneNo error userId:" << jid->username().c_str() << "msg: " << responseData;
            return;
        }
    };

    if (_pComm) {
		HttpRequest req(url.str(), RequestMethod::POST);
		req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (ret) {
            ret = LogicManager::instance()->getDatabase()->insertOrUpdateUserPhoneNo(jid->basename(), phoneNo);
        }
        else
            phoneNo = errMsg;
    }

    return ret;
}

/**
 * 修改头像接口
 * @param headPath
 */
bool UserManager::changeUserHead(const std::string &headurl)
{
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/profile/set_profile.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();


    nJson obj;
    nJson objs;
    obj["user"] = PLAT.getSelfUserId().data();
    obj["domain"] = PLAT.getSelfDomain().data();
    obj["url"] = headurl.data();
    objs.push_back(obj);
    std::string postData = objs.dump();
    bool retSts = false;
    StUserCard userCard = StUserCard();
    auto callback = [postData, headurl, &retSts, &userCard](int code, const std::string& responsData) {

        if (code == 200) {
            nJson dataObj = Json::parse(responsData);

            if (dataObj == nullptr) {
                error_log("json paring error"); return;
            }

            int ret = Json::get<bool>(dataObj, "ret");
            if (ret) {
                nJson data = Json::get<nJson >(dataObj, "data");

                for(auto &item : data)
                {
                    userCard.xmppId = Json::get<std::string >(item, "user");
                    userCard.xmppId += Json::get<std::string >(item, "domain");
                    userCard.version = std::stoi(Json::get<std::string >(item, "version"));
                    userCard.headerSrc = Json::get<std::string >(item, "url");
                    if(!userCard.headerSrc.empty() && userCard.headerSrc.find("http") != 0){
                        userCard.headerSrc = NavigationManager::instance().getFileHttpHost() + "/" +  userCard.headerSrc;
                    }
                    userCard.mood = Json::get<std::string >(item, "mood");
                }
                retSts = true;
            }
            else
            {
                error_log("UpdateMood error {0}", Json::get<std::string >(dataObj, "errmsg"));
            }

        }
    };

    if (_pComm) {
        HttpRequest req(url.str(), RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (retSts) {
            std::vector<StUserCard> arUserInfos;
            arUserInfos.push_back(userCard);
            retSts = LogicManager::instance()->getDatabase()->setUserCardInfo(arUserInfos);
        }
    }

    return retSts;
}

/**
 *
 * @param mood
 * @return
 */
void UserManager::UpdateMood(const std::string &mood)
{
    QTalk::Entity::JID selfId(PLAT.getSelfXmppId().data());

    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/profile/set_profile.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << selfId.username()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << selfId.domainname();

    //
    nJson obj;
    nJson objs;
    obj["user"] = selfId.username().data();
    obj["domain"] = selfId.domainname().data();
    obj["mood"] = mood.data();
    objs.push_back(obj);
    std::string postData = objs.dump();
    bool retSts = false;
    std::vector<StUserCard> arUserInfos;
    auto callback = [postData, &retSts, &arUserInfos](int code, const std::string& responsData) {

        if (code == 200) {
            nJson dataObj = Json::parse(responsData);

            if (dataObj == nullptr) {
                error_log("json paring error"); return;
            }

            int ret = Json::get<bool>(dataObj, "ret");
            if (ret) {
                nJson data = Json::get<nJson >(dataObj, "data");

                for(auto &item : data)
                {
                    StUserCard userCard;
                    userCard.xmppId = Json::get<std::string >(item, "user");
                    userCard.xmppId += Json::get<std::string >(item, "domain");
                    userCard.version = std::stoi(Json::get<std::string >(item, "version"));
                    userCard.headerSrc = Json::get<std::string >(item, "url");
                    userCard.mood = Json::get<std::string >(item, "mood");
                    arUserInfos.push_back(userCard);
                }
                retSts = true;
            }
            else
            {
                error_log("UpdateMood error {0}", Json::get<std::string >(dataObj, "errmsg"));
            }

            }
    };

    if (_pComm) {
        HttpRequest req(url.str(), RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);

        if (retSts) {

            LogicManager::instance()->getDatabase()->setUserCardInfo(arUserInfos);
            //
            CommMsgManager::updateMoodRet(selfId.basename(), mood);
        }
    }
}
