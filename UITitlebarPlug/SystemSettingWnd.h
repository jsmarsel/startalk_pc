//
// Created by admin on 2019-02-19.
//

#ifndef QTALK_V2_SYSTEMSETTINGWND_H
#define QTALK_V2_SYSTEMSETTINGWND_H

#include "../CustomUi/UShadowWnd.h"
#include <QMap>
#include <QCheckBox>
#include <QComboBox>
#include <QStyledItemDelegate>
#include "../Platform/AppSetting.h"
#include "MessageManager.h"

class QVBoxLayout;
class QListWidget;
class ActionLabel;
class QListWidgetItem;
class SKRecorder;
class QTextEdit;
class QSpinBox;
class SettingCheckBox : public QCheckBox
{
public:
    explicit SettingCheckBox(const QString& text = QString(), bool defaultVal = false, QWidget* parent = nullptr)
        :QCheckBox(text, parent)
    {
        setChecked(defaultVal);
        setObjectName("SettingCheckBox");
    }
    ~SettingCheckBox() override = default;

};

class NoSlidingHandoverComboBox : public QComboBox {
public:
    explicit NoSlidingHandoverComboBox(QWidget* parent = nullptr)
            :QComboBox(parent) {
        setObjectName("NoSlidingHandoverComboBox");
    }

protected:
    void wheelEvent(QWheelEvent* e) override
    {

    }

    void showPopup() override  {
        QComboBox::showPopup();
        QWidget *popup = this->findChild<QFrame *>();
        if(popup) {
            popup->setMaximumHeight(300);
            popup->move(parentWidget()->mapToGlobal(this->geometry().bottomLeft()));
        }
    }
};

class ComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class SystemSettingWnd  : public UShadowDialog{
    Q_OBJECT
public:
    explicit SystemSettingWnd(QWidget * parent = nullptr);
    ~SystemSettingWnd() override;

Q_SIGNALS:
    void sgSetAutoLogin(bool);
    void sgFeedbackLog(const QString&);
    void sgUpdateHotKey();
	void saveConfig();
    void sgClearSystemCache();
    void msgSoundChanged();

protected:
    bool eventFilter(QObject* o, QEvent* e) override;
	void hideEvent(QHideEvent* e) override;

private slots:
    void operatingModeButtonsToggled(int, bool);

private:
    void initUi();
    void initSetting(int type);

private:
    void initMsgNotify(QVBoxLayout* itemLay);
    void initHotCut(QVBoxLayout* itemLay);
    void initSession(QVBoxLayout* itemLay);
    void initAutoReply(QVBoxLayout* itemLay);
    void initFolder(QVBoxLayout* itemLay);
    void initFontSetting(QVBoxLayout* itemLay);
    void initOtherSetting(QVBoxLayout* itemLay);
    void initVersionInfo(QVBoxLayout* itemLay);
    void initFeedback(QVBoxLayout* itemLay);

private:
    SKRecorder* addSortCutItem(const QString& text, const QKeySequence& keys, QVBoxLayout* layout,std::function<void(const QKeySequence& keys)> callback);

private slots:
    void setAutoReplyMessage();
    void onAutoReplyClicked(int);
    void onLeaveMinuteChanged(int);
    void onScaleFactorChanged(int);
    void onFontLevelChanged(int);
    void onReplyStartTimeChanged(int);
    void onReplyEndTimeChanged(int);

private:
    QListWidget *_settingListWidget{};
    QVBoxLayout * _pNavlayout{};
    QMap<QListWidgetItem*, ActionLabel*> _mapSettingItems{};
    QMap<QFrame*, ActionLabel*> _mapSettingWgt{};

    QSpinBox *_autoReplyStartTimeSpinBox{};
    QSpinBox *_autoReplyEndTimeSpinBox{};

private:
    QTextEdit* _pAutoReplyEdit{};
//    QComboBox* _pAutoPeplyCombobox{};

    SettingCheckBox* blackTheme{};

private:

    enum settingType
    {
        EM_SETTING_MESSAGE,
        EM_SETTING_HOTKEY,
        EM_SETTING_AUTOREPLY,
        EM_SETTING_SESSION,
        EM_SETTING_FOLDER,
        EM_SETTING_FONT,
        EM_SETTING_OTHERSETTING,
        EM_SETTING_VERSION,
        EM_SETTING_SUGGESTION,
        // todo 暂时不显示

//        EM_SETTING_FRIEND,
    };

    enum {EM_USER_FOLDER, EM_FILE_FOLDER};
};


#endif //QTALK_V2_SYSTEMSETTINGWND_H
