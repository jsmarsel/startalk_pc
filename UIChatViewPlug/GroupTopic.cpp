#include "GroupTopic.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>
#include "QToolButton"
#include "GroupTopicEditor.h"


GroupTopic::GroupTopic(QString &groupId,
                       QString &groupName,
                       QString &groupTopic,
                       QWidget *parent)
        : QFrame(parent), _groupId(groupId),
          _groupName(groupName), _groupTopic(groupTopic) {
    setObjectName("GroupTopic");
    setFrameShape(QFrame::NoFrame);
    initUi();
//	setTopic("");
}


GroupTopic::~GroupTopic() {
    if (nullptr != _pBtnShowMore) {
        delete _pBtnShowMore;
        _pBtnShowMore = nullptr;
    }
}

/**
  * @函数名   setTopic
  * @功能描述 设置群公告
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void GroupTopic::setTopic(const QString &topic) {
    _groupTopic = topic;
    if (topic.isEmpty()) {
        _pStackLay->setCurrentWidget(_pEmptyLabel);
    } else {
        _pStackLay->setCurrentWidget(_pTopicEdit);
        _pTopicEdit->setText(topic);
    }
}

/**
  * @函数名   initUi
  * @功能描述 初始化ui
  * @参数
  * @author   cc
  * @date     2018/10/09
  */
void GroupTopic::initUi() {
    auto *pLabelTitle = new QLabel(tr("群公告"), this);

    auto *editBtn = new QToolButton(this);
    editBtn->setObjectName("GroupTopicEditBtn");
    editBtn->setFixedSize(16, 16);

    _pStackLay = new QStackedLayout();
    _pStackLay->setMargin(2);
    _pEmptyLabel = new QLabel(tr("暂无公告"), this);
    _pEmptyLabel->setAlignment(Qt::AlignCenter);
    _pStackLay->addWidget(_pEmptyLabel);
    _pTopicEdit = new QTextEdit(this); //
    _pTopicEdit->viewport()->installEventFilter(this);
    _pStackLay->addWidget(_pTopicEdit);
    _pTopicEdit->installEventFilter(this);
    _pTopicEdit->setAcceptRichText(false);
    _pTopicEdit->verticalScrollBar()->setVisible(false);
    _pBtnShowMore = new QPushButton;

    _pTopicEdit->setReadOnly(true);
    _pTopicEdit->setFrameShape(QFrame::NoFrame);

    pLabelTitle->setObjectName("GroupTopicTitle");
    _pTopicEdit->setObjectName("TopicEdit");
    _pEmptyLabel->setObjectName("TopicEmptyLabel");
    _pBtnShowMore->setObjectName("BtnShowMore");

    _pStackLay->setCurrentWidget(_pEmptyLabel);
//	pLabelTitle->setContentsMargins(13, 15, 0, 0);
//	_pTopicEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	_pEmptyLabel->installEventFilter(this);

    auto *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(13, 10, 15, 0);
    topLayout->addWidget(pLabelTitle);
    topLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));
    topLayout->addWidget(editBtn);
    topLayout->setAlignment(pLabelTitle, Qt::AlignVCenter);
    topLayout->setAlignment(editBtn, Qt::AlignVCenter);
    //topLayout->addWidget(_pBtnShowMore);

    auto *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addLayout(topLayout);
    layout->addLayout(_pStackLay);

    connect(editBtn, &QToolButton::clicked, this, &GroupTopic::onEditBtnClicked);
//	connect(_pTopicEdit, &QTextEdit::cursorPositionChanged, this, &GroupTopic::onEditBtnClicked);
}

/**
 *
 * @param o
 * @param e
 * @return
 */
bool GroupTopic::eventFilter(QObject *o, QEvent *e) {
    if (o == _pTopicEdit) {
        if (e->type() == QEvent::Enter) {
            _pTopicEdit->verticalScrollBar()->setVisible(true);
        } else if (e->type() == QEvent::Leave) {
            _pTopicEdit->verticalScrollBar()->setVisible(false);
        }
    } else if( o == _pTopicEdit->viewport() || o == _pEmptyLabel) {
        if (e->type() == QEvent::MouseButtonPress) {
            onEditBtnClicked();
        } else if (e->type() == QEvent::Enter) {
		    setCursor(Qt::PointingHandCursor);
	    } else if (e->type() == QEvent::Leave) {
		    setCursor(Qt::ArrowCursor);
	    }
    }

    return QFrame::eventFilter(o, e);
}

void GroupTopic::onEditBtnClicked() {
    GroupTopicEditor editor(_groupId, _groupName, _groupTopic, this);
    editor.exec();
}