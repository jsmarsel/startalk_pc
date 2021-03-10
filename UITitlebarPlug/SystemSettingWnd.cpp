#include <utility>

//
// Created by admin on 2019-02-19.
//

#include "SystemSettingWnd.h"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QToolButton>
#include <QListWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QScrollBar>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QSpinBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonArray>
#include <QButtonGroup>
#include <QApplication>
#include <QDate>
#include "../include/Line.h"
#include "DropMenu.h"
#include "../Platform/Platform.h"
#include "../CustomUi/QtMessageBox.h"
#include "SKRecorder.h"
#include "../QtUtil/Utils/Log.h"
#include "../CustomUi/TitleBar.h"

#include <QProcess>
#include <QFontDatabase>
#include <QDebug>
#include <QSettings>
#include <QPainter>
#include <QFontMetrics>

void ComboBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
//    QStyledItemDelegate::paint(painter, option, index);

    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);

    QRect rect = option.rect;

    auto text = index.data(Qt::DisplayRole).toString();
    painter->setFont(QFont(text));
    painter->drawText(rect, tr("%1( 中国 China 0123 )").arg(text));
    painter->restore();
    painter->drawLine(rect.x(), rect.y(), rect.width(), rect.y());
}

QSize ComboBoxDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const  {
    auto text = index.data(Qt::DisplayRole).toString();
    QFontMetrics fm((QFont(text)));
    return {200, fm.height() + 8};
}

SystemSettingWnd::SystemSettingWnd(QWidget *parent)
        : UShadowDialog(parent, true){

    initUi();
}

SystemSettingWnd::~SystemSettingWnd() = default;

void SystemSettingWnd::initUi()
{
    this->setFixedSize(648,573);
    auto mainLay = new QVBoxLayout(_pCenternWgt);
    mainLay->setMargin(0);
    mainLay->setSpacing(0);
    //
    auto* titleFrm = new TitleBar(tr("系统设置"), this, this);
    mainLay->addWidget(titleFrm);
    setMoverAble(true, titleFrm);

    auto contentFrm = new QFrame(this);
    contentFrm->setObjectName("SettingContentFrm");
    mainLay->addWidget(contentFrm);

    auto *colLay = new QHBoxLayout(contentFrm);
    colLay->setSpacing(0);
    colLay->setMargin(0);

    auto navFrm = new QFrame(this);
    navFrm->setObjectName("NavFrm");
    colLay->addWidget(navFrm);
    _pNavlayout = new QVBoxLayout(navFrm);
    _pNavlayout->setAlignment(Qt::AlignmentFlag::AlignHCenter);
    _pNavlayout->setMargin(0);
    _pNavlayout->setSpacing(0);

    colLay->addWidget(new Line(Qt::Vertical));

    auto settingFrm = new QFrame(this);
    settingFrm->setObjectName("SettingFrm");
    colLay->addWidget(settingFrm);
    auto *Settinglayout = new QVBoxLayout(settingFrm);
    Settinglayout->setSpacing(0);
    Settinglayout->setMargin(0);

    //
    _settingListWidget = new QListWidget(this);
    _settingListWidget->setFixedWidth(452);
    _settingListWidget->setFrameShape(QFrame::NoFrame);
    _settingListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _settingListWidget->verticalScrollBar()->setSingleStep(15);
    _settingListWidget->setObjectName("SettingListWidget");
    _settingListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _settingListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    _settingListWidget->installEventFilter(this);
    Settinglayout->addWidget(_settingListWidget);
    //
    for(int index = EM_SETTING_MESSAGE;
        index <= EM_SETTING_SUGGESTION; index++)
    {
        initSetting(index);
    }
    _pNavlayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding));
#ifdef _MACOS
    setWindowFlags(this->windowFlags() | Qt::Dialog);
#endif
}

void SystemSettingWnd::initSetting(int type)
{
    QString text, objName;
    auto itemFrm = new QFrame(this);
    itemFrm->setObjectName("fileDirectoryFrm");
    itemFrm->installEventFilter(this);

    auto *vlayout = new QVBoxLayout(itemFrm);
    vlayout->setSpacing(10);
    vlayout->setContentsMargins(30,30,30,1);

    auto *titleLbl = new QLabel(this);
    titleLbl->setObjectName("SettingTitleLbl");
    vlayout->addWidget(titleLbl);

    itemFrm->setFixedSize(450, 300);
    switch(type)
    {
        case EM_SETTING_MESSAGE:
        {
            text = tr("消息设置");
            objName = "msgNotifyLbl";

            initMsgNotify(vlayout);
            break;
        }
        case EM_SETTING_HOTKEY:
        {
            text = tr("热键设置");
            objName = "hotCutLbl";
            initHotCut(vlayout);
            break;
        }
        case EM_SETTING_SESSION:
        {
            text = tr("会话设置");
            objName = "sessionLbl";
            initSession(vlayout);
            break;
        }
        case EM_SETTING_AUTOREPLY:
        {
            text = tr("自动回复");
            objName = "sessionLbl";
            itemFrm->setFixedSize(450, 450);
            initAutoReply(vlayout);
            break;
        }
        case EM_SETTING_FOLDER:
        {
            text = tr("文件设置");
            objName = "autoReplyLbl";
            initFolder(vlayout);
            break;
        }
//        case EM_SETTING_FRIEND:
//        {
//            text = tr("好友验证");
//            objName = "friendAuthorityLbl";
//            initFriendAuthority(vlayout);
//            break;
//        }
        case EM_SETTING_FONT:
        {
            text = tr("外观设置");
            objName = "fontSettingLbl";
            initFontSetting(vlayout);
            break;
        }
        case EM_SETTING_OTHERSETTING:
        {
            text = tr("其他设置");
            objName = "otherSettingLbl";
            initOtherSetting(vlayout);
            break;
        }
        case EM_SETTING_VERSION:
        {
            text = tr("版本信息");
            objName = "versionInfoLbl";
            initVersionInfo(vlayout);
            break;
        }
        case EM_SETTING_SUGGESTION:
        {
            text = tr("意见反馈");
            objName = "feedbackLbl";
            itemFrm->setFixedHeight(500);
            initFeedback(vlayout);
            break;
        }
        default:
            return;
    }
    //
    titleLbl->setText(text);

    auto *actLabel = new ActionLabel(text, this);
    actLabel->setContentsMargins(25, 0, 0, 0);
    actLabel->setCheckAble(true);
    actLabel->setFixedSize(176, 52);
    actLabel->setObjectName(objName);
    _pNavlayout->addWidget(actLabel);

    auto *settingItem = new QListWidgetItem(_settingListWidget);
    settingItem->setSizeHint(itemFrm->frameSize());
    _settingListWidget->addItem(settingItem);
    _settingListWidget->setItemWidget(settingItem, itemFrm);

    _mapSettingWgt[itemFrm] = actLabel;
    _mapSettingItems[settingItem] = actLabel;
    actLabel->setCheckState(_mapSettingItems.size() == 1);
    connect(actLabel, &ActionLabel::clicked, [this,  actLabel, settingItem](){
        _settingListWidget->scrollToItem(settingItem, QAbstractItemView::PositionAtTop);
        for(const auto lab : _mapSettingItems.values())
            lab->setCheckState(lab == actLabel);
    });
}

/**
 *
 * @param o
 * @param e
 * @return
 */
bool SystemSettingWnd::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Enter)
    {
        auto *wgt = qobject_cast<QFrame*>(o);
        if(wgt && _mapSettingWgt.contains(wgt))
        {
            static QFrame *curWgt = nullptr;
            if(wgt != curWgt)
            {
                if(nullptr != curWgt)
                    _mapSettingWgt[curWgt]->setCheckState(false);
                _mapSettingWgt[wgt]->setCheckState(true);
                curWgt = wgt;
            }
        }
    }

    return QDialog::eventFilter(o,e);
}

void SystemSettingWnd::hideEvent(QHideEvent* e)
{
	emit saveConfig();
	QWidget::hideEvent(e);
}

void SystemSettingWnd::initMsgNotify(QVBoxLayout* vlayout) {

	auto *msgAudioCK = new SettingCheckBox(tr("新消息声音提醒"), AppSetting::instance().getNewMsgAudioNotify(), this);
	auto *nativeNotify = new SettingCheckBox(tr("使用系统提示窗"), AppSetting::instance().getUseNativeMessagePrompt(), this);
	auto *msgNotifyCK = new SettingCheckBox(tr("显示新消息提示窗"), AppSetting::instance().getNewMsgTipWindowNotify(), this);
	auto *msgPhonePushCK = new SettingCheckBox(tr("手机端随时接收推送"), AppSetting::instance().getPhoneAnyReceive(), this);
	auto *strongWarn = new SettingCheckBox(tr("消息强提醒"), AppSetting::instance().getStrongWarnFlag(), this);
    nativeNotify->setEnabled(AppSetting::instance().getNativeMessagePromptEnable());

#ifdef _MACOS
    nativeNotify->setVisible(false);
#endif

	auto* msgAudioLay = new QHBoxLayout;
	auto* changeSoundBtn = new QPushButton(tr("更换提示音"));
	changeSoundBtn->setObjectName("SettingBtn");
    msgAudioLay->addWidget(msgAudioCK);
    msgAudioLay->addWidget(changeSoundBtn);
    msgAudioLay->setAlignment(changeSoundBtn, Qt::AlignRight);

    vlayout->addLayout(msgAudioLay);
    vlayout->addWidget(nativeNotify);
    vlayout->addWidget(msgNotifyCK);
    vlayout->addWidget(msgPhonePushCK);
    vlayout->addWidget(strongWarn);
    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));

    msgPhonePushCK->setVisible(false);

    connect(msgNotifyCK, &QCheckBox::stateChanged,[](int state){
        AppSetting::instance().setNewMsgTipWindowNotify(state == Qt::Checked);
    });
    connect(msgAudioCK, &QCheckBox::stateChanged,[](int state){
        AppSetting::instance().setNewMsgAudioNotify(state == Qt::Checked);
    });
    connect(msgPhonePushCK, &QCheckBox::stateChanged,[](int state){
        AppSetting::instance().setPhoneAnyReceive(state==Qt::Checked);
    });
    connect(strongWarn, &QCheckBox::stateChanged,[](int state){
        AppSetting::instance().setStrongWarnFlag(state==Qt::Checked);
    });
    connect(nativeNotify, &QCheckBox::stateChanged,[](int state){
        AppSetting::instance().setUseNativeMessagePrompt(state==Qt::Checked);
    });

    connect(changeSoundBtn, &QPushButton::clicked, this, &SystemSettingWnd::msgSoundChanged);
}

void SystemSettingWnd::initHotCut(QVBoxLayout* vlayout) {

    auto *hotCutEnableCK = new SettingCheckBox(tr("启用热键"), AppSetting::instance().getHotCutEnable(), this);
    vlayout->addWidget(hotCutEnableCK);

	QKeySequence screenShot;
	QKeySequence wakeWnd;

	std::string screenShotHotKey =  AppSetting::instance().getScreenshotHotKey();
	std::string wakeUpHotKey =  AppSetting::instance().getWakeWndHotKey();

	if (screenShotHotKey.empty())
	{
		screenShot = QKeySequence(Qt::Key_A|Qt::AltModifier);
		screenShotHotKey = screenShot.toString(QKeySequence::NativeText).toStdString();
		AppSetting::instance().setScreenshotHotKey(screenShotHotKey);
	}
	else 
		screenShot = QKeySequence(QString::fromStdString(screenShotHotKey), QKeySequence::NativeText);

	if (wakeUpHotKey.empty())
	{
		wakeWnd = QKeySequence(Qt::Key_W | Qt::AltModifier);
		wakeUpHotKey = wakeWnd.toString(QKeySequence::NativeText).toStdString();
		AppSetting::instance().setWakeWndHotKey(wakeUpHotKey);
	}
	else 
		wakeWnd = QKeySequence(QString::fromStdString(wakeUpHotKey), QKeySequence::NativeText);

    SKRecorder* screenItem = addSortCutItem(tr("屏幕捕捉"), screenShot, vlayout, [this](const QKeySequence& keys){
        std::string hotKey = keys.toString(QKeySequence::NativeText).toStdString();
        AppSetting::instance().setScreenshotHotKey(hotKey);
        emit sgUpdateHotKey();
    });
    SKRecorder* wakeUpItem = addSortCutItem(tr("快速唤起"), wakeWnd, vlayout,[this](const QKeySequence& keys){
        std::string hotKey = keys.toString(QKeySequence::NativeText).toStdString();
        AppSetting::instance().setWakeWndHotKey(hotKey);
        emit sgUpdateHotKey();
    });

    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));

    connect(hotCutEnableCK, &QCheckBox::stateChanged,[screenItem, wakeUpItem](int state){
        bool isOk = state==Qt::Checked;
        AppSetting::instance().setHotCutEnable(isOk);
        screenItem->setEnabled(isOk);
        wakeUpItem->setEnabled(isOk);
    });
}

/**
 *
 * @param text
 * @param defaultVal
 * @param layout
 */
SKRecorder* SystemSettingWnd::addSortCutItem(const QString& text, const QKeySequence& keys,
        QVBoxLayout* layout, std::function<void(const QKeySequence& keys)> callback)
{
    auto *itemLay = new QHBoxLayout;
    itemLay->setMargin(0);
    itemLay->setSpacing(20);

    layout->addLayout(itemLay);

    auto *tLabel = new QLabel(text, this);
    tLabel->setObjectName("SettingTLabel");
    itemLay->addWidget(tLabel);

    auto *cutEdit = new SKRecorder(this,300,32);
    cutEdit->setShorCut(keys);
    cutEdit->setKeyChangeEvent(std::move(callback));
    itemLay->addWidget(cutEdit);

    itemLay->setStretch(0, 0);
    itemLay->setStretch(1, 1);

    return cutEdit;
}

void SystemSettingWnd::initSession(QVBoxLayout* vlayout) {

    auto* sendMessageLabel = new QLabel(tr("发送消息"), this);
    sendMessageLabel->setObjectName("sendMessageLabel");
    auto* sendMessageBox = new NoSlidingHandoverComboBox(this);
    sendMessageBox->addItem(QKeySequence(Qt::Key_Enter).toString(QKeySequence::NativeText));
    sendMessageBox->addItem(QKeySequence(Qt::Key_Enter | Qt::ShiftModifier).toString(QKeySequence::NativeText));
    sendMessageBox->addItem(QKeySequence(Qt::Key_Enter | Qt::AltModifier).toString(QKeySequence::NativeText));
    sendMessageBox->addItem(QKeySequence(Qt::Key_Enter | Qt::ControlModifier).toString(QKeySequence::NativeText));
    sendMessageBox->addItem(QKeySequence(Qt::Key_S | Qt::AltModifier).toString(QKeySequence::NativeText));

    auto* sendMessageLay = new QHBoxLayout;
    sendMessageLay->addWidget(sendMessageLabel);
    sendMessageLay->addWidget(sendMessageBox);
    sendMessageLabel->setFixedWidth(100);
    //
    auto *showMood = new SettingCheckBox(tr("群会话显示签名"), AppSetting::instance().getShowMoodFlag(), this);
//    auto *autoDeleteSession = new SettingCheckBox(tr("切换会话时自动销毁旧会话"), AppSetting::instance().getAutoDeleteSession(), this);
    //
    vlayout->addLayout(sendMessageLay);
    vlayout->addWidget(showMood);
//    vlayout->addWidget(autoDeleteSession);
    showMood->setVisible(false);
    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));

    std::string sendMessageKey = AppSetting::instance().getSendMessageKey();
    int index = sendMessageBox->findText(sendMessageKey.data());
    if(index != -1)
    {
        sendMessageBox->setCurrentIndex(index);
    }
    connect(sendMessageBox, &QComboBox::currentTextChanged, [](const QString &key){
        AppSetting::instance().setSendMessageKey(key.toStdString());
    });
    connect(showMood, &QCheckBox::stateChanged,[](int state){
        bool isOk = state==Qt::Checked;
        AppSetting::instance().setShowMoodFlag(isOk);
    });
//
//    connect(autoDeleteSession, &QCheckBox::stateChanged,[](int state){
//        bool isOk = state==Qt::Checked;
//        AppSetting::instance().setAutoDeleteSession(isOk);
//    });
}

/**
 *
 * @return
 */
void SystemSettingWnd::initAutoReply(QVBoxLayout* vlayout) {

    {
        auto *hLay = new QHBoxLayout;
        hLay->setContentsMargins(0, 10, 0, 0);
        hLay->setSpacing(5);
        hLay->setAlignment(Qt::AlignmentFlag::AlignLeft);
        vlayout->addLayout(hLay);
        vlayout->addItem(new QSpacerItem(10, 10));

        auto *leverCK = new SettingCheckBox(tr("启用自动回复"), AppSetting::instance().getAutoReplyEnable(), this);
        hLay->addWidget(leverCK);

        bool enable = AppSetting::instance().getAutoReplyEnable();
        leverCK->setChecked(enable);

        connect(leverCK, &QCheckBox::stateChanged,[](int state){
            AppSetting::instance().setAutoReplyEnable(state == Qt::Checked);
        });
    }
    {
        auto *hLay = new QHBoxLayout;
        vlayout->addLayout(hLay);
        hLay->setAlignment(Qt::AlignLeft);
        auto *descLb = new QLabel(tr("启用自动回复时间段"), this);
        descLb->setObjectName("SettingDescLab");
        hLay->addWidget(descLb);
        descLb = new QLabel(tr("至"), this);
        descLb->setObjectName("SettingDescLab");
        descLb->setFixedWidth(20);
        descLb->setAlignment(Qt::AlignCenter);

        _autoReplyStartTimeSpinBox = new QSpinBox(this);
        _autoReplyEndTimeSpinBox = new QSpinBox(this);

        _autoReplyStartTimeSpinBox->setMinimum(0);
        _autoReplyStartTimeSpinBox->setSuffix(tr(" 时"));
        _autoReplyEndTimeSpinBox->setMinimum(0);
        _autoReplyEndTimeSpinBox->setSuffix(tr(" 时"));
        _autoReplyStartTimeSpinBox->setMaximum(24);
        _autoReplyEndTimeSpinBox->setMaximum(24);
        _autoReplyStartTimeSpinBox->setSingleStep(1);

        hLay->addWidget(_autoReplyStartTimeSpinBox);
        hLay->addWidget(descLb);
        hLay->addWidget(_autoReplyEndTimeSpinBox);
        hLay->setContentsMargins(20, 5, 5, 5);

        vlayout->addItem(new QSpacerItem(10, 10));

        int startTime = AppSetting::instance().getAutoReplyStartTime();
        int endTime = AppSetting::instance().getAutoReplyEndTime();
        _autoReplyStartTimeSpinBox->setValue(startTime);
        _autoReplyEndTimeSpinBox->setValue(endTime);

        connect(_autoReplyStartTimeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onReplyStartTimeChanged(int)));
        connect(_autoReplyEndTimeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onReplyEndTimeChanged(int)));
    }
    {
        auto *lay = new QVBoxLayout;
        auto * awaysAsk = new QRadioButton(tr("始终自动回复"), this);

        auto* hLay = new QHBoxLayout;
        hLay->setAlignment(Qt::AlignLeft);
        lay->addLayout(hLay);
        hLay->setSpacing(1);
        auto * leaveAsk = new QRadioButton(tr("离开电脑超"), this);
        hLay->addWidget(leaveAsk);
        auto *secondEdit = new QSpinBox(this);
        secondEdit->setMinimum(1);
        secondEdit->setMaximum(60 * 24);
        secondEdit->setSuffix(tr(" 分钟"));
        hLay->addWidget(secondEdit);
        lay->addWidget(awaysAsk);
        lay->addWidget(leaveAsk);
        lay->setContentsMargins(20, 5, 5, 5);
        vlayout->addLayout(lay);


        int leave = AppSetting::instance().getLeaveMinute();
        secondEdit->setValue(leave);

        bool awaysAutoReply = AppSetting::instance().getAwaysAutoReply();
        awaysAsk->setChecked(awaysAutoReply);
        leaveAsk->setChecked(!awaysAutoReply);

        auto* group = new QButtonGroup(this);
        group->addButton(leaveAsk, 0);
        group->addButton(awaysAsk, 1);
        connect(group, SIGNAL(buttonClicked(int)), this, SLOT(onAutoReplyClicked(int)));
        connect(secondEdit, SIGNAL(valueChanged(int)), this, SLOT(onLeaveMinuteChanged(int)));
    }
    {
        auto *hLay = new QHBoxLayout;
        hLay->setContentsMargins(20, 5, 5, 5);
        hLay->setAlignment(Qt::AlignmentFlag::AlignLeft);
        vlayout->addLayout(hLay);

        _pAutoReplyEdit = new QTextEdit(this);
        _pAutoReplyEdit->setAcceptRichText(false);
        _pAutoReplyEdit->setFixedSize(356,70);
        _pAutoReplyEdit->setAcceptRichText(false);
        hLay->addWidget(_pAutoReplyEdit);
        //
        std::string autoReplyMessage = AppSetting::instance().getAutoReplyMsg();
        if(autoReplyMessage.empty())
            autoReplyMessage = tr("不在办公室").toStdString();
        _pAutoReplyEdit->setText(autoReplyMessage.data());

        connect(_pAutoReplyEdit, &QTextEdit::textChanged, this, &SystemSettingWnd::setAutoReplyMessage);
        setAutoReplyMessage();
    }

    vlayout->addItem(new QSpacerItem(10, 30, QSizePolicy::Minimum, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));
}

/**
 *
 */
void SystemSettingWnd::onAutoReplyClicked(int id)
{
    AppSetting::instance().setAwaysAutoReply(id);
}

/**
 *
 */
void SystemSettingWnd::setAutoReplyMessage() {

    QString text = _pAutoReplyEdit->toPlainText();
    AppSetting::instance().setAutoReplyMsg(text.toStdString());
//    AppSetting::instance().setAutoReplyCusMsg(text.toStdString());
}

/**
 *
 * @param minute
 */
void SystemSettingWnd::onLeaveMinuteChanged(int minute)
{
    AppSetting::instance().setLeaveMinute(minute);
}

/**
 * 文件夹设置
 * @return
 */
void SystemSettingWnd::initFolder(QVBoxLayout* vlayout) {

     // 用户目录
    auto func([this, vlayout](int type) {

        auto *userDirLay = new QHBoxLayout;
        userDirLay->setSpacing(20);
        userDirLay->setMargin(0);
        vlayout->addLayout(userDirLay);

        auto* tmpLab = new QLabel(this);
        tmpLab->setObjectName("SettingTLabel");
        userDirLay->addWidget(tmpLab);

        auto *fileDirEdit = new QLineEdit(this);
        fileDirEdit->setReadOnly(true);
        fileDirEdit->setObjectName("SettingLineEdit");
        userDirLay->addWidget(fileDirEdit);

        auto *userDirBtnLay = new QHBoxLayout;
        vlayout->addLayout(userDirBtnLay);

        auto *changeSaveFileBtn = new QPushButton(this);
        changeSaveFileBtn->setFixedHeight(28);
        changeSaveFileBtn->setText(tr("变更文件夹"));
        changeSaveFileBtn->setObjectName("SettingBtn");

        auto *openFileDirBtn = new QPushButton(tr("打开文件夹"), this);
        openFileDirBtn->setFixedHeight(28);
        openFileDirBtn->setObjectName("SettingBtn");

        userDirBtnLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
        userDirBtnLay->addWidget(changeSaveFileBtn);
        userDirBtnLay->addWidget(openFileDirBtn);

        QString editText;
        switch (type)
        {
            case EM_USER_FOLDER:
            {
                tmpLab->setText(tr("用户目录"));
                editText = QString::fromStdString(AppSetting::instance().getUserDirectory());
                break;
            }
            case EM_FILE_FOLDER:
            {
                tmpLab->setText(tr("文件目录"));
                editText = QString::fromStdString(AppSetting::instance().getFileSaveDirectory());
                break;
            }
            default:
                break;
        }

        QFontMetrics f(fileDirEdit->font());
        fileDirEdit->setText(f.elidedText(editText, Qt::ElideMiddle, 320));

        connect(changeSaveFileBtn, &QPushButton::clicked, [this, type, fileDirEdit](){

            std::string path;
            switch (type)
            {
                case EM_USER_FOLDER:
                {
                    int ret = QtMessageBox::question(this, tr("重要提示"), "<font color=red>重新设置用户目录会丢失部分设置, 是否继续 ?</font>");
                    if(ret != QtMessageBox::EM_BUTTON_YES)
                    {
                        return;
                    }
                    path = AppSetting::instance().getUserDirectory();
                    break;
                }
                case EM_FILE_FOLDER:
                    path = AppSetting::instance().getFileSaveDirectory();
                    break;
                default:
                    break;
            }
            QString folderPath = QFileDialog::getExistingDirectory(this, tr("请选择一个文件夹"), QString::fromStdString(path));
            if(!folderPath.isEmpty())
            {
                QFontMetrics ff(fileDirEdit->font());
                fileDirEdit->setText(ff.elidedText(folderPath, Qt::ElideMiddle, 320));
                switch (type)
                {
                    case EM_USER_FOLDER:
                    {
                        AppSetting::instance().setUserDirectory(folderPath.toStdString());
                        QtMessageBox::information(this, tr("友情提示"), tr("部分目录重启后生效"));
                        break;
                    }
                    case EM_FILE_FOLDER:
                        AppSetting::instance().setFileSaveDirectory(folderPath.toStdString());
                        break;
                    default:
                        break;
                }
            }
        });

        connect(openFileDirBtn, &QPushButton::clicked, [  type](){

            std::string path;
            switch (type)
            {
                case EM_USER_FOLDER:
                    path = AppSetting::instance().getUserDirectory();
                    break;
                case EM_FILE_FOLDER:
                    path = AppSetting::instance().getFileSaveDirectory();
                    break;
                default:
                    break;
            }
            QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(path)));
        });
    });

    func(EM_USER_FOLDER);
    func(EM_FILE_FOLDER);
    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));

    Line *capLine = new Line(Qt::Horizontal, this);
    vlayout->addWidget(capLine);
}

//void SystemSettingWnd::initFriendAuthority(QVBoxLayout* vlayout) {
//
//    vlayout->addItem(new QSpacerItem(10, 15, QSizePolicy::Expanding));
//
//    { // 权限类型
//
//        auto *friendModeLay = new QHBoxLayout;
//        friendModeLay->setSpacing(0);
//        friendModeLay->setMargin(0);
//        vlayout->addLayout(friendModeLay);
//
//        QLabel *tLabel = new QLabel;
//        tLabel->setFixedWidth(74);
//        tLabel->setText(tr("权限类型"));
//        tLabel->setObjectName("SettingTLabel");
//        friendModeLay->addWidget(tLabel);
//
//        auto *friendModeCB = new NoSlidingHandoverComboBox(this);
//        friendModeCB->addItem(tr("全部接受"));
//        friendModeCB->addItem("500");
//        friendModeCB->addItem("1000");
//        friendModeCB->addItem("2000");
//        friendModeCB->setObjectName("SettingCBox");
//        friendModeLay->addWidget(friendModeCB);
//
//        friendModeLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
//    }
//
//    { // 问题
//
//        auto *questionLay = new QHBoxLayout;
//        questionLay->setSpacing(0);
//        questionLay->setMargin(0);
//        vlayout->addLayout(questionLay);
//
//        auto *titleLbl = new QLabel;
//        titleLbl->setFixedWidth(74);
//        titleLbl->setFixedHeight(13);
//        titleLbl->setText(tr("问题"));
//        titleLbl->setStyleSheet("font-size:13px;font-family:PingFang-SC-Regular;font-weight:400;color:#666666;");
//        questionLay->addWidget(titleLbl);
//
//        auto *questionEdit = new QLineEdit;
//        questionEdit->setFixedHeight(28);
//        questionEdit->setText("C:\\Users\\suuiduofud");
//        questionEdit->setObjectName("SettingLineEdit");
//        questionLay->addWidget(questionEdit);
//    }
//
//    { // 答案
//        auto *anwserLay = new QHBoxLayout;
//        anwserLay->setSpacing(0);
//        anwserLay->setMargin(0);
//        vlayout->addLayout(anwserLay);
//
//        QLabel *titleLbl = new QLabel;
//        titleLbl->setFixedWidth(74);
//        titleLbl->setFixedHeight(13);
//        titleLbl->setText(tr("答案"));
//        titleLbl->setStyleSheet("font-size:13px;font-family:PingFang-SC-Regular;font-weight:400;color:#666666;");
//        anwserLay->addWidget(titleLbl);
//
//        auto *anwserEdit = new QLineEdit;
//        anwserEdit->setFixedHeight(28);
//        anwserEdit->setPlaceholderText(tr("请输入问题"));
//        anwserEdit->setObjectName("SettingLineEdit");
//        anwserLay->addWidget(anwserEdit);
//    }
//
//    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
//
//    Line *capLine = new Line(Qt::Horizontal, this);
//    vlayout->addWidget(capLine);
//}

void SystemSettingWnd::initFontSetting(QVBoxLayout* vlayout) {

    blackTheme = new SettingCheckBox(tr("暗黑模式"), false, this);
    int mode = AppSetting::instance().getThemeMode();
    blackTheme->setChecked(2 == mode);
    vlayout->addWidget(blackTheme);
    //
    auto* scaleEnable = new SettingCheckBox(tr("高分屏适配"), false, this);
    vlayout->addWidget(scaleEnable);
    //
    {
        auto* languageLabel = new QLabel(tr("语言"), this);
        auto* languageCombox = new NoSlidingHandoverComboBox(this);
        languageCombox->addItem("中文");
        languageCombox->addItem("English");
        languageCombox->addItem("한국어");
        //
        int language = AppSetting::instance().getLanguage();
        if(language == QLocale::English)
            languageCombox->setCurrentText("English");
        else if(language == QLocale::Korean)
            languageCombox->setCurrentText("한국어");
        else
            languageCombox->setCurrentText("中文");
        auto* languageLay = new QHBoxLayout();
        languageLay->addWidget(languageLabel);
        languageLay->addWidget(languageCombox);
        vlayout->addLayout(languageLay);
        vlayout->setAlignment(languageCombox, Qt::AlignRight);

        connect(languageCombox, &QComboBox::currentTextChanged, [this, languageCombox](const QString &text){
            Q_UNUSED(text);
            QtMessageBox::information(this, tr("提示"), tr("外观设置重启后生效"));
            if(languageCombox->currentIndex() == 1)
                AppSetting::instance().setLanguage(QLocale::English);
            else if(languageCombox->currentIndex() == 2)
                AppSetting::instance().setLanguage(QLocale::Korean);
            else
                AppSetting::instance().setLanguage(QLocale::Chinese);
        });
    }
    //
    QFontDatabase fontDatabase;
    QStringList families = fontDatabase.families();
    QString defaultFont = QApplication::font().family();
    //
    auto* fontLabel = new QLabel(tr("字体选择"), this);
    auto* fontCombox = new NoSlidingHandoverComboBox(this);
    fontCombox->addItems(families);
//    fontCombox->setItemDelegate(new ComboBoxDelegate);
    //
    std::string sysFont = AppSetting::instance().getFont();
    if(sysFont.empty())
    {
        sysFont = defaultFont.toStdString();
        AppSetting::instance().setFont(sysFont);
    }
    if(families.contains(sysFont.data()))
        fontCombox->setCurrentText(sysFont.data());

    auto* fontLay = new QHBoxLayout();
    fontLay->addWidget(fontLabel);
    fontLay->addWidget(fontCombox);
    vlayout->addLayout(fontLay);
    vlayout->setAlignment(fontCombox, Qt::AlignRight);
    //
    auto* fontLevelLabel = new QLabel(tr("字体大小"), this);
    auto* fontLevelCombox = new NoSlidingHandoverComboBox(this);
    fontLevelCombox->addItem(tr("默认大小"), AppSetting::FONT_LEVEL_NORMAL);
    fontLevelCombox->addItem(tr("大字体"), AppSetting::FONT_LEVEL_BIG);
    fontLevelCombox->addItem(tr("小字体"), AppSetting::FONT_LEVEL_SMALL);
    //
    int fontLevel = AppSetting::instance().getFontLevel();
    fontLevelCombox->setCurrentIndex(fontLevel);
    auto* fontLevelLay = new QHBoxLayout();
    fontLevelLay->addWidget(fontLevelLabel);
    fontLevelLay->addWidget(fontLevelCombox);
    vlayout->addLayout(fontLevelLay);
    vlayout->setAlignment(fontLevelCombox, Qt::AlignRight);
    // 缩放因子
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qunar.com", "");
//    QString scale_factor = settings.value("QT_SCALE_FACTOR").toByteArray();
//    if(!scale_factor.isEmpty())
//    {
//        int scale = scale_factor.toDouble();
//        scaleEdit->setValue(scale);
//    }

    bool scale_enable = settings.value("QT_SCALE_ENABLE").toBool();
    scaleEnable->setChecked(scale_enable);
//    scaleEdit->setEnabled(scale_enable);

    connect(scaleEnable, &QCheckBox::stateChanged, [](int state) {
        bool check = state == Qt::Checked;
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qunar.com", "");
        settings.setValue("QT_SCALE_ENABLE", check);
//        scaleEdit->setEnabled(check);
    });
//    connect(scaleEdit, SIGNAL(valueChanged(int)), this, SLOT(onScaleFactorChanged(int)));
    //
    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));

    connect(blackTheme, &QCheckBox::stateChanged, [this](int state) {
        QtMessageBox::information(this, tr("提示"), tr("外观设置重启后生效"));
        bool check = state == Qt::Checked;
        AppSetting::instance().setThemeMode(check ? 2 : 1);
    });

    connect(fontCombox, &QComboBox::currentTextChanged, [this](const QString& text){
        QtMessageBox::information(this, tr("提示"), tr("外观设置重启后生效"));
        if(!text.isEmpty())
            AppSetting::instance().setFont(text.toStdString());
    });

    connect(fontLevelCombox, SIGNAL(currentIndexChanged(int)), this, SLOT(onFontLevelChanged(int)));
}

void SystemSettingWnd::onScaleFactorChanged(int value) {
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "qunar.com", QApplication::applicationName());
    settings.setValue("QT_SCALE_FACTOR", QString::number(value).toUtf8());
}

#ifdef _WINDOWS
#include <QSettings>
void appAutoRun(bool bAutoRun)
{
    QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);

    if (bAutoRun)
    {
        QString strAppPath=QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        reg.setValue("wirtepad",strAppPath);
    }
    else
    {
        reg.setValue("wirtepad","");
    }

}
#endif

void SystemSettingWnd::initOtherSetting(QVBoxLayout* vlayout) {

    auto *autoLoginCK = new SettingCheckBox(tr("自动登录"), AppSetting::instance().getAutoLoginEnable(), this);
    vlayout->addWidget(autoLoginCK);

#ifdef _WINDOWS
    auto *autoStartUp = new SettingCheckBox(tr("开机自启动"), AppSetting::instance().getSelfStart(), this);
    vlayout->addWidget(autoStartUp);

    connect(autoStartUp, &QCheckBox::stateChanged, [this](int state){
        appAutoRun(state == Qt::Checked);
        AppSetting::instance().setSelfStart(state == Qt::Checked);
    });
#endif

//    auto *openLink = new SettingCheckBox(tr("使用内置浏览器打开对话中的链接"), AppSetting::instance().getOpenLinkWithAppBrowser(), this);
//    vlayout->addWidget(openLink);

    auto *openOaLink = new SettingCheckBox(tr("使用内置浏览器打开OA链接"), AppSetting::instance().getOpenOaLinkWithAppBrowser(), this);
    vlayout->addWidget(openOaLink);

    auto *showSendMessageWnd = new SettingCheckBox(tr("显示发送按钮"), AppSetting::instance().getShowSendMessageBtnFlag(), this);
    vlayout->addWidget(showSendMessageWnd);

    auto *testChannel = new SettingCheckBox(tr("加入内测"), AppSetting::instance().getTestchannel() - 1, this);
    vlayout->addWidget(testChannel);

//    auto *logLevel = new SettingCheckBox(tr("记录INFO日志"), false, this);
//    vlayout->addWidget(logLevel);

//    int level = AppSetting::instance().getLogLevel();
//    logLevel->setChecked(1 == level);

    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal, this));

    connect(autoLoginCK, &QCheckBox::stateChanged, [this](int state){
        emit sgSetAutoLogin(state == Qt::Checked);
    });
//    connect(openLink, &QCheckBox::stateChanged, [](int state){
//        AppSetting::instance().setOpenLinkWithAppBrowser(state == Qt::Checked);
//    });
    connect(openOaLink, &QCheckBox::stateChanged, [](int state){
        AppSetting::instance().setOpenOaLinkWithAppBrowser(state == Qt::Checked);
    });
    connect(showSendMessageWnd, &QCheckBox::stateChanged, [](int state){
        AppSetting::instance().setShowSendMessageBtnFlag(state == Qt::Checked);
    });

    connect(testChannel, &QCheckBox::stateChanged, [](int state){
        int testChannel = state == Qt::Checked;
        testChannel += 1;
        AppSetting::instance().setTestchannel(testChannel);
    });

//    connect(logLevel, &QCheckBox::stateChanged, [](int state){
//        int level = 0;
//        if(state == Qt::Checked)
//            level = 2;
//        else
//            level = 3;
//        QTalk::logger::setLevel(level);
//        AppSetting::instance().setLogLevel(level);
//    });

}

void deleteDictionary(const QString& dirPath)
{
    QFileInfo info(dirPath);
    if(info.isDir())
    {
        QDir baseDir(dirPath);
        auto infoList = baseDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for(const auto& tmpInfo : infoList)
        {
            if(tmpInfo.isDir())
            {
                deleteDictionary(tmpInfo.absoluteFilePath());
                baseDir.rmpath(tmpInfo.absoluteFilePath());
            }
            else {
                QFile::remove(tmpInfo.absoluteFilePath());
            }
        }
    }
    else
    {
        QFile::remove(dirPath);
    }
}

void deleteHistoryLogs()
{
    QString logBasePath = QString("%1/logs").arg(PLAT.getAppdataRoamingPath().data());

    QDir baseDir(logBasePath);
    auto infoList = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(const auto& info : infoList)
    {
        QString dirName = info.baseName();
        auto dirDate = QDate::fromString(dirName, "yyyy-MM-dd");
        auto now = QDate::currentDate();
        if(dirDate.addDays(5) <= now)
        {
            deleteDictionary(info.absoluteFilePath());
            baseDir.rmpath(info.absoluteFilePath());
        }
    }
}

void SystemSettingWnd::initVersionInfo(QVBoxLayout* vlayout) {

    QLabel *versionLbl = new QLabel(tr("当前版本:"), this);
    versionLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    vlayout->addWidget(versionLbl);

    { // 检查更新
        auto *versionLay = new QHBoxLayout;
        versionLay->setSpacing(10);
        versionLay->setContentsMargins(65,0,0,0);
        vlayout->addLayout(versionLay);

        auto *versionLab = new QLabel(this);
        versionLay->addWidget(versionLab);

        std::string clientVersion = PLAT.getGlobalVersion();
        QString currentVersion = QString("%1 ( %2 )").arg(clientVersion.data()).arg(PLAT.getClientNumVerison());
        versionLab->setText(currentVersion);

        auto *build_time_label = new QLabel(this);
        build_time_label->setContentsMargins(65,0,0,0);
        vlayout->addWidget(build_time_label);

        std::string build_time = PLAT.get_build_date_time();
        build_time_label->setText(QString("build at: %1").arg(build_time.data()));

    }

    auto *repairLbl = new QLabel(tr("便捷维护:"), this);
    vlayout->addWidget(repairLbl);

    { // 便捷维护
        auto *repairLay = new QHBoxLayout;
        repairLay->setAlignment(Qt::AlignmentFlag::AlignLeft);
        repairLay->setSpacing(30);
        repairLay->setContentsMargins(65,0,0,0);
        vlayout->addLayout(repairLay);

        auto *repairBtn = new QPushButton(this);
        repairBtn->setFixedHeight(28);
        repairBtn->setText(tr("清除缓存"));
        repairBtn->setObjectName("SettingBtn");
        repairLay->addWidget(repairBtn);

        auto *clearLogs = new QPushButton(this);
        clearLogs->setFixedHeight(28);
        clearLogs->setText(tr("清除日志"));
        clearLogs->setObjectName("SettingBtn");
        repairLay->addWidget(clearLogs);

        connect(repairBtn, &QPushButton::clicked, [this](){
            emit sgClearSystemCache();
        });

        connect(clearLogs, &QPushButton::clicked, [this](){
            deleteHistoryLogs();
            QtMessageBox::information(this, tr("提醒"), tr("日志清理完毕"));
//            emit sgClearSystemCache();
        });
    }

    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    vlayout->addWidget(new Line(Qt::Horizontal,this));
}

/**
 * 意见反馈
 * @return
 */
void SystemSettingWnd::initFeedback(QVBoxLayout* vlayout) {

    QFrame *feedbackInputFrm = new QFrame(this);
    vlayout->addWidget(feedbackInputFrm);

    auto *vInputLay = new QVBoxLayout(feedbackInputFrm);
    vInputLay->setMargin(0);

    auto *inputEdit = new QTextEdit(this);
    inputEdit->setAcceptRichText(false);
    inputEdit->setPlaceholderText(tr("请输入反馈描述"));
    inputEdit->setObjectName("FeedbackEdit");
    inputEdit->setFixedHeight(100);
    vInputLay->addWidget(inputEdit);

     // 提交按钮
    auto *hLayout = new QHBoxLayout;
    hLayout->setAlignment(Qt::AlignmentFlag::AlignRight);
    vInputLay->addLayout(hLayout);
    auto *submitBtn = new QPushButton(tr("提交"), this);
    submitBtn->setObjectName("SubmitBtn");
    submitBtn->setFixedSize(72,32);
    hLayout->addWidget(submitBtn);

    connect(submitBtn, &QPushButton::clicked, [this, inputEdit](){
        QString strText = inputEdit->toPlainText();
        emit sgFeedbackLog(strText);
        inputEdit->clear();
        this->setVisible(false);
    });


    vlayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
}

void SystemSettingWnd::operatingModeButtonsToggled(int id, bool status)
{
    if(status){
        int seat = id % 10;
        int sid = id / 10;
        TitlebarMsgManager::setServiceSeat(sid,seat);
    }
}

//
void SystemSettingWnd::onFontLevelChanged(int level)
{
    QtMessageBox::information(this, tr("提示"), tr("外观设置重启后生效"));
    AppSetting::instance().setFontLevel(level);
}

void SystemSettingWnd::onReplyStartTimeChanged(int time) {
    int endTime = _autoReplyEndTimeSpinBox->value();
    if(time >= endTime){
        _autoReplyStartTimeSpinBox->setValue(time - 1);
    }
    else {
        AppSetting::instance().setAutoReplyStartTime(time);
    }
}

void SystemSettingWnd::onReplyEndTimeChanged(int time) {
    int startTime = _autoReplyStartTimeSpinBox->value();
    if(startTime >= time){
        _autoReplyEndTimeSpinBox->setValue(time + 1);
    }
    else {
        AppSetting::instance().setAutoReplyEndTime(time);
    }
}