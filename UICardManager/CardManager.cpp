#include <memory>
#include "CardManager.h"
#include "UserCard.h"
#include "GroupCard.h"
#include <QFile>
#include <QtConcurrent>
#include <QApplication>
#include "MsgManager.h"
#include "../UICom/uicom.h"
#include "../Platform/Platform.h"

CardManager::CardManager()
{
//    _pMsgManager = new UserCardMsgManager;
    _pMsgListener = new UserCardMessageListener(this);
    connect(this, &CardManager::showUserCardSignal, this, &CardManager::showUserCardSlot);
    connect(this, &CardManager::showGroupCardSignal, this, &CardManager::showGroupCardSlot);
    qRegisterMetaType<std::map<std::string, QTalk::StUserCard> >("std::map<std::string, QTalk::StUserCard>");
    qRegisterMetaType<std::map<std::string, QUInt8> >("std::map<std::string, QUInt8>");
    connect(this, &CardManager::sgUpdateGroupMember, this, &CardManager::updateGroupMember);
}

CardManager::~CardManager()
{
    if(_pMsgListener)
        delete _pMsgListener;
}

void CardManager::shwoUserCard(const QString &userId)
{
    static bool flag = false;

    if(flag) return;
    else flag = true;

    _pUserCard = new user_card(this);
    connect(_pUserCard, &user_card::sgJumpToStructre, [this](const QString & structreName)
    {
        emit sgSwitchCurFun(1);
        emit sgJumpToStructre(structreName);
        _pUserCard->close();
    });
    auto ret = QtConcurrent::run([this, userId]()
    {
        {
            std::lock_guard<QTalk::util::spin_mutex> lock(sm);
            _imuserSup = std::make_shared<QTalk::Entity::ImUserSupplement>();
            _userInfo = std::make_shared<QTalk::Entity::ImUserInfo>();
            _userInfo->XmppId = userId.toStdString();
            _imuserSup->XmppId = userId.toStdString();
            UserCardMsgManager::getUserCard(_imuserSup, _userInfo);
            //
            _user_medal.clear();
            UserCardMsgManager::getUserMedal(userId.toStdString(), _user_medal);
        }
//        if (nullptr != _imuserSup && _pMsgManager) {
//            emit showUserCardSignal();
//        }
    });
    _pUserCard->showCenter(false, nullptr);
    _pUserCard->raise();

    // wait data
//    ret.waitForFinished();
    while (!ret.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);

    // show user card
    showUserCardSlot();
    //
    flag = false;
}

/**
 *
 * @param groupId
 */
void CardManager::showGroupCard(const QString &groupId)
{
    static bool flag = false;

    if(flag) return;
    else flag = true;

    _groupCard = new GroupCard(this);
    auto ret = QtConcurrent::run([this, groupId]()
    {
        //
        _imGroupSup = std::make_shared<QTalk::Entity::ImGroupInfo>();
        _imGroupSup->GroupId = groupId.toStdString();
        UserCardMsgManager::getGroupMembers(groupId.toStdString());
        UserCardMsgManager::getGroupCard(_imGroupSup);
    });
    // wait data
//    ret.waitForFinished();
    _groupCard->showCenter(false, nullptr);
    _groupCard->raise();

    while (!ret.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);

    // show group card
    showGroupCardSlot();
    flag = false;
}

/**
 *
 */
void CardManager::showUserCardSlot()
{
    if (nullptr != _pUserCard)
    {
        std::string usrId = _imuserSup->XmppId;
//        std::lock_guard<QTalk::util::spin_mutex> lock(sm);
        int flags = _arStarContact.contains(usrId);
        flags |= _arBlackList.contains(usrId) << 1;
//        flags |= _arFriends.contains(usrId) << 2;
        _pUserCard->setFlags(flags);
        _pUserCard->showUserCard(_imuserSup, _userInfo);

        if (_mapMaskNames.contains(usrId))
            _pUserCard->setMaskName(QString::fromStdString(_mapMaskNames[usrId]));

        //
        _pUserCard->showMedal(_user_medal);
        //
//        _pUserCard->adjustSize();
//		_pUserCard->resize(_pUserCard->width() + 10, _pUserCard->height());
        QApplication::setActiveWindow(_pUserCard);
    }
}

void CardManager::getPhoneNo(const std::string &userId)
{
    QtConcurrent::run([this, userId]()
    {
        std::string phoneNo;
        std::lock_guard<QTalk::util::spin_mutex> lock(sm);
        UserCardMsgManager::getUserPhoneNo(userId, phoneNo);

        if (!phoneNo.empty())
            emit gotPhoneNo(userId, phoneNo);
    });
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/12/16
  */
void CardManager::updateUserConfig(const std::vector<QTalk::Entity::ImConfig> &arConfigs)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    QVector<std::string> oldStarContact = _arStarContact;
    QVector<std::string> oldBlackList = _arBlackList;
    _arStarContact.clear();
    _arBlackList.clear();
    _mapMaskNames.clear();
    auto it = arConfigs.begin();

    for (; it != arConfigs.end(); it++)
    {
        std::string subKey = it->ConfigSubKey;

        if (it->ConfigKey == "kStarContact")
        {
            if (oldStarContact.contains(subKey))
                oldStarContact.removeOne(subKey);

            //
            _arStarContact.push_back(subKey);
        }
        else if (it->ConfigKey == "kBlackList")
        {
            if (oldBlackList.contains(subKey))
                oldBlackList.removeOne(subKey);

            //
            _arBlackList.push_back(subKey);
        }
        else if (it->ConfigKey == "kMarkupNames")
            _mapMaskNames[subKey] = it->ConfigValue;
    }
}

void CardManager::updateUserConfig(const std::map<std::string, std::string> &deleteData,
                                   const std::vector<QTalk::Entity::ImConfig> &arImConfig)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);

    for(const auto &it : deleteData)
    {
        if (it.second == "kStarContact")
        {
            if (_arStarContact.contains(it.first))
                _arStarContact.removeOne(it.first);
        }
        else if (it.second == "kBlackList")
        {
            if (_arBlackList.contains(it.first))
                _arBlackList.removeOne(it.first);
        }
        else if (it.second == "kMarkupNames")
        {
            if (_mapMaskNames.contains(it.first))
                _mapMaskNames.remove(it.first);
        }
    }

    for(const auto &it : arImConfig)
    {
        if (it.ConfigKey == "kStarContact")
        {
            if (!_arStarContact.contains(it.ConfigSubKey))
                _arStarContact.push_back(it.ConfigSubKey);
        }
        else if (it.ConfigKey == "kBlackList")
        {
            if (!_arBlackList.contains(it.ConfigSubKey))
                _arBlackList.push_back(it.ConfigSubKey);
        }
        else if (it.ConfigKey == "kMarkupNames")
            _mapMaskNames[it.ConfigSubKey] = it.ConfigValue;
    }
}

///**
// *
// * @param friends
// */
//void CardManager::onRecvFriends(const std::vector<QTalk::Entity::IMFriendList> &friends) {
//    _arFriends.clear();
//
//    for (const QTalk::Entity::IMFriendList& imfriend : friends) {
//        _arFriends.push_back(imfriend.XmppId);
//    }
//}

/**
 * 星标联系人
 * @param userId
 */
void CardManager::starUser(const std::string &userId)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    bool isStar = _arStarContact.contains(userId);
    QtConcurrent::run([ userId, isStar]()
    {
        QString val = QString::number(isStar ? 0 : 1);
        UserCardMsgManager::setUserSetting(isStar, "kStarContact", userId, val.toStdString());
    });
}

/**
 * 加入黑名单
 */
void CardManager::addBlackList(const std::string &userId)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    bool isBlack = _arBlackList.contains(userId);
    QtConcurrent::run([ isBlack, userId]()
    {
        QString val = QString::number(isBlack ? 0 : 1);
        UserCardMsgManager::setUserSetting(isBlack, "kBlackList", userId, val.toStdString());
    });
}

/**
 *
 */
void CardManager::showGroupCardSlot()
{
    if (nullptr != _groupCard && nullptr != _imGroupSup)
    {
        _groupCard->setData(_imGroupSup);
        QApplication::setActiveWindow(_groupCard);
    }
}

/**
 *
 * @param groupId
 * @param userCards
 */
void CardManager::onRecvGroupMember(const std::string &groupId,
                                    const std::map<std::string, QTalk::StUserCard> &userCards,
                                    const std::map<std::string, QUInt8> &userRole)
{
    if (nullptr != _imGroupSup && _imGroupSup->GroupId == groupId)
        emit sgUpdateGroupMember(userCards, userRole);
}

/**
 *
 */
void CardManager::updateGroupMember(std::map<std::string, QTalk::StUserCard> userCards, std::map<std::string, QUInt8> userRole)
{
    if (_groupCard)
        _groupCard->showGroupMember(userCards, userRole);
}

void CardManager::updateGroupInfo(const QString &groupId, const QString &groupName, const QString &topic,
                                  const QString &desc)
{
    QtConcurrent::run([ = ]()
    {
        std::shared_ptr<QTalk::StGroupInfo> groupinfo(new QTalk::StGroupInfo);
        groupinfo->groupId = groupId.toStdString();
        groupinfo->name = groupName.toStdString();
        groupinfo->desc = desc.toStdString();
        groupinfo->title = topic.toStdString();
        UserCardMsgManager::updateGroupInfo(groupinfo);
    });
}

/**
 *
 * @param groupi
 */
void CardManager::quitGroup(const QString &groupId)
{
    QtConcurrent::run([groupId]()
    {
        UserCardMsgManager::quitGroup(groupId.toStdString());
    });
}

/**
 *
 * @param groupId
 */
void CardManager::destroyGroup(const QString &groupId)
{
    QtConcurrent::run([groupId]()
    {
        UserCardMsgManager::destroyGroupMsg(groupId.toStdString());
    });
}

void CardManager::setUserMaskName(const std::string &userId, const std::string &maskName)
{
    QtConcurrent::run([ userId, maskName]()
    {
        UserCardMsgManager::setUserSetting(false, "kMarkupNames", userId, maskName);
    });
}

void CardManager::setUserMood(const std::string &mood)
{
    QtConcurrent::run([ = ]()
    {
        UserCardMsgManager::updateMood(mood);
    });
}

std::string CardManager::getSourceHead(const std::string &headLink)
{
    return UserCardMsgManager::getSourceImage(headLink);
}
