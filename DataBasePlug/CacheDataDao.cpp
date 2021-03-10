//
// Created by lihaibin on 2019-02-27.
//
#include <iostream>
#include "CacheDataDao.h"
#include "../QtUtil/Utils/Log.h"
#include "../QtUtil/nJson/nJson.h"

#define HOTLINE_KEY "hotline"
#define HOTLINE_TYPE 8

#define USER_ID "USER_ID"
#define USER_ID_TYPE 6

#define GROUP_READMARK "GROUP_READMARK"
#define GROUP_READMARK_TIME 7

#define GROUP_READMARK_LOGIN_BEFORE "GROUP_READMARK_LOGIN_BEFORE"
#define GROUP_READMARK_LOGIN_BEFORE_TIME 12

#define NEW_MESSAGE_FLAG "NEW_MESSAGE_FLAG"
#define NEW_MESSAGE_FLAG_TYPE 1024


CacheDataDao::CacheDataDao(qtalk::sqlite::database *sqlDb)
        : DaoInterface(sqlDb, "IM_Cache_Data") {

}

bool CacheDataDao::creatTable() {
    if (!_pSqlDb) {
        return false;
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS `IM_Cache_Data` ( "
                      "`key`       TEXT,"
                      "`type`    INTEGER,"
                      "`value`     TEXT,"
                      "`valueInt`         INTEGER default 0,"
                      "PRIMARY KEY(`key`,`type`))";

    try {
        qtalk::sqlite::statement query(*_pSqlDb, sql);
        return query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("CREATE TABLE IM_Cache_Data error {0}", e.what());
        return false;
    }
}


bool CacheDataDao::insertUserId(const std::string& value) {
    if (!_pSqlDb) {
        return false;
    }

    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, USER_ID);
        query.bind(2, USER_ID_TYPE);
        query.bind(3, value);

        return query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("insertHotLine exception : {0}", e.what());
        return false;
    }
}

bool CacheDataDao::insertHotLine(std::string value) {
    if (!_pSqlDb) {
        return false;
    }

    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, HOTLINE_KEY);
        query.bind(2, HOTLINE_TYPE);
        query.bind(3, value);

        return query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("insertHotLine exception : {0}", e.what());
        return false;
    }
}

void CacheDataDao::getHotLines(std::string &hotLines) {
    if (!_pSqlDb) {
        return;
    }
    std::string sql = "SELECT `value` FROM IM_Cache_Data WHERE `key` = ? AND `type` = ?;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, HOTLINE_KEY);
        query.bind(2, HOTLINE_TYPE);

        if (query.executeNext()) {
            hotLines = query.getColumn(0).getString();
        }
    }
    catch (const std::exception &e) {
        error_log("exception : {0}", e.what());
    }
}

bool CacheDataDao::isHotlineMerchant(const std::string& xmppid) {
    std::string sql = "SELECT `value` FROM IM_Cache_Data WHERE `key` = ? AND `type` = ?;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, HOTLINE_KEY);
        query.bind(2, HOTLINE_TYPE);

        if (query.executeNext()) {
            std::string value = query.getColumn(0).getString();
            nJson jsonObj= Json::parse(value);
            if (jsonObj == nullptr) {
                error_log("json paring error"); return false;
            }
            bool ret = Json::get<bool >(jsonObj, "ret");
            if(ret){
                nJson data = Json::get<nJson>(jsonObj, "data");
                nJson array= Json::get<nJson>(data,"myhotlines");

                if(array == nullptr){
                    return false;
                }
                if(array.empty()){
                    return false;
                }
                for(auto & item : array){
                    std::string result = Json::convert<std::string>(item);
                    if(xmppid == result){
                        return true;
                    }
                }

                return false;
            }
        }
        return false;
    }
    catch (const std::exception &e) {
        error_log("exception : {0}", e.what());
        return false;
    }
}

std::string CacheDataDao::getGroupReadMarkTime(){
    std::string sql = "SELECT `value` FROM IM_Cache_Data WHERE `key` = ? AND `type` = ?;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, GROUP_READMARK);
        query.bind(2, GROUP_READMARK_TIME);

        if (query.executeNext()) {
            std::string value = query.getColumn(0).getString();
            return value;
        }
        return "0";
    }
    catch (const std::exception &e) {
        error_log("getGroupReadMarkTime exception : {0}", e.what());
        return "0";
    }
}
bool CacheDataDao::updateGroupReadMarkTime(const std::string& time){
    if (!_pSqlDb) {
        return false;
    }

    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, GROUP_READMARK);
        query.bind(2, GROUP_READMARK_TIME);
        query.bind(3, time);

        return query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("updateGroupReadMarkTime exception : {0}", e.what());
        return false;
    }
}

std::string CacheDataDao::getLoginBeforeGroupReadMarkTime(){
    std::string sql = "SELECT `value` FROM IM_Cache_Data WHERE `key` = ? AND `type` = ?;";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, GROUP_READMARK_LOGIN_BEFORE);
        query.bind(2, GROUP_READMARK_LOGIN_BEFORE_TIME);

        if (query.executeNext()) {
            std::string value = query.getColumn(0).getString();
            return value;
        }
        return "0";
    }
    catch (const std::exception &e) {
        error_log("getGroupReadMarkFailTime exception : {0}", e.what());
        return "0";
    }
}
bool CacheDataDao::saveLoginBeforeGroupReadMarkTime(const std::string& time){
    if (!_pSqlDb) {
        return false;
    }

    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, GROUP_READMARK_LOGIN_BEFORE);
        query.bind(2, GROUP_READMARK_LOGIN_BEFORE_TIME);
        query.bind(3, time);

        return query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("saveGroupReadMarkFailTime exception : {0}", e.what());
        return false;
    }
}

void CacheDataDao::clear_data_01() {
    if (!_pSqlDb) {
        return;
    }
//    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    std::string sql = "Delete from IM_Cache_Data where `key` = ? and `type` = ?";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, HOTLINE_KEY);
        query.bind(2, HOTLINE_TYPE);
        query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("clear_data_01 exception : {0}", e.what());
    }
}

//
void CacheDataDao::insertNewMessageTimestamp(QInt64 time) {
    if (!_pSqlDb) {
        return;
    }
    std::string sql = "INSERT OR REPLACE INTO IM_Cache_Data (`key`, `type`, `value`) values (?, ?, ?);";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, NEW_MESSAGE_FLAG);
        query.bind(2, NEW_MESSAGE_FLAG_TYPE);
        query.bind(3, std::to_string(time));

        query.executeStep();
    }
    catch (const std::exception &e) {
        error_log("clear_data_01 exception : {0}", e.what());
    }
}

QInt64 CacheDataDao::getNewMessageTimestamp() {
    QInt64 time = 0 ;
    if (!_pSqlDb) {
        return time;
    }
    std::string sql = "SELECT `value` From IM_Cache_Data Where `key` = ? And `type` = ?";
    qtalk::sqlite::statement query(*_pSqlDb, sql);
    try {
        query.bind(1, NEW_MESSAGE_FLAG);
        query.bind(2, NEW_MESSAGE_FLAG_TYPE);
        if(query.executeNext())
        {
            time = std::strtoll(query.getColumn(0).getString().data(), nullptr, 0);
        }
        return time;
    }
    catch (const std::exception &e) {
        error_log("clear_data_01 exception : {0}", e.what());
        return time;
    }
}