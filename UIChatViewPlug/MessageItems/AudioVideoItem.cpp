﻿//
// Created by cc on 2019-02-28.
//

#include "AudioVideoItem.h"

#include "ChatViewMainPanel.h"
#include "../CustomUi/HeadPhotoLab.h"

#include <QSpacerItem>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QUrl>
#include "../../Platform/Platform.h"
#include "../../WebService/AudioVideo.h"
#include "../../QtUtil/Entity/JID.h"
#include "../CustomUi/QtMessageBox.h"
#include "../../UICom/qimage/qimage.h"

extern ChatViewMainPanel *g_pMainPanel;
AudioVideoItem::AudioVideoItem(const StNetMessageResult &msgInfo, QWidget *parent) :
        MessageItemBase(msgInfo, parent),
        _pIconLabel(nullptr),
        _contentLab(Q_NULLPTR){
    init();
}

void AudioVideoItem::init() {
    initLayout();
}

void AudioVideoItem::initLayout() {

    _contentSize = QSize(370, 160);
    _mainMargin = QMargins(15, 0, 20, 0);
    _mainSpacing = 10;
    if (QTalk::Entity::MessageDirectionSent == _msgInfo.direction) {
        _headPixSize = QSize(0, 0);
        _nameLabHeight = 0;
        _leftMargin = QMargins(0, 0, 0, 0);
        _rightMargin = QMargins(0, 0, 0, 0);
        _leftSpacing = 0;
        _rightSpacing = 0;
        initSendLayout();
    } else if (QTalk::Entity::MessageDirectionReceive == _msgInfo.direction) {
        _headPixSize = QSize(28, 28);
        _nameLabHeight = 16;
        _leftMargin = QMargins(0, 10, 0, 0);
        _rightMargin = QMargins(0, 0, 0, 0);
        _leftSpacing = 0;
        _rightSpacing = 4;
        initReceiveLayout();
    }
    if (QTalk::Enum::ChatType::GroupChat != _msgInfo.type) {
        _nameLabHeight = 0;
    }
    setContentsMargins(0, 5, 0, 5);
}

QSize AudioVideoItem::itemWdtSize() {
    int height = qMax(_mainMargin.top() + _nameLabHeight + _mainSpacing + _contentFrm->height() + _mainMargin.bottom(),
                      _headPixSize.height()); // 头像和文本取大的
    int width = _contentFrm->width();
    if(nullptr != _readStateLabel)
    {
        height += 12;
    }
    return {width, height + 8};
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.19
  */
void AudioVideoItem::initSendLayout() {
    auto *mainLay = new QHBoxLayout(this);
    mainLay->setContentsMargins(_mainMargin);
    mainLay->setSpacing(_mainSpacing);
    mainLay->addWidget(_btnShareCheck);
    auto *horizontalSpacer = new QSpacerItem(40, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLay->addItem(horizontalSpacer);
    auto *rightLay = new QVBoxLayout;
    rightLay->setContentsMargins(_rightMargin);
    mainLay->addLayout(rightLay);
    if (!_contentFrm) {
        _contentFrm = new QFrame(this);
    }
    _contentFrm->setObjectName("messSendContentFrm");
    _contentFrm->setFixedWidth(_contentSize.width());
    //
    auto* tmpLay = new QHBoxLayout;
    tmpLay->setMargin(0);
    tmpLay->setSpacing(5);
    tmpLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    if(nullptr != _sending && nullptr != _resending)
    {
        tmpLay->addWidget(_sending);
        tmpLay->addWidget(_resending);
    }
    tmpLay->addWidget(_contentFrm);
    tmpLay->setAlignment(_contentFrm, Qt::AlignRight);
    rightLay->addLayout(tmpLay);
    if (nullptr != _readStateLabel) {
        auto *rsLay = new QHBoxLayout;
        rsLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
        rsLay->setMargin(0);
        rsLay->addWidget(_readStateLabel);
        rightLay->addLayout(rsLay);
    }
    mainLay->setStretch(0, 1);
    mainLay->setStretch(1, 0);

    initContentLayout();
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.10.19
  */
void AudioVideoItem::initReceiveLayout() {
    auto *mainLay = new QHBoxLayout(this);
    mainLay->setContentsMargins(_mainMargin);
    mainLay->setSpacing(_mainSpacing);
    mainLay->addWidget(_btnShareCheck);
    auto *leftLay = new QVBoxLayout;
    leftLay->setContentsMargins(_leftMargin);
    leftLay->setSpacing(_leftSpacing);
    mainLay->addLayout(leftLay);
    leftLay->addWidget(_headLab);
    auto *vSpacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);
    leftLay->addItem(vSpacer);

    leftLay->setStretch(0, 0);
    leftLay->setStretch(1, 1);

    auto *rightLay = new QVBoxLayout;
    rightLay->setContentsMargins(_rightMargin);
    rightLay->setSpacing(_rightSpacing);
    mainLay->addLayout(rightLay);
    if (QTalk::Enum::ChatType::GroupChat == _msgInfo.type
        && QTalk::Entity::MessageDirectionReceive == _msgInfo.direction ) {
        auto* nameLay = new QHBoxLayout;
        nameLay->setMargin(0);
        nameLay->setSpacing(5);
        nameLay->addWidget(_nameLab);
        nameLay->addWidget(_medalWgt);
        rightLay->addLayout(nameLay);
    }
    if (!_contentFrm) {
        _contentFrm = new QFrame(this);
    }
    _contentFrm->setObjectName("messReceiveContentFrm");
    _contentFrm->setFixedWidth(_contentSize.width());
    rightLay->addWidget(_contentFrm);
    rightLay->setStretch(0, 0);
    rightLay->setStretch(1, 1);

    auto *horizontalSpacer = new QSpacerItem(40, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLay->addItem(horizontalSpacer);
    if (QTalk::Enum::ChatType::GroupChat == _msgInfo.type) {
        mainLay->setStretch(0, 0);
        mainLay->setStretch(1, 0);
        mainLay->setStretch(2, 1);
    } else {
        mainLay->setStretch(0, 0);
        mainLay->setStretch(1, 1);
    }

    initContentLayout();
}

/**
 *this
 */
void AudioVideoItem::initContentLayout() {

    QString content = tr("视频通话");
    bool isCalled = QTalk::Entity::MessageDirectionSent == _msgInfo.direction;
    if(!_msgInfo.extend_info.isEmpty())
    {
        QJsonDocument document = QJsonDocument::fromJson(_msgInfo.extend_info.toUtf8());
        if(document.isNull())
        {

        }
        else {
            auto obj = document.object();
            QString type = obj.value("type").toString();
            QInt64 time = obj.value("time").toInt();
            if(type == "cancel")
                content = isCalled ? tr("已取消") : tr("对方已取消");
            else if(type == "close")
            {
                content = tr("通话时长 ");
                auto t = QDateTime::fromSecsSinceEpoch(time).toString("mm:ss");
                content.append(t);
            }
            else if(type == "deny")
                content = isCalled ? tr("对方已拒绝") : tr("已拒绝");
            else if(type == "timeout")
                content = isCalled ? tr("对方暂时无人接听") : tr("对方已取消");
        }
    }

    if(QTalk::Entity::WebRTC_MsgType_Video == _msgInfo.msg_type)
        content = tr("发送端版本过低，视频无法接通");

    _contentFrm->installEventFilter(this);
    auto * lay = new QHBoxLayout(_contentFrm);
    lay->setMargin(12);
    _pIconLabel = new QLabel(this);
    _contentLab = new QLabel(content, this);
    lay->setSpacing(1);
    lay->addWidget(_pIconLabel, 0);
    lay->addWidget(_contentLab, 1);

    QPixmap icon = QTalk::qimage::loadImage(":/chatview/image1/messageItem/AudioVideo.png", false);
    icon = icon.scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    _pIconLabel->setPixmap(icon);
    _contentLab->adjustSize();
    _contentFrm->setFixedSize(_contentLab->width() + 55, 40);
}

bool AudioVideoItem::eventFilter(QObject* o, QEvent* e)
{
    if(o == _contentFrm)
    {
//        if(e->type() == QEvent::Enter)
//        {
//            setCursor(Qt::PointingHandCursor);
//        }
//        else if(e->type() == QEvent::Leave)
//        {
//            setCursor(Qt::ArrowCursor);
//        }
    }

    return MessageItemBase::eventFilter(o, e);
}

void AudioVideoItem::mousePressEvent(QMouseEvent *event) {

    if(event->button() == Qt::LeftButton && _contentFrm->geometry().contains(event->pos()))
    {
#ifdef _WINDOWS
        #ifdef PLATFORM_WIN32
        QtMessageBox::information(g_pMainPanel, tr("提醒"), tr("暂不支持此功能!"));
        return;
#endif
#endif
        if(g_pMainPanel)
        {
            if(_msgInfo.type == QTalk::Enum::TwoPersonChat)
            {
                std::string peerId = _msgInfo.xmpp_id.toStdString();
                if(peerId.empty())
                    peerId = _msgInfo.real_id.toStdString();
                if(peerId.empty())
                    return;
                QTalk::Entity::JID jid(peerId);
                QTalk::Entity::UID uid(jid.basename());
//                g_pMainPanel->sendStartAudioVideoMessage(uid, QTalk::Entity::WebRTC_MsgType_VideoCall == _msgInfo.Type);
                std::string selfId = PLAT.getSelfUserId();
//                AudioVideo::start2Talk(selfId, jid.username());
                g_pMainPanel->start2Talk_old(jid.basename(),
                        QTalk::Entity::WebRTC_MsgType_VideoCall == _msgInfo.msg_type, true);
            }
            else if(_msgInfo.type == QTalk::Enum::GroupChat)
            {
                std::string groupId = QTalk::Entity::JID(_msgInfo.xmpp_id.toStdString()).basename();
                if(groupId.empty())
                    return;
                auto info = DB_PLAT.getGroupInfo(groupId);
                g_pMainPanel->startGroupTalk(groupId.data(), QString::fromStdString(QTalk::getGroupName(info)));
            }
        }
    }
    QFrame::mousePressEvent(event);
}