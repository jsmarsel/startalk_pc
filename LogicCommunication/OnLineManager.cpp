#include "OnLineManager.h"
#include <iostream>
#include <sstream>
#include "../Platform/NavigationManager.h"
#include "../Platform/Platform.h"
#include "../QtUtil/nJson/nJson.h"
#include "Communication.h"
#include "../QtUtil/Utils/Log.h"
//#include "../QtUtil/Enum/im_enum.h"

using namespace QTalk;

OnLineManager::OnLineManager(Communication *pComm) : _pComm(pComm)
{
}

/**
  * @函数名
  * @功能描述 根据参数中用户是否在线
  * @参数
  * @date 2018.10.12
  */
bool OnLineManager::getOnLineUser(const std::set<std::string> &users, bool sendRet)
{

    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/domain/get_user_status.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();

    std::string postData;
    {
        nJson gObj;
        nJson userAry;
        for (const std::string &user : users)
        {
            userAry.push_back(user);
        }
        gObj["users"] = userAry;
        postData = gObj.dump();
    }

    std::map<std::string, std::string> userStatus;

    bool retSts = false;
    auto callback = [users, &retSts, &userStatus](int code, const std::string &responseData) {
        if (code == 200)
        {
            nJson data = Json::parse(responseData);

            if (data == nullptr)
            {
                error_log("json paring error");
                return;
            }

            bool ret = Json::get<bool>(data, "ret");
            if (ret)
            {
                nJson dataObj = Json::get<nJson>(data, "data");
                if (nullptr != dataObj)
                {
                    nJson userStatusAry = Json::get<nJson>(dataObj, "ul");

                    for (auto &item : userStatusAry)
                    {
                        std::string struser = Json::get<std::string>(item, "u");
                        std::string strstatus = Json::get<std::string>(item, "o");
                        userStatus[struser] = strstatus;
                    }
                }
                retSts = true;
                return;
            }
        }
        else
        {
        }
    };

    if (_pComm)
    {
        QTalk::HttpRequest req(url.str(), RequestMethod::POST);
        req.body = postData;
        req.header["Content-Type"] = "application/json;";
        req.timeout = 10L;
        _pComm->addHttpRequest(req, callback, false);
        if (retSts)
        {
            // sigle search
            if (sendRet && userStatus.size() == 1)
            {
                CommMsgManager::sendGotUsersStatus(userStatus.cbegin()->first,
                                                   userStatus.cbegin()->second);
            }

            PLAT.loadOnlineData(userStatus);
        }
    }

    return false;
}
