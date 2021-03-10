//
// Created by cc on 18-11-9.
//

#include "TipMessageItem.h"
#include <QLabel>
#include <QHBoxLayout>
#include "../../WebService/WebService.h"
#include "../../Platform/Platform.h"

TipMessageItem::TipMessageItem(QWidget *parent)
    //:MessageItemBase(msgInfo, parent)
{
    _pLabel = new QLabel(this);
    _pLabel->setObjectName("TipLabel");

    auto* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    layout->addWidget(_pLabel);
    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

    //绑定超链接与信号槽
    connect(_pLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
	_pLabel->setAlignment(Qt::AlignCenter);
}

TipMessageItem::~TipMessageItem()
{

}

void TipMessageItem::openURL(QString url) {
    MapCookie cookies;
    cookies["ckey"] = QString::fromStdString(PLAT.getClientAuthKey());
    WebService::loadUrl(QUrl(url), false, cookies);
}

void TipMessageItem::setText(const QString &text)
{
    _strText = text;
    _pLabel->setText(text);
}

QString TipMessageItem::getText()
{
    return _strText;
}

void TipMessageItem::resizeEvent(QResizeEvent *e)
{
    _pLabel->hide();
    _pLabel->show();
    QWidget::resizeEvent(e);
}
