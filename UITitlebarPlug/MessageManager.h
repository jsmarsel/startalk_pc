﻿#ifndef _MESSAGEMANAGER_H_
#define _MESSAGEMANAGER_H_

#include "../EventBus/EventHandler.hpp"
#include "../Message/LoginMessgae.h"
#include "../EventBus/HandlerRegistration.hpp"
#include "../Message/UserMessage.h"
#include "../Message/ChatMessage.h"
#include <functional>

class SearchInfoEvent;
class TitlebarMsgManager : public Object
{
public:
	static void getUserCard(const std::string &domain, const std::string &userName, const int &version);
	static std::string getHeadPath(const std::string& netPath);
    static void sendSearch(SearchInfoEvent & event);
	static void saveConfig();
	static void clearSystemCache();
	static void changeUserHead(const std::string& headpath);

	static void chanegUserStatus(const std::string& status);
	static void setServiceSeat(int sid,int seat);

    static std::string uploadImage(const std::string &localFilePath);
    //
    static void sendPostReq(const std::string &url, const std::string &params,
                     std::function<void(int code, const std::string &responseData)> callback);
};

class MainPanel;
class TitlebarMsgListener : public EventHandler<UserCardMessgae>, public EventHandler<ChangeHeadRetMessage>
        , public EventHandler<SwitchUserStatusRet>
{
public:
    explicit TitlebarMsgListener(MainPanel *pMainPanel);
    ~TitlebarMsgListener() override;

public:
	void onEvent(UserCardMessgae& e) override;
	void onEvent(ChangeHeadRetMessage& e) override;
	void onEvent(SwitchUserStatusRet& e) override;

private:
	MainPanel* _pMainPanel;
};

#endif//_MESSAGEMANAGER_H_
