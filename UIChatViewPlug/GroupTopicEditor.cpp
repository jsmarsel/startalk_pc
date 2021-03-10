//
// Created by cc on 2020/12/9.
//

#include "GroupTopicEditor.h"
#include "../CustomUi/TitleBar.h"
#include <QVBoxLayout>
#include <QtConcurrent>
#include "../include/Line.h"
#include "../CustomUi/QtMessageBox.h"
#include "MessageManager.h"

//
GroupTopicEditor::GroupTopicEditor(const QString& groupId,
                                   const QString& groupName,
                                   const QString& groupTopic,
                                   QWidget *parent)
    : UShadowDialog(parent, true)
{
    _groupId = groupId;
    _groupTopic = groupTopic;

    auto *title = new TitleBar(tr("群公告 - %1").arg(groupName), this, this);
    title->setFixedHeight(40);
    this->setMoverAble(true, title);
    //
    _pEditBtn = new QToolButton(this);
    _pClearBtn = new QToolButton(this);
    _pOkBtn = new QToolButton(this);
    _pCancelBtn = new QToolButton(this);
    _pEditBtn->setFixedSize(20, 20);
    _pClearBtn->setFixedSize(20, 20);
    _pOkBtn->setFixedSize(20, 20);
    _pCancelBtn->setFixedSize(20, 20);

    _pEditBtn->setObjectName("GroupTopicEdit");
    _pClearBtn->setObjectName("GroupTopicClear");
    _pOkBtn->setObjectName("GroupTopicOk");
    _pCancelBtn->setObjectName("GroupTopicCancel");

    _pEditBtn->setToolTip(tr("编辑"));
    _pClearBtn->setToolTip(tr("删除"));
    _pOkBtn->setToolTip(tr("确定"));
    _pCancelBtn->setToolTip(tr("取消"));

    auto *btnFrm = new QFrame(this);
    btnFrm->setObjectName("GroupTopicBtnFrm");
    btnFrm->setFixedHeight(40);
    auto* btnLay = new QHBoxLayout(btnFrm);
    btnLay->setContentsMargins(0, 0, 20, 0);
    btnLay->setSpacing(10);
    btnLay->setAlignment(Qt::AlignVCenter);
    btnLay->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    btnLay->addWidget(_pEditBtn);
    btnLay->addWidget(_pOkBtn);
    btnLay->addWidget(_pCancelBtn);
    btnLay->addWidget(_pClearBtn);

    //
    _pEdit = new QTextEdit(this);
    _pEdit->setPlaceholderText(tr("请输入群公告"));
    _pEdit->setAcceptRichText(false);
    _pEdit->setText(groupTopic);
    _pEdit->document()->setDocumentMargin(15);
    //
    this->setFixedSize(520, 400);

    auto* mainFrm = new QFrame(this);
    mainFrm->setObjectName("GroupTopicMainFrm");
    auto* mainLay = new QVBoxLayout(mainFrm);
    mainLay->setContentsMargins(0, 0, 0, 10);
    mainLay->setSpacing(0);
    mainLay->addWidget(title);
    mainLay->addWidget(new Line(this));
    mainLay->addWidget(btnFrm);
    mainLay->addWidget(new Line(this));
    mainLay->addWidget(_pEdit);

    auto *lay = new QVBoxLayout(_pCenternWgt);
    lay->setMargin(0);
    lay->addWidget(mainFrm);
    //
    enableUI(false);

    connect(_pEditBtn, &QToolButton::clicked, this, &GroupTopicEditor::onEditBtnClicked);
    connect(_pClearBtn, &QToolButton::clicked, this, &GroupTopicEditor::onClearBtnClicked);
    connect(_pCancelBtn, &QToolButton::clicked, this, &GroupTopicEditor::onCancelBtnClicked);
    connect(_pOkBtn, &QToolButton::clicked, this, &GroupTopicEditor::onOkBtnClicked);
}

//
void GroupTopicEditor::onEditBtnClicked() {
    enableUI(true);
}

void GroupTopicEditor::enableUI(bool editing) {

    _pEdit->setReadOnly(!editing);
    _pEditBtn->setVisible(!editing);
    _pClearBtn->setVisible(!editing);
    _pOkBtn->setVisible(editing);
    _pCancelBtn->setVisible(editing);
}

void GroupTopicEditor::onCancelBtnClicked() {
    enableUI(false);

    _pEdit->setText(_groupTopic);
}

void GroupTopicEditor::onOkBtnClicked() {
    enableUI(false);
    const auto text = _pEdit->toPlainText();
    if(_groupTopic != text) {
        QtConcurrent::run(&ChatMsgManager::updateGroupTopic, _groupId.toStdString(), text.toStdString());
    }
    this->close();
}

void GroupTopicEditor::onClearBtnClicked() {
	int res = QtMessageBox::question(this, tr("提醒"), tr("确定要清空群公告?"));
	if(res != QtMessageBox::EM_BUTTON_YES)
		return;
    QtConcurrent::run(&ChatMsgManager::updateGroupTopic, _groupId.toStdString(), std::string());
    this->close();
}