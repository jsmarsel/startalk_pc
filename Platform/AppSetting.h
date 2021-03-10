//
// Created by admin on 2019-02-22.
//

#ifndef QTALK_V2_APPSETTING_H
#define QTALK_V2_APPSETTING_H

#include <string>
#include "platform_global.h"
#include "../include/Spinlock.h"

class PLATFORMSHARED_EXPORT AppSetting{

public:
    AppSetting() ;

public:

    enum MsgFontSize {
        Normal,
        Big,
        MostBig,
    };

//    enum AddFriendMode {
//        AllAgree,           //全部接受
//        QuestionAndAnswer,  //需要问答
//        NeedAgree,          //需要确认
//    };

public:
    std::string saveAppSetting();
	void initAppSetting(const std::string& setting);
    // 消息通知
    bool getNewMsgAudioNotify();     //新消息声音提醒
    void setNewMsgAudioNotify(bool flag);
	std::string getgetNewMsgAudioPath();   //
	void setgetNewMsgAudioPath(const std::string& path);   //
    bool getNewMsgTipWindowNotify();    
    void setNewMsgTipWindowNotify(bool flag);
    bool getPhoneAnyReceive();       //手机端随时接收推送
    void setPhoneAnyReceive(bool flag);
    bool getStrongWarnFlag();
    void setStrongWarnFlag(bool flag);

    // 热键设置
    bool getHotCutEnable();          //热键是否开启
    void setHotCutEnable(bool flag);
    std::string getScreenshotHotKey();        //截图快捷键
    void setScreenshotHotKey(const std::string& hotKey);
    std::string getWakeWndHotKey();        //截图快捷键
    void setWakeWndHotKey(const std::string& hotKey);

    void setScreentShotHideWndFlag(bool hide);
    bool getScreentShotHideWndFlag();

    // 会话设置
    void setSendMessageKey(std::string key); // 发送快捷键
    std::string getSendMessageKey();
    void setShowMoodFlag(bool flag);
    bool getShowMoodFlag();

    void setAutoDeleteSession(bool autoDelete);
    bool getAutoDeleteSession();

    // 自动回复
    bool getAutoReplyEnable();
    void setAutoReplyEnable(bool flag);
    int  getLeaveMinute();
    void setLeaveMinute(int minute);
    bool getAwaysAutoReply();
    void setAwaysAutoReply(bool flag);
    std::string getAutoReplyMsg();
    void setAutoReplyMsg(std::string msg);
//    std::string getAutoReplyCusMsg();
//    void setAutoReplyCusMsg(std::string msg);

    void setAutoReplyStartTime(int time);
    void setAutoReplyEndTime(int time);
    bool withinEnableTime(int hour);
    int getAutoReplyStartTime();
    int getAutoReplyEndTime();

    // 文件目录
    std::string getUserDirectory();
    void setUserDirectory(const std::string& userDirectory);
    std::string getFileSaveDirectory();
    void setFileSaveDirectory(const std::string& fileSaveDirectory);
    // 皮肤设置
    int getThemeMode();
    void setThemeMode(int mode);
    std::string getFont();
    void setFont(const std::string& font);
    // 其他设置
    bool getSelfStart();            // 开机自启动
    void setSelfStart(bool flag);

//    void setLogLevel(int level);
//    int getLogLevel();

    bool getAutoLoginEnable();            // 是否自动登录
    void setAutoLoginEnable(bool flag);

    bool getOpenOaLinkWithAppBrowser(); // 使用内置浏览器打开OA链接
    void setOpenOaLinkWithAppBrowser(bool flag);

    bool getShowSendMessageBtnFlag(); // 显示发送按钮
    void setShowSendMessageBtnFlag(bool flag);

    int getTestchannel() { return _channel;};
    void setTestchannel(int channel) {_channel = channel;};

    void setNativeMessagePromptEnable(bool flag) { _supportNativeMessagePrompt = flag; };
    bool getNativeMessagePromptEnable() { return _supportNativeMessagePrompt; };
    void setUseNativeMessagePrompt(bool flag) { _useNativeMessagePrompt = flag; };
    bool getUseNativeMessagePrompt() { return _supportNativeMessagePrompt && _useNativeMessagePrompt; };

//    void setCoEdit(const std::string& coEdit);
//    std::string getCoEdit();

    enum {FONT_LEVEL_NORMAL = 0, FONT_LEVEL_BIG, FONT_LEVEL_SMALL};
    void setFontLevel(int level);
    int getFontLevel();
    //
    void setLanguage(int language);
    int getLanguage();

public:
    void setNewVersion(int version);
    int getNewVersion();

public:
    void setEnableProxy(bool enable);
    bool enableProxy();

private:

    // 消息通知
    bool _newMsgAudioNotify;     //新消息声音提醒
	std::string _newMsgAudioPath;
    bool _newMsgTipWindowNotify;    //新消息声音提醒
    bool _phoneAnyReceive;       //手机端随时接收推送
    bool _strongWarn;

    // 热键设置
    bool _hotCutEnable;          //热键是否开启
    std::string  _screenshotKey; // 截图快捷键
    std::string  _wakeWndKey; // 唤醒屏幕快捷键
    bool _screentShotHideWndFlag;

    // 会话设置
    std::string _sendMessageKey;
    bool _showMoodFlag;
    bool _autoDeleteSession;

    // 自动回复
    bool _leaveCheckEnable;
    int  _leaveMinute{5};
//    bool _autoReplyPreset;
    std::string _autoReplyMsg;
//    std::string _autoReplyCusMsg;
    int _autoReplyStartTime{0};
    int _autoReplyEndTime{24};
    bool _awaysAutoReply{false};

    // 文件目录
    std::string _userDirectory;
    std::string _fileSaveDirectory;

    // 皮肤设置
    int _themeMode;
    std::string _font;
    int _fontLevel;
    int _language{};

    // 其他设置
    bool _autoStartUp;            // 开机自启动
    bool _autoLoginEnable;
    bool _openOaWithAppBrowser;
    bool _showSendMessageBtn;
    int _channel;
//    int _logLevel;
    //
    bool _supportNativeMessagePrompt;
    bool _useNativeMessagePrompt;
    //
    int _newVersion {};

public:
    bool with_ssl = true;
    bool _enableProxy = false;

private:
    QTalk::util::spin_mutex sm;

public:
	static AppSetting& instance();
};


#endif //QTALK_V2_APPSETTING_H
