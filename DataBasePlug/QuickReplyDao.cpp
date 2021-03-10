//
// Created by lihaibin on 2019-06-26.
//
#include "QuickReplyDao.h"
#include "../QtUtil/Utils/utils.h"
#include "../QtUtil/Utils/Log.h"
#include "../QtUtil/nJson/nJson.h"

QuickReplyDao::QuickReplyDao(qtalk::sqlite::database *sqlDb) :
        DaoInterface(sqlDb, "IM_QUICK_REPLY_GROUP"){

}

bool QuickReplyDao::creatTable() {
    if (!_pSqlDb) {
        return false;
    }
    std::string sql = "CREATE TABLE IF NOT EXISTS `IM_QUICK_REPLY_GROUP` ( "
                      "`sid`	        INTEGER, "
                      "`groupname`	        TEXT, "
                      "`groupseq`	            INTEGER, "
                      "`version`	        INTEGER default 1, "
                      "PRIMARY KEY(`sid`) ) ";
    qtalk::sqlite::statement query(*_pSqlDb, sql);

    std::string sql1 = "CREATE TABLE IF NOT EXISTS `IM_QUICK_REPLY_CONTENT` ( "
                       "`sid`	        Long, "
                       "`gid`	        Long, "
                       "`content`	        TEXT, "
                       "`contentseq`	            INTEGER, "
                       "`version`	        INTEGER default 1, "
                       "PRIMARY KEY(`sid`,'gid') ) ";
    qtalk::sqlite::statement query1(*_pSqlDb, sql1);

    bool result = query.executeStep() && query1.executeStep();
    if(result){
        std::string sql2 = "CREATE INDEX IF NOT EXISTS IX_IM_QUICK_REPLY_CONTENT_GID ON IM_QUICK_REPLY_CONTENT(gid);";
        qtalk::sqlite::statement query2(*_pSqlDb, sql2);
        return result && query2.executeStep();
    } else{
        return result;
    }
}

void QuickReplyDao::batchInsertQuickReply(const std::string & pData) {

    nJson jsonObj = Json::parse(pData);

    if (jsonObj == nullptr) {
        error_log("json paring error"); return;
    }
    bool ret = Json::get<bool>(jsonObj, "ret");
    if(ret){
        std::string sql = "insert or REPLACE into IM_QUICK_REPLY_GROUP(sid, groupname, groupseq, version) values(?, ?, ?, ?);";
        std::string deleteSql = "delete from IM_QUICK_REPLY_GROUP where sid = ?";

        qtalk::sqlite::statement insertStatement(*_pSqlDb, sql);

        qtalk::sqlite::statement deleteStatement(*_pSqlDb, deleteSql);

        std::string sql1 = "insert or REPLACE into IM_QUICK_REPLY_CONTENT(sid, gid, content, contentseq,version) values(?, ?, ?, ?,?);";
        std::string deleteSql1 = "delete from IM_QUICK_REPLY_CONTENT where sid = ?";

        qtalk::sqlite::statement insertStatement1(*_pSqlDb, sql1);

        qtalk::sqlite::statement deleteStatement1(*_pSqlDb, deleteSql1);

        nJson data = Json::get<nJson >(jsonObj, "data");
        nJson groupInfo = Json::get<nJson >(data, "groupInfo");
        nJson groups = Json::get<nJson >(groupInfo, "groups");
        nJson contentInfo = Json::get<nJson >(data, "contentInfo");
        nJson contents = Json::get<nJson >(contentInfo, "contents");

        try {
            _pSqlDb->exec("begin immediate;");
            if(groups.is_array()) {
                for(auto& group : groups){

                    int isDel = Json::get<bool >(group,"isdel");
                    if(isDel){
                        deleteStatement.bind(1, group["id"].get<std::string>());
                        deleteStatement.executeStep();
                        deleteStatement.resetBindings();
                    } else{
                        insertStatement.bind(1, Json::get<long long >(group,"id"));
                        insertStatement.bind(2, Json::get<std::string >(group,"groupname"));
                        insertStatement.bind(3, Json::get<long long >(group,"groupseq"));
                        insertStatement.bind(4, Json::get<long long >(group,"version"));

                        insertStatement.executeStep();
                        insertStatement.resetBindings();
                    }
                }
                insertStatement.clearBindings();
                deleteStatement.clearBindings();
            }


            if(contents.is_array()) {
                for(const auto& content : contents){
                    bool isDel = Json::get<bool >(content,"isdel");
                    if(isDel){
                        deleteStatement1.bind(1,Json::get<long long >(content,"id"));
                        deleteStatement1.executeStep();
                        deleteStatement1.resetBindings();
                    } else{
                        insertStatement1.bind(1,Json::get<long long >(content,"id"));
                        insertStatement1.bind(2,Json::get<long long >(content,"groupid"));
                        insertStatement1.bind(3,Json::get<std::string >(content,"content"));
                        insertStatement1.bind(4,Json::get<long long >(content,"contentseq"));
                        insertStatement1.bind(5,Json::get<long long >(content,"version"));
                        insertStatement1.executeStep();
                        insertStatement1.resetBindings();
                    }
                }
                insertStatement1.clearBindings();
                deleteStatement1.clearBindings();
            }

            _pSqlDb->exec("commit transaction;");

        } catch(...) {
            insertStatement.clearBindings();
            deleteStatement.clearBindings();
            insertStatement1.clearBindings();
            deleteStatement1.clearBindings();
            _pSqlDb->exec("rollback transaction;");
        }
    }
}
void QuickReplyDao::getQuickReplyVersion(QInt64 version[]) {
    QInt64 groupVersion = 0;
    QInt64 contentVersion = 0;
    std::string sql = "select max(version) from IM_QUICK_REPLY_GROUP;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);

    std::string sql1 = "select max(version) from IM_QUICK_REPLY_CONTENT;";
    qtalk::sqlite::statement query1(*_pSqlDb, sql1);
    try {
        if (query.executeNext()) {
            groupVersion = query.getColumn(0).getInt64();
        }

        if(query1.executeNext()){
            contentVersion = query1.getColumn(0).getInt64();
        }

    }
    catch (const std::exception &e) {
        error_log("exception : {0}", e.what());
    }
    version[0] = groupVersion;
    version[1] = contentVersion;
}

void QuickReplyDao::getQuickGroups(std::vector<QTalk::Entity::ImQRgroup> &groups) {
    if (!_pSqlDb) {
        return;
    }

    // 获取所有用户
    std::string sql = "SELECT `sid`,`groupname`,`groupseq`,`version` FROM IM_QUICK_REPLY_GROUP order by groupseq asc;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        while (query.executeNext()) {
            QTalk::Entity::ImQRgroup group;
            group.sid = query.getColumn(0).getInt();
            group.groupname = query.getColumn(1).getString();
            group.groupseq = query.getColumn(2).getInt();
            group.version = query.getColumn(3).getInt();
            groups.push_back(group);
        }
    }
    catch (const std::exception &e) {
        error_log("exception : {0}", e.what());
    }
}

void QuickReplyDao::getQuickContentByGroup(std::vector<QTalk::Entity::IMQRContent> &contents, int id) {
    if (!_pSqlDb) {
        return;
    }

    // 获取所有用户
    std::string sql = "SELECT `sid`,`gid`,`content`,`contentseq`,`version` FROM IM_QUICK_REPLY_CONTENT where gid = ? order by contentseq asc;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    query.bind(1,id);
    try {
        while (query.executeNext()) {
            QTalk::Entity::IMQRContent content;
            content.sid = query.getColumn(0).getInt();
            content.gid = query.getColumn(1).getInt();
            content.content = query.getColumn(2).getString();
            content.contentseq = query.getColumn(3).getInt();
            content.version = query.getColumn(4).getInt();
            contents.push_back(content);
        }
    }
    catch (const std::exception &e) {
        error_log("exception : {0}", e.what());
    }
}