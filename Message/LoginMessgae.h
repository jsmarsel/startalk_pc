﻿#ifndef _LOGINMESSGAE_H_
#define _LOGINMESSGAE_H_

#include <string>
#include <map>
#include <utility>
#include <vector>
#include <memory>
#include "../EventBus/Event.hpp"
#include "../entity/IM_Session.h"

typedef std::shared_ptr<std::vector<std::shared_ptr<QTalk::Entity::ImSessionInfo>>> ImSessions;

class SaveQchatQVTToDB : public Event
{
public:
    SaveQchatQVTToDB(std::string qvt) : strQVT(std::move(qvt)){};

public:
    std::string strQVT;
};

class GetQchatQVTFromDB : public Event
{
public:
    std::string strQVT;
};

class GetQchatToken : public Event
{
public:
    GetQchatToken(const std::string &qvt) : strQVT(qvt){};

public:
    std::string strQVT;
    std::map<std::string, std::string> userMap;
};

//
class LoginMessage : public Event
{
public:
    LoginMessage(std::string userName, std::string password)
        : ret(false), strUserName(std::move(userName)), strPassword(std::move(password))
    {
    }

public:
    bool ret;
    std::string strUserName;
    std::string strPassword;
};

// 登陆成功消息
class LoginSuccessMessage : public Event
{
public:
    void setSessionId(const std::string &sessionId) { strSessionId = sessionId; }
    std::string getSessionId() { return strSessionId; }

private:
    std::string strSessionId;
};

class SynSeverDataEvt : public Event
{
};

//
class GotStructureMessage : public Event
{
public:
    bool _bRet; // 结果
};

//
class SynOfflineSuccees : public Event
{
};

class RemoveSessionData : public Event
{
public:
    explicit RemoveSessionData(const std::string &peerId) : _peerId(peerId) {}

public:
    std::string _peerId;
};

class GetSessionData : public Event
{
};

class GetNavAddrInfo : public Event
{
public:
    explicit GetNavAddrInfo(const std::string &nav)
        : ret(false), navAddr(nav) {}

public:
    bool ret;
    std::string navAddr;
};

class GetNavDomain : public Event
{
public:
    explicit GetNavDomain(const std::string &nav)
        : navAddr(nav) {}

public:
    const std::string &navAddr;
    std::string doamin;
};

class HeartBeat : public Event
{
};

class StartUpdaterEvt : public Event
{
public:
    explicit StartUpdaterEvt(std::string users) : users(std::move(users)) {}

public:
    std::string users;
};

class LoginProcessMessage : public Event
{
public:
    explicit LoginProcessMessage(const std::string &statusMessage, bool &status)
        : status(status), message(statusMessage) {}

public:
    bool &status;
    std::string message;
};

#endif //_LOGINMESSGAE_H_
