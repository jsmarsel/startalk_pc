#include "MessageManager.h"
#include "../EventBus/EventBus.h"
#include "MainWindow.h"
#include "../Message/StatusMessage.h"
#include "../Message/ChatMessage.h"
#include "../Message/UserMessage.h"

void QTalkMsgManager::sendHearBeat()
{
    HeartBeat e;
    EventBus::FireEvent(e);
}

void QTalkMsgManager::sendOnlineState(const QInt64 &login_t, const QInt64 &logout_t, const std::string& ip)
{
    UserOnlineState e(login_t, logout_t, ip);
    EventBus::FireEvent(e);
}

void QTalkMsgManager::sendOperatorStatistics(const std::string& ip, const std::vector<QTalk::StActLog>& operators)
{
    OperatorStatistics e(ip, operators);
    EventBus::FireEvent(e);
}

void QTalkMsgManager::sendLogReport(const std::string& desc, const std::string &logPath)
{
    LogReportMessage e(desc, logPath);
    EventBus::FireEvent(e);
}

void QTalkMsgManager::chanegUserStatus(const std::string& status)
{
    SwitchUserStatusEvt evt(status);
    EventBus::FireEvent(evt);
}

void QTalkMsgManager::reportDump(const std::string& ip, const std::string& id, const std::string &dump, QInt64 crashTime) {
    ReportDump e(ip, id, dump, crashTime);
    EventBus::FireEvent(e);
}

void QTalkMsgManager::checkUpdater(int version) {
    CheckUpdaterEvt e;
    e.version = version;
    EventBus::FireEvent(e);
}

void QTalkMsgManager::reportLogin() {
    ReportLogin e;
    EventBus::FireEvent(e);
}

void QTalkMsgManager::addExceptCpu(double cpu, QInt64 time, const std::string &stack) {
    ExceptCpuEvt e;
    e.cpu = cpu;
    e.stack = stack;
    e.time = time;
    EventBus::FireEvent(e);
}

/*------------------*/
QTalkMsgListener::QTalkMsgListener(MainWindow* pUiControl)
	:_pUiControl(pUiControl)
{
	EventBus::AddHandler<LoginSuccessMessage>(*this);
	EventBus::AddHandler<GetHistoryError>(*this);
	EventBus::AddHandler<CheckUpdaterResultEvt>(*this);
}

//
void QTalkMsgListener::onEvent(LoginSuccessMessage& e)
{
	if (nullptr != _pUiControl)
	{
		_pUiControl->OnLoginSuccess(e.getSessionId());
	}
}

void QTalkMsgListener::onEvent(GetHistoryError &e) {
    if (nullptr != _pUiControl) {
        _pUiControl->onGetHistoryError();
    }
}

void QTalkMsgListener::onEvent(CheckUpdaterResultEvt &e) {
    if (nullptr != _pUiControl) {
        _pUiControl->onCheckUpdater(e.hasUpdate, e.link.data(), e.forceUpdate);
    }
}

void QTalkMsgListener::onEvent(GoBackLoginWndEvt &e) {
    if(_pUiControl)
        _pUiControl->reLoginWithErr(e.reason);
}

void QTalkMsgListener::onEvent(SystemQuitEvt &e) {
    if(_pUiControl)
        _pUiControl->systemQuitByEvt();
}