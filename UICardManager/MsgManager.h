//
// Created by cc on 18-11-6.
//

#ifndef QTALK_V2_MSGMANAGER_H
#define QTALK_V2_MSGMANAGER_H

#include "../EventBus/Object.hpp"
#include "../Message/UserMessage.h"
#include "../EventBus/EventHandler.hpp"
#include "../EventBus/HandlerRegistration.hpp"
#include "../entity/im_group.h"
#include "../Message/GroupMessage.h"
#include <string>
#include <set>
#include <memory>

class CardManager;
class UserCardMsgManager : public Object
{
public:
    UserCardMsgManager();
    ~UserCardMsgManager() override;

public:
    static void getUserCard(std::shared_ptr<QTalk::Entity::ImUserSupplement>&,
            std::shared_ptr<QTalk::Entity::ImUserInfo>&);
    static void getUserMedal(const std::string& xmppId, std::set<QTalk::StUserMedal>& medal);
    static void getUserPhoneNo(const std::string& userId, std::string& phoneNo);
    static void setUserSetting(bool isSetting, const std::string& key, const std::string& subKey, const std::string& value);
    static void getGroupCard(std::shared_ptr<QTalk::Entity::ImGroupInfo>&);
    static void getGroupMembers(const std::string& groupId);
    static void updateGroupInfo(std::shared_ptr<QTalk::StGroupInfo> groupinfo);
    static void quitGroup(const std::string &groupId);
    static void destroyGroupMsg(const std::string& groupId);
    static void updateMood(const std::string& mood);
    static std::string getSourceImage(const std::string& netPath);
    static void getMedalUser(int medalId, std::vector<QTalk::StMedalUser>& metalUsers);
    static bool modifyUserMedal(int medalId, bool wear);
};

class UserCardMessageListener : public EventHandler<UpdateUserConfigMsg>
//        , public EventHandler<AllFriends>
        , public EventHandler<UpdateGroupMember>
        , public EventHandler<GroupMemberMessage>
        , public EventHandler<IncrementConfig>
{
public:
    explicit UserCardMessageListener(CardManager* mainPanel);
    ~UserCardMessageListener() override;

protected:
    void onEvent(UpdateUserConfigMsg &e) override;
//    void onEvent(AllFriends &e) override;
    void onEvent(UpdateGroupMember &e) override;
    void onEvent(GroupMemberMessage &e) override;
    void onEvent(IncrementConfig &e) override;

private:
    CardManager* _pMainPanel;


					};
#endif //QTALK_V2_MSGMANAGER_H
