﻿//
// Created by cc on 19-1-9.
//
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
#ifndef QTALK_V2_NEWMESSAGETIP_H
#define QTALK_V2_NEWMESSAGETIP_H

#include <QFrame>
#include <QLabel>

class NewMessageTip : public QFrame
{
	Q_OBJECT
public:
    explicit NewMessageTip(QWidget* parent);

public:
    void onNewMessage();
    void onResetWnd();
	void onResize();

protected:
    void mousePressEvent(QMouseEvent* e) override;

private:
    int _newMessageCount{0};

private:
    QLabel* _pLabel;

private:
    QWidget* _parentWgt{};
};


#endif //QTALK_V2_NEWMESSAGETIP_H
