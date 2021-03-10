#ifndef _TOOLWGT_H_
#define _TOOLWGT_H_

#include <QFrame>
#include "../entity/im_transfer.h"
#include <QPointer>

class QPushButton;
class InputWgt;
class ChatViewItem;
class QMenu;
class QCheckBox;
class QComboBox;
class EmoticonMainWgt;
class ToolWgt : public QFrame
{
	Q_OBJECT

public:
	ToolWgt(InputWgt* pInputWgt, ChatViewItem* chatItem);
	~ToolWgt() override;

private:
    void initUi();
    void onFileBtnClicked();
    void onBtnScreenshot();
	void onpBtnEmoticon();
	void sendJsonPrud(const QString &products);
	void sendQuickReply(const std::string& text);
	void sessionTransfer(const std::string& newJid,const std::string& reason);

public:
    void switchSession(const int &i);

Q_SIGNALS:
    void showSearchWnd();

private:
    QPointer<InputWgt>     _pInputWgt{};
    QPointer<ChatViewItem> _pChatItem{};
	QPushButton* _pBtnEmoticon{};
	QPushButton* _pBtnScreenshot{};
	QPushButton* _pBtnScreenshotSet{};
    QPushButton* _pBtnCode{};
	QPushButton* _pBtnShock{};
	QPushButton* _pBtnFile{};
	QPushButton* _pBtnVideo{};
	QPushButton* _pBtnHistory{};
//	QPushButton* _pBtnShare{};
	QPushButton* _pBtnMultifunction{};
	QMenu*       _pMenu{};
	QAction*     _pScreenShotHideWnd{};

	QMenu* _pMultiMenu{};
};

#endif//_TOOLWGT_H_
