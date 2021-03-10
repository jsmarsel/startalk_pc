﻿//
// Created by cc on 19-1-9.
//

#include "NewMessageTip.h"
#include <QHBoxLayout>
#include <QListWidget>
#include "../CustomUi/HeadPhotoLab.h"

NewMessageTip::NewMessageTip(QWidget *parent)
    : QFrame(parent), _parentWgt(parent)
{
    this->setFixedHeight(40);
    this->setMinimumWidth(120);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setObjectName("NewMessageTip");
    //
    auto* picLabel = new HeadPhotoLab;
    picLabel->setParent(this);
    picLabel->setHead(":/chatview/image1/go.png", 10, false, false, true);
    picLabel->setFixedWidth(20);
    _pLabel = new QLabel(QString(tr("%1 条新消息")).arg(_newMessageCount), this);
    _pLabel->setObjectName("NewMessageTipLabel");
    auto* lay = new QHBoxLayout(this);
    lay->addWidget(picLabel);
    lay->addWidget(_pLabel);
}

/**
 *
 */
void NewMessageTip::onNewMessage() {
    _newMessageCount++;
    if(!this->isVisible())
    {
       this->setVisible(true);
        int w = (_parentWgt->width() - this->width()) / 2;
        this->move(w, _parentWgt->height() - 50);
    }
    //
    _pLabel->setText(QString(tr("%1 条新消息")).arg(_newMessageCount));
}

/*
 *
 */
void NewMessageTip::mousePressEvent(QMouseEvent *e) {
    onResetWnd();
    QWidget::mousePressEvent(e);
}

/**
 *
 */
void NewMessageTip::onResetWnd() {
    this->setVisible(false);
    _newMessageCount = 0;
    _pLabel->setText(QString(tr("%1 条新消息")).arg(_newMessageCount));
    auto* parentWgt = qobject_cast<QListView*>(_parentWgt);
    if(parentWgt)
        parentWgt->scrollToBottom();
}

void NewMessageTip::onResize()
{
	if (this->isVisible())
	{
		int w = (_parentWgt->width() - this->width()) / 2;
		this->move(w, _parentWgt->height() - 50);
	}
}
