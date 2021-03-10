﻿#include "Platform.h"

#include <time.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include "../QtUtil/lib/Md5/Md5.h"
#include "../QtUtil/lib/Base64/base64.h"
#include "dbPlatForm.h"

//#include <cctype>
#include "../QtUtil/Entity/JID.h"
#include <utility>
#include "../QtUtil/Utils/utils.h"

#ifdef _LINUX
    #include <algorithm>
#endif

Platform::Platform()
    : _mapUserStatus(5000)
{
}

Platform &Platform::instance()
{
    static Platform platform;
    return platform;
}

void Platform::setExecutePath(const std::string &exePath)
{
    executePath = exePath;
}

std::string Platform::getExecutePath()
{
    return executePath;
}

void Platform::setExecuteName(const std::string &name)
{
    executeName = name;
}

std::string Platform::getExecuteName()
{
    return executeName;
}

/**
  * @函数名
  * @功能描述 获取程序appdata Roaming 下路径
  * @参数
  * @date 2018.9.17
  */
std::string Platform::getAppdataRoamingPath() const
{
    return AppSetting::instance().getUserDirectory();
}

/**
  * @函数名
  * @功能描述 设置程序appdata Roaming 下路径
  * @参数
  * @date 2018.9.17
  */
void Platform::setAppdataRoamingPath(const std::string &path)
{
    AppSetting::instance().setUserDirectory(path);
}

std::string Platform::getAppdataRoamingUserPath() const
{
    std::ostringstream userPath;
    userPath << AppSetting::instance().getUserDirectory()
             << "/"
             << _strUserId
             << "@"
             << _strDomain
             << "_"
             << _navName;
    return userPath.str();
}

std::string Platform::getLocalEmoticonPath(const std::string &packegId) const
{
    return AppSetting::instance().getUserDirectory() + "/emoticon/" + packegId;
}

std::string Platform::getLocalEmoticonPacketPath(const std::string &packegId) const
{
    return AppSetting::instance().getUserDirectory() + "/emoticon/packet/" + packegId;
}

std::string Platform::getEmoticonIconPath() const
{
    return AppSetting::instance().getUserDirectory() + "/emoticon/icon";
}

std::string Platform::getTempEmoticonPath(const std::string &packegId)
{
    return AppSetting::instance().getUserDirectory() + "/emoticon/temp/" + packegId;
}

/**
  * @函数名
  * @功能描述 默认临时下载文件目录
  * @参数
  * @date 2018.10.22
  */
std::string Platform::getTempFilePath() const
{
    return getAppdataRoamingUserPath() + "/temp";
}

/**
  * @函数名   getSelfUserId
  * @功能描述 获取自身id
  * @参数
  * @author   cc
  * @date     2018/09/19
  */
std::string Platform::getSelfUserId() const
{
    return _strUserId;
}

std::string Platform::getSelfXmppId() const
{
    if(_strUserId.empty() || _strDomain.empty())
        return std::string();

    std::ostringstream xmppId;
    xmppId << _strUserId
           << "@"
           << _strDomain;
    return xmppId.str();
}

/**
  * @函数名   setSelfUserId
  * @功能描述 设置自身id
  * @参数
  * @author   cc
  * @date     2018/09/19
  */
void Platform::setSelfUserId(const std::string &selfUserId)
{
    _strUserId = selfUserId;
}

std::string Platform::getSelfDomain() const
{
    return this->_strDomain;
}

void Platform::setSelfDomain(const std::string &selfDomain)
{
    this->_strDomain = selfDomain;
}

/**
 *
 * @return 自己名称
 */
std::string Platform::getSelfName()
{
    if(_strSelfName.empty())
    {
        std::shared_ptr<QTalk::Entity::ImUserInfo> info = DB_PLAT.getUserInfo(_strUserId + "@" + _strDomain);

        if (nullptr != info)
            _strSelfName = QTalk::getUserNameNoMask(info);
    }

    return _strSelfName;
}

long long Platform::getServerDiffTime()
{
    return this->_serverTimeDiff;
}

void Platform::setServerDiffTime(long long serverDiffTime)
{
    this->_serverTimeDiff = serverDiffTime;
}

std::string Platform::getServerAuthKey()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(cKeySm);
    return this->_serverAuthKey;
}

void Platform::setServerAuthKey(const std::string &authKey)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(cKeySm);
    this->_serverAuthKey = authKey;
}

std::string Platform::getClientAuthKey()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(cKeySm);
    time_t now = time(nullptr);
    long long time = now - this->_serverTimeDiff;
    std::string key = MD5(this->_serverAuthKey + std::to_string(time)).toString();

    if (key.empty())
        throw std::runtime_error("getClientAuthKey failed: key is empty");

    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    std::stringstream ss;
    ss << "u=" << _strUserId
       << "&k=" << key
       << "&t=" << time
       << "&d=" << _strDomain
       << "&r=" << _strResource;
    std::string content = ss.str();
    std::string result = base64_encode((unsigned char *) content.c_str(), (unsigned int) content.length());
    return result;
}

void Platform::setClientAuthKey(const std::string &clientAuthKey)
{
    this->_q_ckey = clientAuthKey;
}

std::string Platform::getPlatformStr() const
{
#ifdef _WINDOWS
#ifdef _WIN64
    return "PC64";
#else
    return "PC32";
#endif // _WIN32
#else
#ifdef _LINUX
    return "LINUX";
#else
    return "Mac";
#endif
#endif
}

std::string Platform::getClientVersion() const
{
    return std::to_string(GLOBAL_INTERNAL_VERSION);
}

long long Platform::getClientNumVerison() const
{
    return GLOBAL_INTERNAL_VERSION;
}

std::string Platform::get_build_date_time() const
{
    std::ostringstream src;
    src << __TIME__ << " " << __DATE__;
    return src.str();
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.12
  */
bool Platform::isOnline(const std::string &xmppId)
{
    bool isOnline = false;
    {
        std::lock_guard<QTalk::util::spin_mutex> lock(sm);

        if(_mapUserStatus.contains(xmppId))
        {
            isOnline = _mapUserStatus.get(xmppId) != "offline";
            _mapUserStatus.update(xmppId);
        }
        else
            _mapUserStatus.insert(xmppId, "offline");
    }
    return isOnline;
}

//
std::string Platform::getUserStatus(const std::string &userId)
{
    std::string status = "offline";
    {
        std::lock_guard<QTalk::util::spin_mutex> lock(sm);

        if(_mapUserStatus.contains(userId))
        {
            status = _mapUserStatus.get(userId);
            _mapUserStatus.update(userId);
        }
        else
            _mapUserStatus.insert(userId, "offline");
    }
    return status;
}

/**
 *
 */
void Platform::loadOnlineData(const std::map<std::string, std::string> &userStatus)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);

    for(const auto &state : userStatus)
        _mapUserStatus.insert(state.first, state.second);
}

std::vector<std::string> Platform::getInterestUsers()
{
    return _mapUserStatus.keys();
}

std::string Platform::getConfigPath() const
{
    if (!AppSetting::instance().getUserDirectory().empty())
        return AppSetting::instance().getUserDirectory() + "/config";

    return std::string();
}

std::string Platform::getHistoryDir() const
{
    return _strHistoryFileDir;
}

void Platform::setHistoryDir(const std::string &dir)
{
    _strHistoryFileDir = dir;
}

std::string Platform::getSelfResource() const
{
    return _strResource;
}

void Platform::setSelfResource(const std::string &resource)
{
    _strResource = resource;
}

int Platform::getDbVersion()
{
    return DB_VERSION;
}

void Platform::setMainThreadId()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    _mainThreadId = oss.str();
}

std::string Platform::getMainThreadId()
{
    return _mainThreadId;
}

bool Platform::isMainThread()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string tid = oss.str();
    return tid == _mainThreadId;
}


void Platform::setNavName(const std::string &name)
{
    _navName = name;
}


/**
 *
 */
void Platform::setAppNetVersion(long long version)
{
    APPLICATION_NET_VERSION = version;
}

/**
 *
 */
long long Platform::getAppNetVersion()
{
    return APPLICATION_NET_VERSION;
}

/**
 *
 */
void Platform::setBetaUrl(const std::string &url)
{
    beatUrl = url;
}

std::string Platform::getBetaUrl()
{
    return beatUrl;
}

void Platform::setSystemInfo(const std::string &sys)
{
    sys_str = sys;
}

std::string Platform::getSystemInfo()
{
    return sys_str;
}

void Platform::setLoginNav(const std::string &nav)
{
    loginNav = nav;
}

void Platform::setQvt(const std::string &qvt)
{
    _qvt = qvt;
}

std::string Platform::getQvt()
{
    return _qvt;
}

void Platform::setSeats(const std::string &seats)
{
    _seats = seats;
}

std::string Platform::getSeats()
{
    return _seats;
}

std::string Platform::getLoginNav()
{
    return loginNav;
}

namespace QTalk
{
    /**
      * @函数名   GetFileNameByUrl
      * @功能描述 根据url截取文件名
      * @参数
      * @author   cc
      * @date     2018/10/08
      */
    std::string GetFileNameByUrl(const std::string &url)
    {
        if (url.empty()) return std::string();

//        std::string fileName(url);
//        // 获取name=...&
//        int pos = static_cast<int>(fileName.find("name="));
//        if (pos != -1) {
//            fileName = fileName.substr(pos + strlen("name="));
//            fileName = fileName.substr(0, fileName.find_first_of('&'));
//        } else {
//            //
//            pos = fileName.find('?');
//            if(pos != -1)
//            {
//                fileName = fileName.substr(0, pos);
//            }
//
//			pos = fileName.find_last_of('/');
//			if (pos != -1)
//			{
//				fileName = fileName.substr(pos + 1);
//			}
////			pos = fileName.find_last_of('&');
////			if (pos != -1)
////			{
////				fileName = fileName.substr(pos + 1);
////			}
//            pos = fileName.find('=');
//            if(pos != -1)
//            {
//                fileName = fileName.substr(pos + 1);
//            }
//            fileName = fileName.substr(0, fileName.find_first_of('&'));
//        }
//
//        // 判断是否有后缀
//        pos = static_cast<int>(fileName.find('.'));
//        if (pos == -1 || pos == fileName.size() - 1) {
//            fileName += "." + QTalk::utils::getFileSuffix(url);
//        }
//        else if(pos == fileName.size() - 2)
//        {
//            fileName += QTalk::utils::getFileSuffix(url);
//        }
        std::ostringstream fileName;
        std::string rurl(url);
//        unsigned long t = rurl.find_first_of("w=");
//        if(t != -1)
//        {
//            rurl = rurl.substr(0, t);
//        }
        fileName << MD5(rurl).toString()
                 << "."
                 << QTalk::utils::getFileSuffix(url);
        return fileName.str();
    }

    /**
      * @函数名
      * @功能描述
      * @参数
      * @author   cc
      * @date     2018/10/09
      */
    std::string GetHeadPathByUrl(const std::string &url)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/image/headphoto/"
            << GetFileNameByUrl(url);
        return src.str();
    }

    /**
      * @函数名
      * @功能描述
      * @参数
      * @author   cc
      * @date     2018/10/16
      */
    std::string GetFilePathByUrl(const std::string &url)
    {
        std::ostringstream src;
        src << AppSetting::instance().getFileSaveDirectory()
            << "/"
            << GetFileNameByUrl(url);
        return src.str();
    }
    std::string GetImagePathByUrl(const std::string &url)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/image/temp/"
            << GetFileNameByUrl(url);
        return src.str();
    }

    std::string GetSrcImagePathByUrl(const std::string &url)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/image/source/"
            << GetFileNameByUrl(url);
        return src.str();
    }

    std::string getCollectionPath(const std::string &netPath)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/emoticon/collection/"
            << GetFileNameByUrl(netPath);
        return src.str();
    }

    std::string getOAIconPath(const std::string &netPath)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/oaIcon/"
            << GetFileNameByUrl(netPath);
        return src.str();
    }

    std::string getUserName(const std::shared_ptr<QTalk::Entity::ImUserInfo> &userInfo)
    {
        std::string ret;

        if(userInfo)
        {
            ret = DB_PLAT.getMaskName(userInfo->XmppId);

            if(ret.empty())
                ret = userInfo->NickName;

            if(ret.empty())
                ret = userInfo->Name;

            if(ret.empty())
                ret = QTalk::Entity::JID(userInfo->XmppId).username();
        }

        return ret;
    }

    std::string getUserName(const std::string &xmppId)
    {
        std::string ret;

        if(!xmppId.empty())
        {
            auto info = DB_PLAT.getUserInfo(xmppId);
            ret = getUserName(info);

            if(ret.empty())
                ret = QTalk::Entity::JID(xmppId).username();
        }

        return ret;
    }

    std::string getUserNameNoMask(const std::shared_ptr<QTalk::Entity::ImUserInfo> &userInfo)
    {
        std::string ret;

        if(userInfo)
        {
            ret = userInfo->NickName;

            if(ret.empty())
                ret = userInfo->Name;

            if(ret.empty())
                ret = QTalk::Entity::JID(userInfo->XmppId).username();
        }

        return ret;
    }

    std::string getUserNameNoMask(const std::string &xmppId)
    {
        std::string ret;

        if(!xmppId.empty())
        {
            auto info = DB_PLAT.getUserInfo(xmppId);
            ret = getUserNameNoMask(info);

            if(ret.empty())
                ret = QTalk::Entity::JID(xmppId).username();
        }

        return ret;
    }

    std::string getVideoPathByUrl(const std::string &url)
    {
        //
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/video/"
            << GetFileNameByUrl(url);
        return src.str();
    }

    std::string getGroupName(const std::shared_ptr<QTalk::Entity::ImGroupInfo> &groupInfo)
    {
        std::string ret;

        if(groupInfo)
        {
            ret = groupInfo->Name;

            if(ret.empty())
                ret = QTalk::Entity::JID(groupInfo->GroupId).username();
        }

        return ret;
    }

    std::string getGroupName(const std::string &xmppId)
    {
        std::string ret;

        if(!xmppId.empty())
        {
            auto info = DB_PLAT.getGroupInfo(xmppId);
            ret = getGroupName(info);
        }

        return ret;
    }

    std::string getMedalPath(const std::string &link)
    {
        std::ostringstream src;
        src << PLAT.getAppdataRoamingUserPath()
            << "/image/medal/"
            << GetFileNameByUrl(link);
        return src.str();
    }
}

time_t build_time()
{
    string datestr = __DATE__;
    string timestr = __TIME__;
    istringstream iss_date( datestr );
    string str_month;
    int day;
    int year;
    iss_date >> str_month >> day >> year;
    int month;

    if     ( str_month == "Jan" ) month = 1;
    else if( str_month == "Feb" ) month = 2;
    else if( str_month == "Mar" ) month = 3;
    else if( str_month == "Apr" ) month = 4;
    else if( str_month == "May" ) month = 5;
    else if( str_month == "Jun" ) month = 6;
    else if( str_month == "Jul" ) month = 7;
    else if( str_month == "Aug" ) month = 8;
    else if( str_month == "Sep" ) month = 9;
    else if( str_month == "Oct" ) month = 10;
    else if( str_month == "Nov" ) month = 11;
    else if( str_month == "Dec" ) month = 12;
    else exit(-1);

    for( string::size_type pos = timestr.find( ':' ); pos != string::npos; pos = timestr.find( ':', pos ) )
        timestr[ pos ] = ' ';

    istringstream iss_time( timestr );
    int hour, min, sec;
    iss_time >> hour >> min >> sec;
    tm t = {0};
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_year = year - 1900;
    t.tm_hour = hour - 1;
    t.tm_min = min;
    t.tm_sec = sec;
    return mktime(&t);
}
