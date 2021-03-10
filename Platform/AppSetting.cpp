//
// Created by admin on 2019-02-22.
//

#include "AppSetting.h"
#include "../QtUtil/nJson/nJson.h"

AppSetting::AppSetting()
: _newMsgAudioNotify(true)
        , _newMsgTipWindowNotify(true)
        , _phoneAnyReceive(false)
        , _strongWarn(true)
        , _hotCutEnable(true)
        , _screentShotHideWndFlag(false)
        , _showMoodFlag(true)
        , _autoDeleteSession(true)
        , _leaveCheckEnable(false)
//        , _leaveMinute(5)
//        , _autoReplyPreset(true)
        , _autoReplyMsg("")
        , _themeMode(1)
        , _fontLevel(FONT_LEVEL_NORMAL)
        , _autoStartUp(false)            // 开机自启动
        , _openOaWithAppBrowser(true)
        , _showSendMessageBtn(false)
        , _supportNativeMessagePrompt(false)
        , _useNativeMessagePrompt(false)
{

}

void AppSetting::initAppSetting(const std::string& setting)
{
    nJson obj = Json::parse(setting);
    if (obj != nullptr)
    {
        std::lock_guard<QTalk::util::spin_mutex> lock(sm);
        _newMsgAudioNotify = Json::get<bool>(obj, "NEWMSGAUDIO", _newMsgAudioNotify);

        _newMsgAudioNotify = Json::get<bool>(obj, "NEWMSGAUDIO", _newMsgAudioNotify);
        _newMsgAudioPath = Json::get<std::string>(obj, "NEWMSGAUDIOPATH", _newMsgAudioPath);
        _newMsgTipWindowNotify = Json::get<bool>(obj, "NEWMSGTIPWINDOW", _newMsgTipWindowNotify);
        _screenshotKey = Json::get<std::string>(obj, "SCREENSHOT", _screenshotKey);
        _wakeWndKey = Json::get<std::string>(obj, "WAKEWND", _wakeWndKey);
        _openOaWithAppBrowser = Json::get<bool>(obj, "OPENOAWITHAPPBROWSER", _openOaWithAppBrowser);
        _screentShotHideWndFlag = Json::get<bool>(obj, "SCREENTSHOTHIDEWNDFLAG", _screentShotHideWndFlag);
        _showSendMessageBtn = Json::get<bool>(obj, "SHOWSENDMESSAGEBTN", _showSendMessageBtn);
        _strongWarn = Json::get<bool>(obj, "STRONGWARN", _strongWarn);
        _hotCutEnable = Json::get<bool>(obj, "HOTCUTENABLE", _hotCutEnable);
        _sendMessageKey = Json::get<std::string>(obj, "SENDMESSAGEKEY", "Enter");
        _showMoodFlag = Json::get<bool>(obj, "SHOWMOODFLAG", true);
        _autoStartUp = Json::get<bool>(obj, "AUTOSTARTUP", false);
        _leaveCheckEnable = Json::get<bool>(obj, "LEAVECHECKENABLE", false);
        _awaysAutoReply = Json::get<bool>(obj, "AWAYSAUTOREPLY", false);
        _autoReplyMsg = Json::get<std::string>(obj, "AUTOREPLYMSG");
        _leaveMinute = Json::get<int>(obj, "LEAVEMINUTE", 5);
        _useNativeMessagePrompt = Json::get<bool>(obj, "USENATIVEMESSAGEPROMPT", false);
        _autoDeleteSession = Json::get<bool>(obj, "AUTODELETESESSION", true);
        _autoReplyStartTime = Json::get<int>(obj, "AUTOREPLYSTARTTIME", 0);
        _autoReplyEndTime = Json::get<int>(obj, "AUTOREPLYENDTIME", 24);

    }
}

AppSetting & AppSetting::instance()
{
    static AppSetting appSetting;
	return appSetting;
	
}

// 消息通知
bool  AppSetting::getNewMsgAudioNotify() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _newMsgAudioNotify;
} //新消息声音提醒

void AppSetting::setNewMsgAudioNotify(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _newMsgAudioNotify = flag;
}

std::string AppSetting::getgetNewMsgAudioPath()
{
	return _newMsgAudioPath;
}

void AppSetting::setgetNewMsgAudioPath(const std::string & path)
{
	_newMsgAudioPath = path;
}

bool AppSetting::getNewMsgTipWindowNotify() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _newMsgTipWindowNotify;
}    //新消息提示窗

void AppSetting::setNewMsgTipWindowNotify(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _newMsgTipWindowNotify = flag;
}

bool AppSetting::getPhoneAnyReceive() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _phoneAnyReceive;
}       //手机端随时接收推送

void AppSetting::setPhoneAnyReceive(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _phoneAnyReceive = flag;
}

void AppSetting::setStrongWarnFlag(bool flag)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _strongWarn = flag;
}

bool AppSetting::getStrongWarnFlag()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _strongWarn;
}

// 热键设置
bool AppSetting::getHotCutEnable() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _hotCutEnable;
}          //热键是否开启

void AppSetting::setHotCutEnable(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _hotCutEnable = flag;
}

std::string  AppSetting::getScreenshotHotKey() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _screenshotKey;
}        //截图快捷键

void AppSetting::setScreenshotHotKey(const std::string& hotKey) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _screenshotKey = hotKey;
}

std::string  AppSetting::getWakeWndHotKey() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _wakeWndKey;
}

void AppSetting::setWakeWndHotKey(const std::string& hotKey) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _wakeWndKey = hotKey;
}

// 自动回复
bool AppSetting::getAutoReplyEnable() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _leaveCheckEnable;
}

void AppSetting::setAutoReplyEnable(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _leaveCheckEnable = flag;
}

int AppSetting::getLeaveMinute() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _leaveMinute;
}

void AppSetting::setLeaveMinute(int minute) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _leaveMinute = minute;
}

bool AppSetting::getAwaysAutoReply() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _awaysAutoReply;
}

void AppSetting::setAwaysAutoReply(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _awaysAutoReply = flag;
}

std::string AppSetting::getAutoReplyMsg() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoReplyMsg;
}

void AppSetting::setAutoReplyMsg(std::string msg) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoReplyMsg = std::move(msg);
}

//std::string AppSetting::getAutoReplyCusMsg() {
//    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
//    return _autoReplyCusMsg;
//}
//
//void AppSetting::setAutoReplyCusMsg(std::string msg) {
//    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
//    _autoReplyCusMsg = std::move(msg);
//}

void AppSetting::setAutoReplyStartTime(int time)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoReplyStartTime = time;
}

void AppSetting::setAutoReplyEndTime(int time)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoReplyEndTime = time;
}

//
bool AppSetting::withinEnableTime(int hour)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return hour >= _autoReplyStartTime && hour <= _autoReplyEndTime;
}

//
int AppSetting::getAutoReplyStartTime()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoReplyStartTime;
}

//
int AppSetting::getAutoReplyEndTime()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoReplyEndTime;
}

// 文件目录
std::string AppSetting::getUserDirectory() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _userDirectory;
}

void AppSetting::setUserDirectory(const std::string& userDirectory) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _userDirectory = userDirectory;
}

std::string AppSetting::getFileSaveDirectory() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _fileSaveDirectory;
}

void AppSetting::setFileSaveDirectory(const std::string& fileSaveDirectory) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _fileSaveDirectory = fileSaveDirectory;
}

// 皮肤设置
int AppSetting::getThemeMode() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _themeMode;
}

void AppSetting::setThemeMode(int mode) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _themeMode = mode;
}

std::string AppSetting::getFont() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _font;
}

void AppSetting::setFont(const std::string& font) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _font = font;
}

int AppSetting::getFontLevel() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _fontLevel;
}

void AppSetting::setFontLevel(int level) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _fontLevel = level;
}

int AppSetting::getLanguage() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _language;
}

void AppSetting::setLanguage(int language) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _language = language;
}

// 其他设置
bool AppSetting::getSelfStart() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoStartUp;
}

// 开机自启动
void AppSetting::setSelfStart(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoStartUp = flag;
}

bool AppSetting::getAutoLoginEnable() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoLoginEnable;
}            // 是否自动登录

void AppSetting::setAutoLoginEnable(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoLoginEnable = flag;
}


bool AppSetting::getOpenOaLinkWithAppBrowser() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _openOaWithAppBrowser;
}

void AppSetting::setOpenOaLinkWithAppBrowser(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _openOaWithAppBrowser = flag;
}

void AppSetting::setScreentShotHideWndFlag(bool hide) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _screentShotHideWndFlag = hide;
}

bool AppSetting::getScreentShotHideWndFlag() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _screentShotHideWndFlag;
}

void AppSetting::setShowSendMessageBtnFlag(bool flag) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _showSendMessageBtn = flag;
}

bool AppSetting::getShowSendMessageBtnFlag() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _showSendMessageBtn;
}

void AppSetting::setSendMessageKey(std::string key)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _sendMessageKey = std::move(key);
}

std::string AppSetting::getSendMessageKey()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _sendMessageKey;
}

void AppSetting::setShowMoodFlag(bool flag)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _showMoodFlag = flag;
}

bool AppSetting::getShowMoodFlag() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _showMoodFlag;
}

void AppSetting::setAutoDeleteSession(bool autoDelete)
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _autoDeleteSession = autoDelete;
}

bool AppSetting::getAutoDeleteSession()
{
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _autoDeleteSession;
}

void AppSetting::setNewVersion(int ver) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _newVersion = ver;
}


int AppSetting::getNewVersion() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _newVersion ;
}

void AppSetting::setEnableProxy(bool enable) {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    _enableProxy = enable ;
}

bool AppSetting::enableProxy() {
    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    return _enableProxy ;
}


//std::string AppSetting::getCoEdit() {
//    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
//    return _coEditor;
//}
//
//void AppSetting::setCoEdit(const std::string& edit) {
//    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
//    _coEditor = edit;
//}

/**
 *
 * @return
 */
std::string AppSetting::saveAppSetting() {

    std::lock_guard<QTalk::util::spin_mutex> lock(sm);
    nJson obj ;
    obj["NEWMSGAUDIO"] = _newMsgAudioNotify;
    obj["NEWMSGAUDIOPATH"] = _newMsgAudioPath.data();
    obj["NEWMSGTIPWINDOW"] = _newMsgTipWindowNotify;
    obj["SCREENSHOT"] = _screenshotKey.data();
    obj["WAKEWND"] = _wakeWndKey.data();
    obj["OPENOAWITHAPPBROWSER"] = _openOaWithAppBrowser;
    obj["SCREENTSHOTHIDEWNDFLAG"] = _screentShotHideWndFlag;
    obj["SHOWSENDMESSAGEBTN"] = _showSendMessageBtn;
    obj["STRONGWARN"] = _strongWarn;
    obj["HOTCUTENABLE"] = _hotCutEnable;
    obj["SENDMESSAGEKEY"] = _sendMessageKey.data();
    obj["SHOWMOODFLAG"] = _showMoodFlag;
    obj["AUTOSTARTUP"] = _autoStartUp;
    obj["LEAVECHECKENABLE"] = _leaveCheckEnable;
    obj["AWAYSAUTOREPLY"] = _awaysAutoReply;
    obj["AUTOREPLYMSG"] = _autoReplyMsg.data();
    obj["LEAVEMINUTE"] = _leaveMinute;
    obj["USENATIVEMESSAGEPROMPT"] = _useNativeMessagePrompt;
    obj["AUTODELETESESSION"] = _autoDeleteSession;
    obj["AUTOREPLYSTARTTIME"] = _autoReplyStartTime;
    obj["AUTOREPLYENDTIME"] = _autoReplyEndTime;

    return obj.dump(4);
}
