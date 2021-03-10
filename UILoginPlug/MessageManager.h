﻿#ifndef _MESSAGEMANAGER_H_
#define _MESSAGEMANAGER_H_

#include <string>
#include "../EventBus/Object.hpp"
#include "../Message/LoginMessgae.h"
#include "../EventBus/EventHandler.hpp"
#include "../EventBus/HandlerRegistration.hpp"
#include "../Message/StatusMessage.h"
#include "../Message/GroupMessage.h"
#include "../Message/ChatMessage.h"

class UILoginMsgManager : public Object
{
public:
	static bool SendLoginMessage(const std::string& userName, const std::string& password);
	static bool getNavInfo(const std::string& navAddr);
    static std::string getNavDomain(const std::string& addr);
	static std::map<std::string,std::string> getQchatToken(const std::string &qvt);
	static std::string getQchatQvt();
	static bool saveQvtToDB(const std::string& qvt);
    static void startUpdater(const std::string& users);
};

//
class LoginPanel;
class UILoginMsgListener : public EventHandler<LoginSuccessMessage>, public EventHandler<SynOfflineSuccees>
		,public EventHandler<AuthFailed>
		,public EventHandler<GotStructureMessage>, public EventHandler<GotUserGroup>
		,public EventHandler<LoginErrMessage>, public EventHandler<ChangeHeadRetMessage>
		,public EventHandler<LoginProcessMessage>
{
public:
	explicit UILoginMsgListener(LoginPanel* pLoginPanel);
	~UILoginMsgListener() override;

public:
	void onEvent(LoginSuccessMessage& e) override;
    void onEvent(SynOfflineSuccees& e) override;
    void onEvent(AuthFailed& e) override;
	void onEvent(GotStructureMessage& e) override;
	void onEvent(GotUserGroup& e) override;
    void onEvent(LoginErrMessage& e) override;
    void onEvent(ChangeHeadRetMessage& e) override;
    void onEvent(LoginProcessMessage& e) override;

private:
	LoginPanel*          _pLoginPanel;

									};

#endif // _MESSAGEMANAGER_H_
