﻿//
// Created by admin on 2019-01-22.
//

#include "CommonTrdInfoItem.h"

#include "ChatViewMainPanel.h"
#include "../CustomUi/HeadPhotoLab.h"
#include "../../WebService/WebService.h"

#include <QSpacerItem>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QDesktopServices>
#include <QMouseEvent>
#include <QUrl>
#include "../../Platform/Platform.h"
#include "../../UICom/qimage/qimage.h"
#include "../../Platform/NavigationManager.h"
#include <QTextEdit>

extern ChatViewMainPanel *g_pMainPanel;

void load(const StNetMessageResult& msgInfo)
{
    static qint64 t = 0;
    qint64 cur_t = QDateTime::currentMSecsSinceEpoch();
    if(cur_t - t < 500)
        return;
    else
        t = cur_t;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(msgInfo.extend_info.toUtf8());
    if (!jsonDocument.isNull()) {
        QJsonObject jsonObject = jsonDocument.object();
        QString linkUrl = jsonObject.value("linkurl").toString();

        //        bool userDftBrowser = AppSetting::instance().getOpenLinkWithAppBrowser();
        MapCookie cookies;
        cookies["ckey"] = QString::fromStdString(PLAT.getClientAuthKey());
        if (/**userDftBrowser ||**/
            linkUrl.contains(NavigationManager::instance().getShareUrl().data()))
        {
            if(linkUrl.contains("?"))
                linkUrl += "&";
            else
                linkUrl += "?";
            //
            linkUrl = QString("%5username=%1&company=%2&group_id=%3&rk=%4").arg(PLAT.getSelfUserId().data())
                    .arg("qunar").arg(msgInfo.xmpp_id).arg(PLAT.getServerAuthKey().data()).arg(linkUrl);
            WebService::loadUrl(QUrl(linkUrl), false, cookies);
        }
        else
            QDesktopServices::openUrl(QUrl(linkUrl));
    }
}


CommonTrdInfoItem::CommonTrdInfoItem(const StNetMessageResult &msgInfo, QWidget *parent) :
        MessageItemBase(msgInfo, parent),
        _iconLab(Q_NULLPTR),
        _titleLab(Q_NULLPTR),
        _contentLab(Q_NULLPTR),
        _sourceLab(Q_NULLPTR) {
    init();
}

void CommonTrdInfoItem::init() {
    initLayout();
}

void CommonTrdInfoItem::initLayout() {

    _contentSize = QSize(350, 0);
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

QSize CommonTrdInfoItem::itemWdtSize() {
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
void CommonTrdInfoItem::initSendLayout() {
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
void CommonTrdInfoItem::initReceiveLayout() {
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

void CommonTrdInfoItem::initContentLayout() {

    QJsonDocument jsonDocument = QJsonDocument::fromJson(_msgInfo.extend_info.toUtf8());
    if (jsonDocument.isNull()) {

        auto *vBox = new QVBoxLayout;
        vBox->setSpacing(5);
        _contentFrm->setLayout(vBox);

        _titleLab = new QLabel(this);
        _titleLab->setObjectName("titleLab");
        _titleLab->setWordWrap(true);
        _titleLab->setFixedWidth(_contentSize.width() - 20);
        _titleLab->setStyleSheet("QLabel{font-size:14px;color:#212121;}");
        _titleLab->setText(_msgInfo.body);
        _titleLab->adjustSize();
        vBox->addWidget(_titleLab);

        _contentFrm->setFixedSize(10 + 10 + _titleLab->width(), 10 + 10 + _titleLab->height());

    } else {
        QJsonObject jsonObject = jsonDocument.object();
        bool allContent = jsonObject.value("showas667").toBool(false);
		allContent |= (_msgInfo.msg_type == QTalk::Entity::MessageTypeCommonTrdInfoV2);
        QString title = jsonObject.value("title").toString();
        QString desc = jsonObject.value("desc").toString();
        if (desc.isEmpty()) {
            desc = tr("查看详情");
        }
        if (!_titleLab) {

            auto *cBox = new QVBoxLayout(_contentFrm);
            cBox->setMargin(10);

            QFrame *contentFrm = new QFrame(this);
            contentFrm->setObjectName("CommonTrdInfoItemMainFrm");
            cBox->addWidget(contentFrm);

            auto *hBox = new QHBoxLayout(contentFrm);
            hBox->setMargin(10);
            hBox->setSpacing(18);

            _iconLab = new QLabel(this);
            _iconLab->setFixedSize(40, 40);
            _iconLab->setAlignment(Qt::AlignmentFlag::AlignTop);

            auto *vBox = new QVBoxLayout;
            vBox->setSpacing(8);
            hBox->addWidget(_iconLab);
            hBox->addLayout(vBox);
            hBox->setStretch(0, 0);
            hBox->setStretch(1, 1);
            hBox->setAlignment(_iconLab, Qt::AlignTop | Qt::AlignHCenter);

            _titleLab = new QLabel(this);
            _titleLab->setObjectName("CommonTrdInfoItemTitleLabel");
            _titleLab->setFixedHeight(18);
            _titleLab->setFixedSize(_contentSize.width()-10-12-40-18-12-10,18);
            vBox->addWidget(_titleLab);

            _contentLab = new CommonTrdInfoEdit(this);
            _contentLab->setFrameShape(QFrame::NoFrame);
            _contentLab->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            _contentLab->setReadOnly(true);
            _contentLab->installEventFilter(this);
            _contentLab->setFocusPolicy(Qt::NoFocus);
            _contentLab->setContextMenuPolicy (Qt::NoContextMenu);
            connect(_contentLab, &QTextEdit::cursorPositionChanged, [this](){
                auto pos = _contentLab->textCursor().position();
                if(pos != 0) load(_msgInfo);
            });
            _contentLab->setObjectName("contentLab");
            if (allContent) {
                _contentLab->setFixedWidth(_titleLab->width());
            } else {
                _contentLab->setFixedSize(_titleLab->width(), 20);
            }
            vBox->addWidget(_contentLab);

            vBox->setStretch(0, 0);
            vBox->setStretch(1, 1);
        }

        QFontMetrics titleFT(_titleLab->font());
        int tWidth = _titleLab->width();
        title = titleFT.elidedText(title, Qt::ElideRight, tWidth);
        _titleLab->setText(title);

        QFontMetrics contentFT(_contentLab->font());
        int cWidth = _contentLab->geometry().width();
        _contentLab->setText(desc);
        if (!allContent) {
            desc = contentFT.elidedText(desc, Qt::ElideRight, cWidth);
        } else {
            _contentLab->document()->adjustSize();
            QSizeF contentSize = _contentLab->document()->size();
            _contentLab->setFixedHeight((int)contentSize.height());
        }
        _contentLab->setText(desc);

        connect(this, &CommonTrdInfoItem::sgDownloadedIcon, this, &CommonTrdInfoItem::setIcon, Qt::QueuedConnection);
        QString imgUrl = jsonObject.value("img").toString();
        QString placeHolder = ":/chatview/image1/defaultShareIcon.png";
        QPixmap defaultPix = QTalk::qimage::loadImage(placeHolder, false, true, 40, 40);
        if (imgUrl.isEmpty()) {
            _iconLab->setPixmap(defaultPix);
        } else {
            std::string imgPath = QTalk::GetImagePathByUrl(imgUrl.toStdString());
            if(QFile::exists(imgPath.data()))
            {
                _iconLab->setPixmap(QTalk::qimage::loadImage(imgPath.data(), false, true, 40, 40));
            }
            else
            {
                _iconLab->setPixmap(defaultPix);
                QPointer<CommonTrdInfoItem> pThis(this);
                QT_CONCURRENT_FUNC([pThis, imgUrl](){
                    std::string downloadFile = ChatMsgManager::getLocalFilePath(imgUrl.toStdString());
                    if(pThis && !downloadFile.empty())
                        emit pThis->sgDownloadedIcon(QString::fromStdString(downloadFile));
                });
            }
        }
        _contentLab->adjustSize();
        this->adjustSize();
        auto height = 10 + 12 + _titleLab->height() + 8 + _contentLab->height() + 12 + 10;

        _contentFrm->setFixedHeight(
                10 + 12 + _titleLab->height() + 8 + _contentLab->height() + 12 + 10);
    }
}

void CommonTrdInfoItem::setIcon(const QString &iconPath) {
    QPixmap pixmap = QTalk::qimage::loadImage(iconPath, false, true, 40, 40);
    _iconLab->setPixmap(pixmap);
}

void CommonTrdInfoItem::mousePressEvent(QMouseEvent *event) {

    if(event->button() == Qt::LeftButton && _contentFrm->geometry().contains(event->pos()))
        load(_msgInfo);
    QFrame::mousePressEvent(event);
}