//
// Created by cc on 18-12-3.
//

#include "UserConfig.h"
#include <iostream>
#include "Communication.h"
#include "../Platform/Platform.h"
#include "../Platform/dbPlatForm.h"
#include "../Platform/NavigationManager.h"
#include "../QtUtil/Utils/Log.h"
#include "../QtUtil/nJson/nJson.h"

void updateDbByJson(nJson jsObj, bool sendEvt);

UserConfig::UserConfig(Communication *comm)
        : _pComm(comm) {

}

void UserConfig::getUserConfigFromServer(bool sendEvt) {
    //
    int configVersion = 0;
    LogicManager::instance()->getDatabase()->getConfigVersion(configVersion);
    debug_log("get userconfig version:{0}", configVersion);
    // url
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/configuration/getincreclientconfig.qunar"
        << "?v=" << PLAT.getClientVersion()
        << "&p=" << PLAT.getPlatformStr()
        << "&u=" << PLAT.getSelfUserId()
        << "&k=" << PLAT.getServerAuthKey()
        << "&d=" << PLAT.getSelfDomain();

    std::string strUrl = url.str();

    nJson obj;
    obj["username"] = PLAT.getSelfUserId();
    obj["host"] = PLAT.getSelfDomain();
    obj["version"] = configVersion;
    std::string postData = obj.dump();
    //
    auto callback = [ sendEvt](int code, const std::string &responseData) {

        if (code == 200) {
            nJson jsonObj= Json::parse(responseData);

            if (jsonObj == nullptr) {
                error_log("json paring error"); return;
            }
            updateDbByJson(jsonObj, sendEvt);
        }
    };

    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);
    }
}

//
void UserConfig::updateUserSetting(QUInt8 operatorType, const std::string &key, const std::string &subKey,
                                   const std::string &val) {

    //
    int configVersion = 0;
    LogicManager::instance()->getDatabase()->getConfigVersion(configVersion);
    debug_log("get userconfig version:{0}", configVersion);
    //
    std::ostringstream url;
    url << NavigationManager::instance().getHttpHost()
        << "/configuration/setclientconfig.qunar";
    std::string strUrl = url.str();
    //
    nJson obj;
    obj["username"] = PLAT.getSelfUserId();
    obj["host"] = PLAT.getSelfDomain();
    obj["resource"] = PLAT.getSelfResource();
    obj["version"] = configVersion;
    obj["operate_plat"] = PLAT.getPlatformStr();
    obj["type"] = operatorType;
    obj["key"] = key;
    obj["subkey"] = subKey;
    obj["value"] = val;
    //
    std::string body = obj.dump();
    //
    auto callback = [](int code, const std::string &responseData) {

        if (code == 200) {
            nJson jsonObj= Json::parse(responseData);
            if (jsonObj == nullptr) {
                error_log("json paring error"); return;
            }
            updateDbByJson(jsonObj, true);

        }
    };

    if (_pComm) {
        QTalk::HttpRequest req(strUrl, QTalk::RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = body;

        _pComm->addHttpRequest(req, callback);
    }

}

void updateDbByJson(nJson jsObj, bool sendEvt) {
    std::map<std::string, std::string> deleteData;
    std::vector<QTalk::Entity::ImConfig> arImConfig;

    bool ret = Json::get<bool>(jsObj, "ret");
    bool isMaskName = false;
    if (ret) 
	{
        nJson data = Json::get<nJson >(jsObj, "data");
        int ver = Json::get<int >(data, "version");
        nJson configs= Json::get<nJson >(data, "clientConfigInfos");
        for (auto & conf : configs) {
            std::string key = Json::get<std::string >(conf, "key");

            if("kMarkupNames" == key && !isMaskName)
                isMaskName = true;
            //
            nJson infos= Json::get<nJson >(conf, "infos");
            for (auto & info : infos) {

                std::string subKey = Json::get<std::string >(info, "subkey");
                int isdel = Json::get<int >(info, "isdel");
                if (isdel) {
                    deleteData[subKey] = key;
                } else {
                    std::string configValue = Json::get<std::string >(info, "configinfo");
                    QTalk::Entity::ImConfig imconf;
                    imconf.ConfigKey = key;
                    imconf.ConfigSubKey = subKey;
                    imconf.ConfigValue = configValue;
                    imconf.Version = ver;

                    arImConfig.push_back(imconf);
                }
            }
        }
        if (ret)
		{
            //
			debug_log("update db config-> delete:{0}, insert{1}", deleteData.size(), arImConfig.size());

            if (!deleteData.empty()) {
                LogicManager::instance()->getDatabase()->bulkRemoveConfig(deleteData);
            }
            if (!arImConfig.empty()) {
                LogicManager::instance()->getDatabase()->bulkInsertConfig(arImConfig);
            }
            //
            if(isMaskName)
            {
                std::map<std::string, std::string> mapConf;
                LogicManager::instance()->getDatabase()->getConfig("kMarkupNames", mapConf);
                DB_PLAT.setMaskNames(mapConf);
            }

            //
            if(sendEvt && (!deleteData.empty() || !arImConfig.empty()) )
            {
                CommMsgManager::incrementConfigs(deleteData, arImConfig);
            }
        }

        return;
    } else {
        error_log(Json::get<std::string>(jsObj, "errmsg"));
    }
}
