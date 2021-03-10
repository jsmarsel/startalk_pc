#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFrame>
#include <QDate>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QSystemTrayIcon>
#include <QMutexLocker>
#include "HotKey/qhotkey.h"
#include "NoOperationThread.h"
#include "LocalServer.h"
#include <QStackedLayout>
#include <QtQuick/QtQuick>
#include "../CustomUi/UShadowWnd.h"
#include "../QtUtil/lib/ini/ConfigLoader.h"
#include "../include/CommonStrcut.h"
#include "../include/STLazyQueue.h"
#include "../UICom/UIEntity.h"
#include "ProcessInfo.h"
#include "qml/QmlView.h"

#ifdef _MACOS
#include <QMacNativeWidget>
#include <QMenuBar>
#endif

class IUITitlebarPlug;
class IUINavigationPlug;
class IUIChatViewPlug;
class IUICardManagerPlug;
class QSystemTrayIcon;
class QTalkMsgListener;
class QTalkMsgManager;
class IUILoginPlug;
class IUIGroupManagerPlug;
class IUIPictureBroswerPlug;
class IUIAddressBookPlug;
class IUIOAManagerPlug;
class IUITScreenPlug;
class SystemTray;
class MainWindow : public UShadowDialog
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

	
public:
	void OnLoginSuccess(const std::string& strSessionId);
    void onAppActive();
    void onAppDeactivate();
    void saveWndState();
    void initSystemTray();
    //
    void checkUpdater();
    void onCheckUpdater(bool hasUpdate, const QString& link, bool force);
    void onGetHistoryError();
    QWidget* getActiveWnd();
    //
    void reLoginWithErr(const std::string &err);
    //
    void systemQuitByEvt();

public:
	inline IUILoginPlug* getLoginPlug() { return _pLoginPlug; }

Q_SIGNALS:
	void LoginSuccess(bool);
	void sgCheckUpdate(bool, const QString&, bool);
	void systemQuitSignal();
	void appDeactivated();
	void systemShortCut();
	void sgAppActive();
	void sgRunNewInstance();
	void sgResetOperator();
	void sgRestartWithMessage(const QString&);
    void sgJumtoSession(const StSessionInfo&);
    void sgSystemQuit();
    void sgShowUpdateClientLabel(bool);

public slots:
	void InitLogin(bool, const QString& loginMsg);
	void LoginResult(bool result);
	void openMainWindow();
	void onCurFunChanged(int index);
	void systemQuit();
	void setAutoLogin(bool autoFlag);
	void onUpdateHotKey();
	void wakeUpWindow();
    //
    void addOperatorLog(const QString& desc);
    void setUserStatus(bool);
    void startTScreen();

private slots:
    //
//    void onShockWnd();
    void onSaveSysConfig();
    void onUserSendMessage();
    void restartWithMessage(const QString& msg);
    void onScreenRemoved(QScreen *screen);
//    void onUpdateClient();
#ifdef Q_OS_MAC
    void onShowMinWnd();
#endif
    void onHourTimer();
    void onShowCheckUpdateWnd();

    // QWidget interface
protected:
	void closeEvent(QCloseEvent *e) override;
	void hideEvent(QHideEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
    void changeEvent(QEvent * event) override;
	bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
	bool event(QEvent* e) override ;

private:
    void init();
    // qml
    void initQml();
    void initPanels();
    void initLayouts();
    void initTitleBar();
    void initNavigation();
    void initChatView();
    void initCardManager();
    void connectPlugs();
	void initGroupManager();
	void initPictureBrowser();
	//
	void initAddressBook();
	//
	void initOAManager();
	//
//	void initTScreen();
	// 心跳timer
    void sendHeartBeat();
    // 初始化位置超出屏幕处理
	void adjustWndRect();
	// 截屏
    void onScreentShot();
    //
    void dealDumpFile();

private:
    QTimer   *_timr{};

private:
    IUITitlebarPlug  *_titleBarPlugin{};
    IUINavigationPlug  *_navigationPlugin{};
    IUIChatViewPlug  *_chatViewPlugin{};
    IUICardManagerPlug *_pCardManagerPlug{};
	IUIGroupManagerPlug* _pGroupManagerPlug{};
    IUIPictureBroswerPlug *_pPictureBrowserPlug{};
	IUIAddressBookPlug *_pAddressBookPlug{};
	IUIOAManagerPlug *_pOAManagerPlug{};
    QFrame * _mainFrm{};
    QWidget  *_titleBar{};
    QWidget  *_navigationPanel{};
    QWidget  *_chatViewPanel{};
    QWidget  *_pCardManager{};
    QWidget * _pGroupManager{};
    QWidget *_pPictureBrowser{};
	QWidget *_pAddressBook{};
	QWidget *_pOAManager{};
    QVBoxLayout  *_mainLayout{};
    QFrame  *_bottomFrm{};
    QSplitter  *_bottomSplt{};

	SystemTray* _pSysTrayIcon{};


    QStackedLayout* bodyLay{};

#ifdef _MACOS
protected:
    QMenuBar*   _pWindowMenuBar{};
    QAction*    _pFeedBackLog{};
#endif

private:
	IUILoginPlug      *_pLoginPlug{};
	QDialog           *_logindlg{};

private:
    QTalk::ConfigLoader  *_pConfigLoader{};

private:
    enum {WND_INVALID, WND_NORMAL, WND_MAXSIZE, WND_FULLSCREEN};

private:
	bool _initUi{true};
	NoOperationThread  *_noOperatorThread{};

private:
    QHotkey* _pScreentShot{};
    QHotkey* _pWakeWnd{};
    STLazyQueue<bool> *_pScreentShotQueue{};

private:
    qint64 _login_t{};
    qint64 _logout_t{};
    QTimer *_pOfflineTimer{};
    bool   _isOffline{}; // 是否离线(包括逻辑离线)
    QString _ipv4Address;

    QMutex _logMutex;
    QTimer* _pLogTimer{};
    std::vector<QTalk::StActLog> _operators;

private:
    LocalServer* _pLocalServer{};
    QDate        _loginDate{};

private:
    ProcessInfo* _prcessInfo{};

private:
    QmlView* _pView;
};
#endif // MAINWINDOW_H
