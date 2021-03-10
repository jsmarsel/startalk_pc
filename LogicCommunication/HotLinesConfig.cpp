//
// Created by lihaibin on 2019-02-26.
//

#include "HotLinesConfig.h"
#include "Communication.h"
#include "../Platform/NavigationManager.h"
#include "../Platform/Platform.h"
#include "../Platform/dbPlatForm.h"
#include "../QtUtil/nJson/nJson.h"
#include <iostream>

HotLinesConfig::HotLinesConfig(Communication *comm)
        : _pComm(comm) {

}

HotLinesConfig::~HotLinesConfig()
= default;

void HotLinesConfig::getVirtualUserRole(){
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/admin/outer/qtalk/getHotlineList";
    std::string strUrl = url.str();

    nJson obj;
    obj["username"] = PLAT.getSelfName().data();
    obj["host"] = PLAT.getSelfDomain().data();
    std::string postDta = obj.dump();
    //
    std::set<std::string> hotlines;
    auto callback = [ &hotlines](int code, const std::string &responseData) {
        if (code == 200) {
            nJson json= Json::parse(responseData);
            if(nullptr == json)
            {
                error_log("error json {0}", responseData);
                return;
            }

            bool ret = Json::get<bool>(json, "ret");
            if(ret)
            {
                nJson data= Json::get<nJson >(json, "data");
                nJson allhotlines= Json::get<nJson >(data, "allhotlines");

                for(auto &item : allhotlines) {
                    if(item && item.is_string())
                        hotlines.insert(item.get<std::string>());
                }
            }
            }
    };
    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postDta;
        _pComm->addHttpRequest(req, callback);

        if(!hotlines.empty())
            DB_PLAT.setHotLines(hotlines);
    }
}

void HotLinesConfig::getServiceSeat() {
    std::ostringstream url;
    url << NavigationManager::instance().getQcadminHost()
        << "/api/seat/getSeatSeStatusWithSid.qunar?qName="
        << PLAT.getSelfUserId();
    std::string strUrl = url.str();

    //
    auto callback = [](int code, const std::string &responseData) {

        if (code == 200) {
            PLAT.setSeats(responseData);
        }
    };
    std::string qvt = PLAT.getQvt();
    if(qvt.empty()){
        return;
    }
    nJson qvtJson= Json::get<nJson >(Json::parse(qvt),"data");

    auto qcookie = Json::get<std::string>(qvtJson,"qcookie");
    auto vcookie = Json::get<std::string>(qvtJson,"vcookie");
    auto tcookie = Json::get<std::string>(qvtJson,"tcookie");
    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::GET);
        std::string requestHeaders = std::string("_q=") + qcookie + ";_v=" + vcookie + ";_t=" + tcookie;
        req.header["Content-Type"] = "application/json;";
        req.header["Cookie"] = requestHeaders;
        _pComm->addHttpRequest(req, callback);
    }
}

void HotLinesConfig::setServiceSeat(int sid, int state) {
    std::ostringstream url;
    url << NavigationManager::instance().getQcadminHost()
        << "/api/seat/upSeatSeStatusWithSid.qunar?qName="
        << PLAT.getSelfUserId()
        << "&st="
        << state
        << "&sid="
        << sid;
    std::string strUrl = url.str();

    //
    auto callback = [](int code, const std::string &responseData) {

        if (code == 200) {

        }
    };
    std::string qvt = PLAT.getQvt();
    if(qvt.empty()){
        return;
    }
    nJson qvtJson= Json::get<nJson >(Json::parse(qvt),"data");
    auto qcookie = Json::get<std::string>(qvtJson,"qcookie");
    auto vcookie = Json::get<std::string>(qvtJson,"vcookie");
    auto tcookie = Json::get<std::string>(qvtJson,"tcookie");
    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::GET);
        std::string requestHeaders = std::string("_q=") + qcookie + ";_v=" + vcookie + ";_t=" + tcookie;
        req.header["Content-Type"] = "application/json;";
        req.header["Cookie"] = requestHeaders;
        _pComm->addHttpRequest(req, callback);
    }
}

//
void HotLinesConfig::serverCloseSession(const std::string& username, const std::string& virtualname) {

    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/admin/outer/qtalk/closeSession";
    std::string strUrl = url.str();

    nJson obj;
    obj["customerName"] = username.data();
    obj["hotlineName"] = virtualname.data();
    std::string postDta = obj.dump();
    //
    auto callback = [virtualname, username](int code, const std::string &responseData) {
        if(code == 200)
            info_log("close session {0} -> {1}", virtualname, username);
        else
            error_log("close session failed {0} -> {1}", virtualname, username);
    };
    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postDta;
        _pComm->addHttpRequest(req, callback);
    }
}

//void HotLinesConfig::getTransferSeatsList(const QTalk::Entity::UID& uid) {
//    QTalk::Entity::JID jid(uid.usrId().c_str());
//    std::string shopJId = jid.username();
//    std::ostringstream url;
//    url << NavigationManager::instance().getHttpHost()
//        << "/admin/outer/qtalk/hotlineSeatList.json";
//    std::string strUrl = url.str();
//    //
//    nJson obj;
//    obj[ "customerName"] = QTalk::Entity::JID(uid.realId()).username();
//    obj["hotlineName"] = shopJId.data();
//    std::string postDta = obj.dump();
//    //
//    auto callback = [ uid](int code, const std::string &responseData) {
//
//        if (code == 200) {
//            nJson json= Json::parse(responseData);
//            bool ret = Json::get<bool>(json,"ret");
//            if(ret){
//                std::vector<QTalk::Entity::ImTransfer> transfers;
//                nJson data= Json::get<nJson >(json,"data");
//                for(auto &item : data) {
//                    QTalk::Entity::ImTransfer transfer;
//                    transfer.userId = Json::get<std::string >(item,"userId");
//                    transfer.userName = Json::get<std::string >(item,"userName");
//                    transfers.push_back(transfer);
//                }
//
//                CommMsgManager::setSeatList(uid,transfers);
//            }
//        }
//    };
//    if (_pComm) {
//        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
//        req.header["Content-Type"] = "application/json;";
//        req.body = postDta;
//        _pComm->addHttpRequest(req, callback);
//    }
//}

void HotLinesConfig::transferCsr(const QTalk::Entity::UID& uid,
                                 const std::string& newCsrName, const std::string &reason) {
    std::string shopJId = uid.usrId();
    std::string customerName = uid.realId();
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/admin/outer/qtalk/transformSeat.json";

    std::string strUrl = url.str();
    nJson obj;
    obj["customerName"] = customerName.data();
    obj["hotlineName"] = shopJId.data();
    obj["currentCsrName"] = PLAT.getSelfUserId().data();
    obj["newCsrName"] = newCsrName.data();
    obj["reason"] = reason.data();
    std::string postDta = obj.dump();
    //
    auto callback = [](int code, const std::string &responseData) {

        if (code == 200) {

        } else {
            error_log("transformSeat {0}", responseData);
        }
    };
    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postDta;
        _pComm->addHttpRequest(req, callback);
    }
}

void HotLinesConfig::updateQuickReply() {
    QInt64 version[2];
    LogicManager::instance()->getDatabase()->getQuickReplyVersion(version);
    QInt64 gversion = version[0];
    QInt64 cversion = version[1];
    // url
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/quickreply/quickreplylist.qunar";

    std::string strUrl = url.str();

    nJson jsonObject;
    jsonObject["username"] = PLAT.getSelfUserId();
    jsonObject["host"] = PLAT.getSelfDomain();
    jsonObject["groupver"] = gversion;
    jsonObject["contentver"] = cversion;

    std::string postData = jsonObject.dump();
    //
    auto callback = [](int code, const std::string &responseData) {
        if (code == 200) {
            LogicManager::instance()->getDatabase()->batchInsertQuickReply(responseData);
        }
    };

    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);
    }
}

void HotLinesConfig::sendProduct(const std::string& username, const std::string& virtualname,
                                 const std::string& product,const std::string& type) {
    // url
    std::ostringstream url;
    url << NavigationManager::instance().getQcadminHost()
        << "/api/pdt/sendNoteByJson.qunar";

    std::string strUrl = url.str();

//    nJson prod= Json::parse(product.c_str());

    nJson jsonObject;
    nJson args;

    args["type"] = type;
    args["seatQName"] = PLAT.getSelfUserId();
    args["userHost"] = PLAT.getSelfDomain();
    args["userQName"] = username;
    args["virtualId"] = virtualname;
    args["seatHost"] = PLAT.getSelfDomain();
    jsonObject["noteArgs"] = args;
    jsonObject["productVO"] = product;

    std::string postData = jsonObject.dump();

    auto callback = [](int code, const std::string &responseData) {
        if (code == 200) {

        }
    };

    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);
    }
}

void HotLinesConfig::sendWechat(const QTalk::Entity::UID &uid) {
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/qchat/sendWe.qunar";

    std::string strUrl = url.str();

    nJson jsonObject;
    jsonObject["from"] = PLAT.getSelfXmppId();
    jsonObject["to"] = uid.usrId();
    jsonObject["realJid"] = uid.realId();
    jsonObject["chatType"] = "5";
    std::string postData = jsonObject.dump();
    auto callback = [](int code, const std::string &responseData) {
        if (code == 200) {

        }
    };

    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);
    }
}

void HotLinesConfig::getHotLineMessageList(const std::string &xmppId) {

    QTalk::Entity::JID jid(xmppId);

    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/robot/qtalk_robot/sendtips"
        << "?rexian_id=" << jid.username()
        << "&m_from=" << jid.basename()
        << "&m_to=" << PLAT.getSelfXmppId();

    auto callback = [](int code, const std::string &responseData) {
        if (code == 200) {

        }
    };

    std::string strUrl = url.str();
    if (_pComm) {
        QTalk::HttpRequest req(strUrl);
        _pComm->addHttpRequest(req, callback);
    }
}
