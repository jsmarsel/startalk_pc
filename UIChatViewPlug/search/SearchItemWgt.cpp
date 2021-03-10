//
// Created by cc on 2019/11/06.
//

#include "SearchItemWgt.h"
#include <QDateTime>
#include <QDebug>
#include <QEvent>
#include <QFileInfo>
#include <blocks/block_define.h>
#include <QLabel>
#include <QtConcurrent>
#include <QPointer>
#include <QApplication>
#include <QMouseEvent>
#include <ChatUtil.h>
#include "../NetImageLabel.h"
#include "../ChatViewMainPanel.h"
#include "../../Platform/Platform.h"
#include "../../UICom/qimage/qimage.h"
#include "../../UICom/StyleDefine.h"
#include "../../WebService/WebService.h"
#include "../../Emoticon/EmoticonMainWgt.h"
#include "../../Platform/NavigationManager.h"
#include "../../CustomUi/QtMessageBox.h"

#define DEM_AT_HTML "<span style=\"color:#FF4E3F;\">%1</span>"

extern ChatViewMainPanel *g_pMainPanel;
/** SearchItemBase **/
SearchItemBase::SearchItemBase(const StNetMessageResult &info, QWidget *parent)
    :QFrame(parent)
{
    QString name = QTalk::getUserName(info.from.toStdString()).data();
    int dir = (PLAT.getSelfXmppId() == info.from.toStdString());
    title = new NameTitleLabel(dir, name, QDateTime::fromMSecsSinceEpoch(info.time).toString("yyyy-MM-dd hh:mm:ss"));

    _pDetailBtn = new QToolButton(this);
    _pDetailBtn->setObjectName("openMessageBtn");
    _pDetailBtn->setFixedSize(20, 20);
    _pDetailBtn->setVisible(false);

    auto* topLay = new QHBoxLayout();
    topLay->setContentsMargins(0, 0, 8, 0);
    topLay->setSpacing(0);
    topLay->addWidget(title);
    topLay->addWidget(_pDetailBtn);

    lay = new QVBoxLayout(this);
    lay->setSpacing(0);
    lay->setContentsMargins(16, 8, 16, 8);
    lay->addLayout(topLay);

    connect(_pDetailBtn, &QToolButton::clicked, [this, info] (bool){
        emit sgGetMessageDetail(info.time, info.xmpp_id, info.type);
    });
}

void SearchItemBase::setDetailButtonVisible(bool visible)
{
    if (_pDetailBtn)
        _pDetailBtn->setVisible(visible);
}

/** SearchTextItem **/
SearchTextItem::SearchTextItem(const StNetMessageResult& info, QWidget *parent)
    :SearchItemBase(info, parent)
{
    _pBrowser = new TextBrowser(this);
    _pBrowser->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    _pBrowser->installEventFilter(this);

    lay->addWidget(_pBrowser);

    connect(_pBrowser, &TextBrowser::sgClicked, this, &SearchTextItem::sgSelectIndex);
    connect(_pBrowser, &TextBrowser::sgImageClicked, g_pMainPanel, &ChatViewMainPanel::sgShowPicture);
    connect(_pBrowser, &TextBrowser::anchorClicked, [](const QUrl &url){
        QString strUrl = url.toString();
//        bool userDftBrowser = AppSetting::instance().getOpenLinkWithAppBrowser();
        if (!strUrl.startsWith("http")) {
            strUrl = (QString("http://%1").arg(strUrl));
        }
//        if (userDftBrowser)
//            WebService::loadUrl(QUrl(strUrl));
//        else
        QDesktopServices::openUrl(QUrl(strUrl));
    });

    QTextCharFormat f;
    f.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    f.setFontWordSpacing(0);
    f.setFontLetterSpacing(0);
    _pBrowser->setCurrentCharFormat(f);
    if(!info.text_messages.empty())
    {
        int imgIndex = 0;
        for (const auto &msg : info.text_messages) {
            switch (msg.type) {
                case StTextMessage::EM_TEXT:
                    _pBrowser->insertPlainText(msg.content);
                    break;
                case StTextMessage::EM_EMOTICON:
                case StTextMessage::EM_IMAGE:
                 {
                    QString imagePath = msg.content;
                    qreal imageWidth = msg.imageWidth;
                    qreal imageHeight = msg.imageHeight;

                    bool noLocal = imagePath.isEmpty() || !QFile::exists(imagePath) || QFileInfo(imagePath).isDir();

                    if (noLocal) {
                        imagePath = ":/chatview/image1/default.png";
                    }

                    if (QPixmap(imagePath).isNull()) {
                        QString realPath = QTalk::qimage::getRealImagePath(imagePath);
                        if (QPixmap(realPath).isNull()) {
                            imagePath = ":/chatview/image1/default.png";
                        } else {
                            imagePath = realPath;
                        }
                    }

                    QTextImageFormat imageFormat;
                    imageFormat.setWidth(imageWidth == 0 ? 80 : imageWidth);
                    imageFormat.setHeight(imageHeight == 0 ? 80 : imageHeight);

                    imageFormat.setProperty(imagePropertyPath, msg.content);
                    imageFormat.setProperty(imagePropertyLink, msg.imageLink);
                    imageFormat.setProperty(imagePropertyIndex, imgIndex++);

                    if (noLocal)
                    {
                        if(StTextMessage::EM_IMAGE == msg.type && !msg.imageLink.isEmpty())
                        {
                            imageFormat.setName(msg.imageLink);
                            downloadImage(msg.imageLink);
                        }
                        else if(StTextMessage::EM_EMOTICON == msg.type && !msg.pkgid.isEmpty() && !msg.shortCut.isEmpty())
                        {
                            imageFormat.setName(msg.pkgid+msg.shortCut);
                            downloadEmoticon(msg.pkgid, msg.shortCut);
                        }
                    }
                    else
                    {
                        imageFormat.setName(imagePath);
                    }

                    _pBrowser->textCursor().insertImage(imageFormat);
                    _pBrowser->setCurrentCharFormat(f);
                    _pBrowser->textCursor().insertText(" ");
                    //
                    QFileInfo imageInfo(imagePath);
                    if(imageInfo.suffix().toUpper() == "GIF" && _mapMovies.find(msg.content) == _mapMovies.end())
                    {
                        _mapMovies[msg.content] = nullptr;
                    }
                    //
                    break;
                }
                case StTextMessage::EM_LINK: {
                    QString content = msg.content;
                    QTextCharFormat linkFormat = _pBrowser->textCursor().charFormat();
                    linkFormat.setForeground(QBrush(QTalk::StyleDefine::instance().getLinkUrl()));
                    linkFormat.setAnchor(true);
                    linkFormat.setAnchorHref(msg.content);
#if QT_DEPRECATED_SINCE(5, 13)
                    linkFormat.setAnchorNames(QStringList() << msg.content);
#else
                    linkFormat.setAnchorName(msg.content);
#endif
                    _pBrowser->textCursor().insertText(msg.content, linkFormat);

                    _pBrowser->setCurrentCharFormat(f);
                    break;
                }
                case StTextMessage::EM_ATMSG: {
                    QString content = QString(DEM_AT_HTML).arg(msg.content);
                    _pBrowser->insertHtml(content);
                    _pBrowser->setCurrentCharFormat(f);
                    break;
                }
                default:
                    break;
            }
        }
    }
    else
    {
        _pBrowser->setText(info.body);
    }
}

QSize SearchTextItem::getContentSize(qreal width) {
    width -= 32;
    auto docSize = _pBrowser->document()->size();
    int h = title->height() + docSize.height();
    return {(int)width, h + 16};
}

bool SearchTextItem::event(QEvent *event) {
    return QFrame::event(event);
}

void SearchTextItem::downloadImage(const QString &imageLink) {

    QPointer<SearchTextItem> pThis(this);
    QtConcurrent::run([pThis, imageLink](){
        auto imagePath = ChatMsgManager::getLocalFilePath(imageLink.toStdString());
        if(pThis)
        {
            auto image = QTalk::qimage::loadImage(imagePath.data(), false);
            pThis->_pBrowser->document()->addResource(QTextDocument::ImageResource,
                                               imageLink, image);
        }
    });
}

void SearchTextItem::downloadEmoticon(const QString& pkgid, const QString& shortCut)
{
    QPointer<SearchTextItem> pThis(this);
    QT_CONCURRENT_FUNC([pThis, pkgid, shortCut](){
        QString localPath = EmoticonMainWgt::instance()->downloadEmoticon(pkgid, shortCut);
        if(pThis)
        {
            auto image = QTalk::qimage::loadImage(localPath, false);
            pThis->_pBrowser->document()->addResource(QTextDocument::ImageResource,
                                                      pkgid+shortCut, image);
        }
    });
}

/*** tip item ***/
SearchTipITem::SearchTipITem(const StNetMessageResult &info, QWidget *parent)
    :SearchItemBase(info, parent)
{
    auto* label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);
    label->setObjectName("TipLabel");
    label->setText(info.body);

    lay->addWidget(label);
}

/*** file item ***/
SearchFileITem::SearchFileITem(const StNetMessageResult &info, QWidget *parent)
    :SearchItemBase(info, parent)
{
    auto* contentFrm = new QFrame(this);
    contentFrm->setFixedWidth(258);
    contentFrm->setObjectName("FileItemWgtLeftFrm");

    auto* iconLabel = new QLabel(this);
    iconLabel->setFixedSize(36, 36);
    QPixmap pixmap = QTalk::qimage::loadImage(info.file_info.fileIcon, false, true, 36);
    iconLabel->setPixmap(pixmap);

    auto* fileNameLabel = new QLabel(info.file_info.fileName, this);
    auto* fileSizeLabel = new QLabel(info.file_info.fileSize, this);
    fileNameLabel->setObjectName("FileItemWgtFileNameLabel");
    fileSizeLabel->setObjectName("FileItemWgtFileSizeLabel");

    auto* infoLay = new QHBoxLayout;
    infoLay->addWidget(fileSizeLabel);
    infoLay->setAlignment(fileSizeLabel, Qt::AlignRight);

    auto* bbLay = new QVBoxLayout;
    bbLay->setMargin(0);
    bbLay->setSpacing(5);
    bbLay->addWidget(fileNameLabel);
    bbLay->addLayout(infoLay);

    auto* leftLay = new QHBoxLayout(contentFrm);
    leftLay->setMargin(10);
    leftLay->addWidget(iconLabel);
    leftLay->addLayout(bbLay);
    leftLay->setAlignment(iconLabel, Qt::AlignVCenter);
    contentFrm->setToolTip(QString("%1\n%2").arg(info.file_info.fileName).arg(info.file_info.fileSize));

    auto* downloadBtn = new QToolButton(this);
    downloadBtn->setObjectName("FileItemWgt_downloadBtn");
    downloadBtn->setToolTip(tr("下载"));
    auto* openPathBtn = new QToolButton(this);
    openPathBtn->setObjectName("FileItemWgt_openPathBtn");
    openPathBtn->setToolTip(tr("打开文件夹"));
    openPathBtn->setFixedSize(26, 26);
    downloadBtn->setFixedSize(26, 26);
    auto* mainLay = new QHBoxLayout;
    mainLay->addWidget(contentFrm);
    mainLay->addItem(new QSpacerItem(30, 10, QSizePolicy::MinimumExpanding));
    mainLay->addWidget(downloadBtn);
    mainLay->addWidget(openPathBtn);

    lay->addLayout(mainLay);

    auto localPath = QTalk::File::getRealFilePath(info.msg_id.toStdString(), info.file_info.fileMd5.toStdString());
    bool downloaded = !localPath.isEmpty();
    downloadBtn->setVisible(!downloaded);
    openPathBtn->setVisible(downloaded);

    connect(openPathBtn, &QToolButton::clicked, [localPath](){
        QFileInfo info(localPath);
        if (localPath.isEmpty() || !info.exists()) {
            QtMessageBox::information(g_pMainPanel, tr("提醒"), tr("未找到本地文件"));
        } else {
            QTalk::File::openFileFolder(localPath);
        }
    });
    connect(downloadBtn, &QToolButton::clicked, [info](){
        QDesktopServices::openUrl(QUrl(info.file_info.fileLink));
    });
}

/** CommonTrd item **/
SearchCommonTrdItem::SearchCommonTrdItem(const StNetMessageResult &info, QWidget *parent)
    :SearchItemBase(info, parent), _link(info.common_trd.link), _xmppId(info.xmpp_id)
{
    contentFrm = new QFrame(this);
    lay->addWidget(contentFrm);
    lay->setSpacing(10);
    contentFrm->setFixedWidth(350);
    contentFrm->setObjectName("borderFrm");
    //
    auto* imageLabel = new NetImageLabel(info.common_trd.img, this);
    imageLabel->setFixedSize(30, 30);
    if(info.common_trd.img.isEmpty())
        imageLabel->setLocalImage(":/chatview/image1/defaultShareIcon.png");
    //
    auto* titleLabel = new QLabel(info.common_trd.title, this);
    titleLabel->setObjectName("CommonTrdInfoItemTitleLabel");

    auto* descLabel = new QLabel(info.common_trd.desc, this);
    descLabel->setObjectName("CommonTrdInfoItemContentLabel");
    if(info.common_trd.desc.isEmpty())
        descLabel->setVisible(false);

    auto* rLay = new QVBoxLayout();
    rLay->setMargin(0);
    rLay->setSpacing(5);
    rLay->addWidget(titleLabel);
    rLay->addWidget(descLabel);
    //
    auto* contentLay = new  QHBoxLayout(contentFrm);
    contentLay->setContentsMargins(10, 5, 5, 5);
    contentLay->addWidget(imageLabel);
    contentLay->addLayout(rLay);
    contentLay->setAlignment(imageLabel, Qt::AlignVCenter);
}

//
void SearchCommonTrdItem::mousePressEvent(QMouseEvent *e) {

    QString linkUrl = _link;

    if(e->button() == Qt::LeftButton && contentFrm->geometry().contains(e->pos()))
    {
//        bool userDftBrowser = AppSetting::instance().getOpenLinkWithAppBrowser();
        MapCookie cookies;
        cookies["ckey"] = QString::fromStdString(PLAT.getClientAuthKey());
        if (/**userDftBrowser ||**/
            linkUrl.contains(NavigationManager::instance().getShareUrl().data()))
            WebService::loadUrl(QUrl(linkUrl), false, cookies);
        else
            QDesktopServices::openUrl(QUrl(linkUrl));
    }

    QFrame::mousePressEvent(e);
}

/** code item **/
SearchCodeItem::SearchCodeItem(const StNetMessageResult& info, QWidget* parent)
    :SearchItemBase(info, parent)
    , _codeStyle(info.code.codeStyle)
    , _codeLanguage(info.code.language)
    , _code(info.code.code)
{
    contentFrm = new QFrame(this);
    lay->addWidget(contentFrm);
    lay->setSpacing(10);
    contentFrm->setFixedWidth(350);
    contentFrm->setObjectName("borderFrm");
    //
    auto* imageLabel = new NetImageLabel("", this);
    imageLabel->setFixedSize(30, 30);
    imageLabel->setLocalImage(":/chatview/image1/messageItem/code.png");
    //
    auto code = info.code.code.left(200).replace("\n", "");
    auto* titleLabel = new QLabel(tr("代码片段"), this);
    titleLabel->setObjectName("CodeTitle");
    auto* descLabel = new QLabel(code, this);
    descLabel->setWordWrap(true);
    descLabel->setObjectName("CodeContent");

    auto* rLay = new QVBoxLayout();
    rLay->setMargin(0);
    rLay->setSpacing(5);
    rLay->addWidget(titleLabel);
    rLay->addWidget(descLabel);
    //
    auto* contentLay = new  QHBoxLayout(contentFrm);
    contentLay->setContentsMargins(10, 10, 5, 5);
    contentLay->addWidget(imageLabel);
    contentLay->addLayout(rLay);
    contentLay->setAlignment(imageLabel, Qt::AlignTop);
}

void SearchCodeItem::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::LeftButton && contentFrm->geometry().contains(e->pos()))
    {
        g_pMainPanel->showShowCodeWnd(_codeStyle, _codeLanguage, _code);
    }
    QFrame::mousePressEvent(e);
}

/** audio video item **/
SearchAudioVideoItem::SearchAudioVideoItem(const StNetMessageResult& info, QWidget* parent)
    :SearchItemBase(info, parent)
{
    QString content = tr("视频通话");
    bool isCalled = info.from == PLAT.getSelfXmppId().data();
    if(!info.extend_info.isEmpty())
    {
        QJsonDocument document = QJsonDocument::fromJson(info.extend_info.toUtf8());
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

    if(QTalk::Entity::WebRTC_MsgType_Video == info.msg_type)
        content = tr("发送端版本过低，视频无法接通");

    auto* contentFrm = new QFrame(this);

    contentFrm->setObjectName("messReceiveContentFrm");
    auto * contentLay = new QHBoxLayout(contentFrm);
    contentLay->setMargin(12);
    auto *pIconLabel = new QLabel(this);
    auto *contentLab = new QLabel(content, this);
    contentLay->setSpacing(1);
    contentLay->addWidget(pIconLabel, 0);
    contentLay->addWidget(contentLab, 1);

    QPixmap icon = QTalk::qimage::loadImage(":/chatview/image1/messageItem/AudioVideo.png", false);
    icon = icon.scaled(25, 25, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pIconLabel->setPixmap(icon);
    contentLab->adjustSize();
    contentFrm->setMaximumWidth(contentLab->width() + 25 + 25);
    lay->addWidget(contentFrm);
}

/** search item **/
SearchVideoItem::SearchVideoItem(const StNetMessageResult &info, QWidget *parent)
    :SearchItemBase(info, parent)
{
    imageLabel = new NetImageLabel(info.video.thumbUrl, this);
    imageLabel->setFixedSize(info.video.width, info.video.height);
    imageLabel->showVideoMask();

    lay->setSpacing(5);
    lay->addWidget(imageLabel);
    //
    _videoUrl = info.video.videoUrl;
    if(!_videoUrl.startsWith("http"))
    {
        if(_videoUrl.startsWith("file") || _videoUrl.startsWith("/file"))
        {
            _videoUrl = QString("%1/%2").arg(NavigationManager::instance().getFileHttpHost().data())
                    .arg(_videoUrl);
        }
        else
            _videoUrl = QString("http://%2").arg(_videoUrl);
    }

}

void SearchVideoItem::mousePressEvent(QMouseEvent *e) {
    if(e->button() == Qt::LeftButton && imageLabel->geometry().contains(e->pos()))
    {
        QDesktopServices::openUrl(_videoUrl);
    }
    QFrame::mousePressEvent(e);
}