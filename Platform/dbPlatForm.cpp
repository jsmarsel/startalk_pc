//
// Created by cc on 2019-01-18.
//

#include "dbPlatForm.h"
#include "../LogicManager/LogicManager.h"
#include "../QtUtil/nJson/nJson.h"

dbPlatForm& dbPlatForm::instance() {
    static dbPlatForm platform;
    return platform;
}

//int dbPlatForm::getAtCount(const std::string & id)
//{
//    int count = 0;
//    //LogicManager::instance()->GetDatabase()->getAtCount(id, count);
//    return count;
//}
//
//int dbPlatForm::getAllUnreadCount()
//{
//    int count = 0;
//    LogicManager::instance()->getDatabase()->getAllUnreadCount(count);
//    return count;
//}


std::shared_ptr<QTalk::Entity::ImUserInfo> dbPlatForm::getUserInfo(const std::string &xmppId, const bool &readDb) {
    std::string userId = xmppId;
    auto indx = xmppId.find('/');
    if(indx != std::string::npos)
        userId = userId.substr(0, indx);

    std::shared_ptr<QTalk::Entity::ImUserInfo> info = _userInfoMap[xmppId];
    if (info && !readDb) {
        return info;
    } else // 如果内存中没有则去数据库中查找
    {
        std::shared_ptr<QTalk::Entity::ImUserInfo> userinfo =
                LogicManager::instance()->getDatabase()->getUserInfoByXmppId(xmppId);
        _userInfoMap[xmppId] = userinfo;
        return userinfo;
    }
}


std::shared_ptr<QTalk::Entity::ImGroupInfo> dbPlatForm::getGroupInfo(const std::string &xmppId, const bool &readDb) {
    std::shared_ptr<QTalk::Entity::ImGroupInfo> info = _groupInfoMap[xmppId];
    if (info && !readDb) {
        return info;
    } else // 如果内存中没有则去数据库中查找
    {
        std::shared_ptr<QTalk::Entity::ImGroupInfo> groupinfo =
                LogicManager::instance()->getDatabase()->getGroupInfoByXmppId(xmppId);
        _groupInfoMap[xmppId] = groupinfo;
        return groupinfo;
    }
    return nullptr;
}

//std::shared_ptr<std::vector<std::shared_ptr<QTalk::Entity::ImSessionInfo> > > dbPlatForm::QueryImSessionInfos() {
//    return LogicManager::instance()->getDatabase()->QueryImSessionInfos();
//}

/**
 *
 * @param groups
 */
void dbPlatForm::getAllGroup(std::vector<QTalk::Entity::ImGroupInfo> &groups)
{
    LogicManager::instance()->getDatabase()->getAllGroup(groups);
}

bool dbPlatForm::isHotLine(const std::string &jid) {

    if(_hotLines.empty())
    {
        std::string strHotLines;
        LogicManager::instance()->getDatabase()->getHotLines(strHotLines);

        if(strHotLines.empty())
            return false;

        nJson obj = Json::parse(strHotLines);
        if(nullptr != obj ) {
            bool ret = obj["ret"];
            if(ret){
                nJson data = Json::get<nJson >(obj, "data");
                auto allHotLines = Json::get<nJson>(data, "allhotlines");

                for(const nJson & item : allHotLines) {
                    _hotLines.insert(Json::convert<std::string>(item, ""));
                }
            }
        }
    }

    return _hotLines.find(jid) != _hotLines.end();
}

std::shared_ptr<std::vector<std::shared_ptr<QTalk::Entity::ImSessionInfo> > > dbPlatForm::reloadSession() {
    return LogicManager::instance()->getDatabase()->reloadSession();
}

std::vector<QTalk::StUserCard> dbPlatForm::getGroupMemberInfo(const std::vector<std::string> &arMembers) {
    std::vector<QTalk::StUserCard> ret;
    if(!arMembers.empty())
    {
        LogicManager::instance()->getDatabase()->getGroupMemberInfo(arMembers, ret);
    }
    return ret;
}

/**
 *
 * @param masks
 */
void dbPlatForm::setMaskNames(const std::map<std::string, std::string> &masks)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    mask_names = masks;
}

std::string dbPlatForm::getMaskName(const std::string& xmppId) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    auto itFind = mask_names.find(xmppId);
    if(itFind != mask_names.end())
        return itFind->second;
    else
        return std::string();
}

//
void dbPlatForm::setHotLines(const std::set<std::string> &hotlines) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _hotLines = hotlines;
}

void dbPlatForm::setMedals(const std::vector<QTalk::Entity::ImMedalList> &medals) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _medals = medals;
}

QTalk::Entity::ImMedalList dbPlatForm::getMedal(const int &id) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    auto itFind = std::find_if(_medals.begin(), _medals.end(), [id](const QTalk::Entity::ImMedalList& medal){
        return medal.medalId == id;
    });

    if(itFind == _medals.end())
        return QTalk::Entity::ImMedalList();
    else
        return *itFind;
}

void dbPlatForm::getAllMedals(std::vector<QTalk::Entity::ImMedalList> &medals) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    medals = this->_medals;
}