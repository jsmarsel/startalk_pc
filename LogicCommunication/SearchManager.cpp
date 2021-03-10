#include "SearchManager.h"
#include <iostream>
#include <sstream>
#include "../Platform/NavigationManager.h"
#include "../Platform/Platform.h"
#include "../QtUtil/nJson/nJson.h"
#include "Communication.h"
#include "../QtUtil/Utils/Log.h"

using namespace QTalk;

SearchManager::SearchManager(Communication *pComm) :
        _pComm(pComm) {

}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.08
  */
void SearchManager::GetSearchResult(SearchInfoEvent &e) {

    std::string searchUrl = NavigationManager::instance().getSearchUrl();
    std::string userId = PLAT.getSelfXmppId();

    nJson obj;
    obj["start"] = e.start;
    obj["length"] = e.length;
    obj["key"] = e.key.c_str();
    obj["qtalkId"] = userId.c_str();
    obj["action"] = e.action;
    if (!e.to_user.empty())
        obj["to_user"] = e.to_user.data();
    else if(!e.to_muc.empty())
        obj["to_muc"] = e.to_muc.data();
    std::string postData = obj.dump();

    auto callback = [&e](int code, const std::string &responseData) {

        nJson response= Json::parse(responseData);
        if(nullptr == response)
        {
            error_log("search error retData:{0}", responseData);
            return;
        }

        if (code == 200) {

            bool ret = Json::get<bool>(response, "ret");
            if(ret)
            {
                nJson data= Json::get<nJson>(response, "data");
                //
                for(auto &item : data) {

                    nJson info= Json::get<nJson >(item, "info");
                    if(nullptr == info) continue;

                    Search::StSearchResult tmpRet = Search::StSearchResult();
                    tmpRet.resultType = Json::get<int >(item, "resultType");
//                    tmpRet.default_portrait = Json::get<std::string >(item, "defaultportrait");
//                    tmpRet.groupId = Json::get<std::string >(item, "groupId");
                    tmpRet.groupLabel = Json::get<std::string >(item, "groupLabel");
                    tmpRet.hasMore = Json::get<bool>(item, "hasMore");

                    if(tmpRet.resultType & Search::EM_ACTION_USER)
                    {
                        for (auto &iitem : info) {
                            Search::StUserItem tmpItem;
                            tmpItem.xmppId = Json::get<std::string >(iitem, "uri");
                            tmpItem.name = Json::get<std::string >(iitem, "name");
                            tmpItem.tips = Json::get<std::string >(iitem, "label");
                            tmpItem.icon = Json::get<std::string >(iitem, "icon");
                            tmpItem.structure = Json::get<std::string >(iitem, "content");

                            tmpRet._users.push_back(tmpItem);
                        }
                    }
                    else if((tmpRet.resultType & Search::EM_ACTION_MUC)
                        || (tmpRet.resultType & Search::EM_ACTION_COMMON_MUC))
                    {
                        for (auto &iitem : info) {
                            Search::StGroupItem tmpItem;

                            nJson hits= Json::get<nJson >(iitem, "hit");
                            tmpItem.type = Json::get<int >(iitem, "todoType");
                            tmpItem.xmppId = Json::get<std::string >(iitem, "uri");
                            tmpItem.name = Json::get<std::string >(iitem, "label");
                            tmpItem.icon = Json::get<std::string >(iitem, "icon");

                            if(nullptr != hits)
                            {
                                for (auto &iiitem : hits) {
                                    tmpItem._hits.push_back(Json::convert<std::string>(iiitem));
                                }
                            }

                            tmpRet._groups.push_back(tmpItem);
                        }
                    }
                    else if((tmpRet.resultType & Search::EM_ACTION_HS_SINGLE) ||
                            (tmpRet.resultType & Search::EM_ACTION_HS_MUC))
                    {
                        for(auto &iitem : info) {
                            Search::StHistory tmpItem;

                            tmpItem.key = e.key;
                            tmpItem.type = Json::get<int >(iitem, "todoType");
                            tmpItem.name = Json::get<std::string >(iitem, "label");
                            tmpItem.icon = Json::get<std::string >(iitem, "icon");
                            if (iitem.contains( "count"))
                                tmpItem.count = Json::get<int >(iitem, "count");
                            tmpItem.body = Json::get<std::string >(iitem, "body");
                            tmpItem.time = atoll(Json::get<std::string >(iitem, "time").data());
                            tmpItem.from = Json::get<std::string >(iitem, "from");
                            tmpItem.to = Json::get<std::string >(iitem, "to");
                            tmpItem.msg_type = atoi(Json::get<std::string >(iitem, "mtype").data());
                            tmpItem.msg_id = Json::get<std::string >(iitem, "msgid");
                            tmpItem.extend_info = Json::get<std::string >(iitem, "extendinfo");
//                            tmpItem.real_from = Json::get<std::string >(iitem, "realfrom");
//                            tmpItem.real_to = Json::get<std::string >(iitem, "realto");

                            tmpRet._history.push_back(tmpItem);
                        }
                    }
                    else if((tmpRet.resultType & Search::EM_ACTION_HS_FILE)) {
                        for (auto &iitem : info) {
                            Search::StHistoryFile tmpItem;

                            tmpItem.key = e.key;
                            tmpItem.source = Json::get<std::string >(iitem, "source");
                            tmpItem.msg_id = Json::get<std::string >(iitem, "msgid");
//                            tmpItem.icon = Json::get<std::string >(iitem, "icon");
                            tmpItem.body = Json::get<std::string >(iitem, "body");
                            tmpItem.extend_info = Json::get<std::string >(iitem, "extendinfo");
                            tmpItem.time = atoll(Json::get<std::string >(iitem, "time").data());
                            tmpItem.from = Json::get<std::string >(iitem, "from");
                            tmpItem.to = Json::get<std::string >(iitem, "to");

                            nJson file_item= Json::get<nJson >(iitem, "fileinfo");

                            tmpItem.file_md5 = Json::get<std::string >(file_item, "FILEMD5");
                            tmpItem.file_name = Json::get<std::string >(file_item, "FileName");
                            tmpItem.file_size = Json::get<std::string >(file_item, "FileSize");
                            tmpItem.file_url = Json::get<std::string >(file_item, "HttpUrl");

                            tmpRet._files.push_back(tmpItem);
                        }
                    }
                    e.searchRet[tmpRet.resultType] = tmpRet;
                }
            }
        } else {
            std::string errmsg = Json::get<std::string >(response, "errmsg");
            error_log("search error errmsg:{0}", errmsg);
        }

    };

    if (_pComm) {
        QTalk::HttpRequest req(searchUrl, RequestMethod::POST);
        req.header["Content-Type"] = "application/json;";
        req.body = postData;
        _pComm->addHttpRequest(req, callback);
    }
}
