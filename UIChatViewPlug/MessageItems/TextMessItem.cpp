#include <utility>

#include "TextMessItem.h"
#include "TextBrowser.h"
#include "../CustomUi/HeadPhotoLab.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QDebug>
#include <QDesktopServices>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QTextDocumentFragment>
#include <QKeyEvent>
#include <blocks/block_define.h>
#include "../ChatViewMainPanel.h"
#include "../../UICom/qimage/qimage.h"
#include "../../WebService/WebService.h"
#include "../../Platform/AppSetting.h"
#include "../../Platform/Platform.h"
#include "../ChatUtil.h"
#include "../../UICom/StyleDefine.h"
#include "../../UICom/uicom.h"
#include "../QtUtil/nJson/nJson.h"

#define DEM_LINK_HTML "<a href=\"%1\" style=\"text-decoration:none; color:rgba(%2);\">%3</a>"

extern ChatViewMainPanel *g_pMainPanel;

TextMessItem::TextMessItem(const StNetMessageResult &msgInfo,
                           QWidget *parent) :
    MessageItemBase(msgInfo, parent),
    _textBrowser(nullptr)
{
    _msgs = QVector<StTextMessage>::fromStdVector(msgInfo.text_messages);
    init();
    setMessageContent(false);
    _textBrowser->installEventFilter(this);
    connect(_textBrowser, &TextBrowser::anchorClicked, this, &TextMessItem::onAnchorClicked);
    connect(_textBrowser, &TextBrowser::imageClicked, this, &TextMessItem::onImageClicked);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.16
  */
QSize TextMessItem::textWdtSize()
{
    if (_textBrowser)
        return _textBrowser->sizeHint();
    else
        return {};
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.17
  */
QSize TextMessItem::itemWdtSize()
{
    int height = qMax(_mainMargin.top() + _nameLabHeight + _mainSpacing + _contentFrm->height() + _mainMargin.bottom(),
                      _headPixSize.height()); // 头像和文本取大的
    int width = _contentFrm->width();

    if(nullptr != _readStateLabel)
        height += 12;

    return {width, height + 8};
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.16
  */
qreal TextMessItem::getRealString(const QString &src, qreal &lastLineWidth)
{
    QFontMetricsF fm(_textBrowser->font());
    m_lineHeight = fm.lineSpacing();
    qreal nCount = src.count("\n");
    qreal nMaxWidth = 0;

    if (nCount == 0)
    {
        nMaxWidth = lastLineWidth = fm.width(src) + lastLineWidth;

        if (nMaxWidth > m_textWidth)
            nMaxWidth = m_textWidth;
    }
    else
    {
        QStringList valLst = src.split("\n");

        for (int i = 0; i < (nCount + 1); i++)
        {
            const QString &value = valLst.at(i);
            qreal fmWidth = fm.width(value);

            if (i == 0)
                lastLineWidth = fmWidth += lastLineWidth;
            else
                lastLineWidth = fmWidth;

            nMaxWidth = fmWidth > nMaxWidth ? fmWidth : nMaxWidth;

            if (fm.width(value) > m_textWidth)
            {
                nMaxWidth = m_textWidth;
                break;
            }
        }
    }

    return nMaxWidth;
}

void startMovies(std::map<QString, QMovie *> &_mapMovies, TextMessItem *textItem)
{
    for(auto &item : _mapMovies)
    {
        if(nullptr == item.second)
        {
            auto *mov = g_pMainPanel->gifManager->getMovie(item.first);
            mov->setCacheMode(QMovie::CacheNone);
            QObject::connect(mov, SIGNAL(frameChanged(int)), textItem, SLOT(onMovieChanged(int)));
            mov->start();
            _mapMovies[item.first] = mov;
        }
    }
}

void deleteMovies(std::map<QString, QMovie *> &_mapMovies)
{
    for(auto &item : _mapMovies)
    {
        if(nullptr != item.second)
        {
            g_pMainPanel->gifManager->removeMovie(item.second);
            item.second = nullptr;
        }
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.16
  */
void TextMessItem::setMessageContent(bool delMov)
{
    QTextCharFormat f;
    f.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    f.setFontWordSpacing(0);
    f.setFontLetterSpacing(0);
    _textBrowser->setCurrentCharFormat(f);
    _textBrowser->clear();

    //
    if(delMov)
    {
        deleteMovies(_mapMovies);
        _mapMovies.clear();
    }

    if (_textBrowser)
    {
        int width = g_pMainPanel->width() - _headPixSize.width();

        if(_msgInfo.type == QTalk::Enum::TwoPersonChat)
            width -= 90;
        else
            width -= 250;

        m_textWidth = width;
        qreal contentMaxW = 0;
        qreal lastLineW = 0;
        QVector<QString> invalidImg;

        for (const auto &msg : _msgs)
        {
            switch (msg.type)
            {
                case StTextMessage::EM_TEXT:
                case StTextMessage::EM_LINK:
                case StTextMessage::EM_ATMSG:
                    contentMaxW = qMax(contentMaxW, getRealString(msg.content, lastLineW));
                    break;

                case StTextMessage::EM_IMAGE:
                case StTextMessage::EM_EMOTICON:
                    {
                        const QString &imagePath = msg.content;

                        if (imagePath.isEmpty() || !QFile::exists(imagePath))
                        {
                            //
                            //warn_log("load head failed, use default picture-> imagePath:{0}", imagePath);
                            //imagePath = ":/chatview/image1/defaultImage.png";
                            invalidImg.push_back(imagePath);
                        }
                        else
                        {
                        }

                        lastLineW += msg.imageWidth;
                        contentMaxW = qMax(lastLineW, contentMaxW);
                        break;
                    }

                default:
                    break;
            }
        }

        width = qMin(width, (int) (contentMaxW)) + 10;
        _textBrowser->setFixedWidth(width);//先设置最大宽度 设置文本后适应文本大小
        _textBrowser->document()->setTextWidth(width);
        //
        int imgIndex = 0;

        for (const auto &msg : _msgs)
        {
            switch (msg.type)
            {
                case StTextMessage::EM_TEXT:
                    _textBrowser->insertPlainText(msg.content);
                    break;

                case StTextMessage::EM_IMAGE:
                case StTextMessage::EM_EMOTICON:
                    {
                        QString imagePath = msg.content;
                        qreal imageWidth = msg.imageWidth;
                        qreal imageHeight = msg.imageHeight;

                        if (invalidImg.contains(imagePath))
                            imagePath = ":/chatview/image1/defaultImage.png";

                        if (QPixmap(imagePath).isNull())
                        {
                            QString realPath = QTalk::qimage::getRealImagePath(imagePath);

                            if (QPixmap(realPath).isNull())
                                imagePath = ":/chatview/image1/defaultImage.png";
                            else
                                imagePath = realPath;
                        }

                        auto suffix = QTalk::qimage::getRealImageSuffix(imagePath);

                        if(suffix.toUpper() == "GIF" && _mapMovies.find(msg.content) == _mapMovies.end())
                        {
                            _mapMovies[msg.content] = nullptr;
//                        imagePath = QTalk::qimage::getGifImagePath(imagePath, imageWidth, imageHeight);
                        }

                        QTextImageFormat imageFormat;
                        imageFormat.setWidth(imageWidth);
                        imageFormat.setHeight(imageHeight);
                        imageFormat.setName(imagePath);
                        imageFormat.setProperty(imagePropertyPath, msg.content);
                        imageFormat.setProperty(imagePropertyLink, msg.imageLink);
                        imageFormat.setProperty(imagePropertyType, msg.type);

                        if(msg.type == StTextMessage::EM_IMAGE)
                            imageFormat.setProperty(imagePropertyIndex, imgIndex++);

                        _textBrowser->textCursor().insertImage(imageFormat);
                        _textBrowser->setCurrentCharFormat(f);
                        //
                        //
                        break;
                    }

                case StTextMessage::EM_LINK:
                    {
                        QString content = msg.content;
                        QTextCharFormat linkFormat = _textBrowser->textCursor().charFormat();
                        linkFormat.setForeground(QBrush(QTalk::StyleDefine::instance().getLinkUrl()));
                        linkFormat.setAnchor(true);
                        linkFormat.setAnchorHref(msg.content);
                        linkFormat.setAnchorName(msg.content);
                        _textBrowser->textCursor().insertText(msg.content, linkFormat);
                        _textBrowser->setCurrentCharFormat(f);
                        break;
                    }

                case StTextMessage::EM_ATMSG:
                    {
                        QString content = QString(DEM_AT_HTML).arg(msg.content);
                        _textBrowser->insertHtml(content);
                        _textBrowser->setCurrentCharFormat(f);
                        break;
                    }

                default:
                    break;
            }
        }

        _textBrowser->setFixedSize(textWdtSize());
        _contentFrm->setFixedSize(textWdtSize().width() + _contentMargin.left() + _contentMargin.right(),
                                  textWdtSize().height() + _contentMargin.top() + _contentMargin.bottom());
    }

    startMovies(_mapMovies, this);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.15
  */
void TextMessItem::init()
{
    this->setFrameShape(QFrame::NoFrame);
    initLayout();
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.15
  */
void TextMessItem::initLayout()
{
    this->setMinimumHeight(20);
    _mainMargin = QMargins(15, 0, 25, 0);
    _contentMargin = QMargins(5, 5, 3, 5);
    _mainSpacing = 10;
    _contentSpacing = 0;

    if (QTalk::Entity::MessageDirectionSent == _msgInfo.direction)
    {
        _headPixSize = QSize(0, 0);
        _nameLabHeight = 0;
        _leftMargin = QMargins(0, 0, 0, 0);
        _rightMargin = QMargins(0, 0, 0, 0);
        _leftSpacing = 0;
        _rightSpacing = 0;
        initSendLayout();
    }
    else if (QTalk::Entity::MessageDirectionReceive == _msgInfo.direction)
    {
        _headPixSize = QSize(28, 28);
        _nameLabHeight = 16;
        _leftMargin = QMargins(0, 10, 0, 0);
        _rightMargin = QMargins(0, 0, 0, 0);
        _leftSpacing = 0;
        _rightSpacing = 0;
        initReceiveLayout();
    }

    if (QTalk::Enum::ChatType::GroupChat != _msgInfo.type)
        _nameLabHeight = 0;
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.16
  */
void TextMessItem::initSendLayout()
{
    auto *mainLay = new QHBoxLayout(this);
    mainLay->setContentsMargins(_mainMargin);
    mainLay->setSpacing(_mainSpacing);
    mainLay->addWidget(_btnShareCheck);
    mainLay->addItem(new QSpacerItem(40, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
    //
    auto *rightLay = new QVBoxLayout;
    rightLay->setContentsMargins(_rightMargin);
    mainLay->addLayout(rightLay);

    if (!_contentFrm)
        _contentFrm = new QFrame(this);

    _contentFrm->setObjectName("messSendContentFrm");
    //
    auto *tmpLay = new QHBoxLayout;
    tmpLay->setMargin(0);
    tmpLay->setSpacing(5);

    if(nullptr != _sending && nullptr != _resending)
    {
        tmpLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
        tmpLay->addWidget(_sending);
        tmpLay->addWidget(_resending);
    }

    tmpLay->addWidget(_contentFrm);
    tmpLay->setAlignment(_contentFrm, Qt::AlignRight);
    rightLay->addLayout(tmpLay);
    auto *contentLay = new QVBoxLayout;
    contentLay->setContentsMargins(_contentMargin);
    rightLay->setSpacing(_rightSpacing);
    _contentFrm->setLayout(contentLay);

    if (nullptr != _readStateLabel)
    {
        auto *rsLay = new QHBoxLayout;
        rsLay->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
        rsLay->setMargin(0);
        rsLay->setSpacing(0);
        rsLay->addWidget(_readStateLabel);
        rightLay->addLayout(rsLay);
        this->setContentsMargins(0, 0, 0, 5);
    }

    if (nullptr == _textBrowser)
        _textBrowser = new TextBrowser(this);

    contentLay->addWidget(_textBrowser);
    contentLay->setSpacing(_contentSpacing);
    mainLay->setStretch(0, 1);
    mainLay->setStretch(1, 0);
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.16
  */
void TextMessItem::initReceiveLayout()
{
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
            && QTalk::Entity::MessageDirectionReceive == _msgInfo.direction )
    {
        auto *nameLay = new QHBoxLayout;
        nameLay->setMargin(0);
        nameLay->setSpacing(5);
        nameLay->addWidget(_nameLab);
        nameLay->addWidget(_medalWgt);
        rightLay->addLayout(nameLay);
    }

    if (!_contentFrm)
        _contentFrm = new QFrame(this);

    _contentFrm->setObjectName("messReceiveContentFrm");
    rightLay->addWidget(_contentFrm);
    rightLay->setStretch(0, 0);
    rightLay->setStretch(1, 1);
    auto *contentLay = new QVBoxLayout;
    contentLay->setContentsMargins(_contentMargin);
    _contentFrm->setLayout(contentLay);

    if (!_textBrowser)
        _textBrowser = new TextBrowser(this);

//    _textBrowser->setFixedWidth(qMin(this->width() - 28 - 30, MAX_CONTENT_WIDTH));//先设置最大宽度 设置文本后适应文本大小
    contentLay->addWidget(_textBrowser);
    contentLay->setSpacing(_contentSpacing);
    auto *horizontalSpacer = new QSpacerItem(40, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLay->addItem(horizontalSpacer);

    if (QTalk::Enum::ChatType::GroupChat == _msgInfo.type)
    {
        mainLay->setStretch(0, 0);
        mainLay->setStretch(1, 0);
        mainLay->setStretch(2, 1);
    }
    else
    {
        mainLay->setStretch(0, 0);
        mainLay->setStretch(1, 1);
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.19
  */
void TextMessItem::onAnchorClicked(const QUrl &url)
{
    QString strUrl = url.toString();

//    bool userDftBrowser = AppSetting::instance().getOpenLinkWithAppBrowser();
    if (!strUrl.startsWith("http"))
        strUrl = (QString("http://%1").arg(strUrl));

//    if (userDftBrowser)
//        WebService::loadUrl(QUrl(strUrl));
//    else
    QDesktopServices::openUrl(QUrl(strUrl));
}

//
void TextMessItem::copyText()
{
    if (_textBrowser)
    {
        auto cursor = _textBrowser->textCursor();
        //
        auto *mimeData = new QMimeData;
        QString mimeDataText;
        nJson objs;

        if(cursor.hasSelection())
        {
            int start = cursor.selectionStart();
            int end = cursor.selectionEnd();
            int index = 0;
            auto it = _msgs.begin();

            for(; (it != _msgs.end() && index < end); it++)
            {
                switch (it->type)
                {
                    case StTextMessage::EM_TEXT:
                    case StTextMessage::EM_LINK:
                    case StTextMessage::EM_ATMSG:
                        {
                            auto maxS = it->content.size() + index;

                            if(maxS < start)
                            {
                                index = maxS;
                                break;
                            }

                            auto s = qMax(start, index);
                            QString data = it->content.mid(s - index, qMin(end, maxS) - s);
                            mimeDataText.append(data);
                            nJson obj;
                            obj["text"] = data.toStdString().data();
                            obj["type"] = 1; // 1  文字 2 图片 ...
                            objs.push_back(obj);
                            index = maxS;
                            break;
                        }

                    case StTextMessage::EM_IMAGE:
                    case StTextMessage::EM_EMOTICON:
                        {
                            index++;

                            if(index < start)
                                break;

                            mimeDataText.append(tr(" [图片] "));
                            // todo send message image link
                            nJson obj;
                            obj["type"] = 2; // 1  文字 2 图片 ...
                            obj["imageLink"] = it->imageLink.toStdString().data();
                            obj["image"] = it->content.toStdString().data();
                            objs.push_back(obj);
                            break;
                        }

                    default:
                        break;
                }
            }
        }
        else
        {
            if(isImageContext())
            {
                auto link = getImageLink();
                std::string tmpImgPath = QTalk::GetImagePathByUrl(link.toStdString());
                std::string srcImgPath = QTalk::GetSrcImagePathByUrl(link.toStdString());
                QFileInfo srcInfo(srcImgPath.data());
                QString imagePath = "";

                if(srcInfo.exists() && srcInfo.isFile())
                    imagePath = srcImgPath.data();
                else
                    imagePath = tmpImgPath.data();

                QPixmap pixmap = QTalk::qimage::loadImage(imagePath, false);

                if(!pixmap.isNull())
                {
                    mimeData->setImageData(pixmap.toImage());
                    nJson obj;
                    obj["type"] = 2; // 1  文字 2 图片 ...
                    obj["imageLink"] = link.toStdString().data();
                    obj["image"] = imagePath.toStdString().data();
                    objs.push_back(obj);
                    std::string userData = objs.dump();
                    mimeData->setData("userData", userData.data());
                    QApplication::clipboard()->setMimeData(mimeData);
                    return;
                }
            }

            for(const auto &msg : _msgs)
            {
                switch (msg.type)
                {
                    case StTextMessage::EM_TEXT:
                    case StTextMessage::EM_LINK:
                    case StTextMessage::EM_ATMSG:
                        {
                            mimeDataText.append(msg.content);
                            nJson obj;
                            obj["type"] = 1; // 1  文字 2 图片 ...
                            obj["text"] = msg.content.toStdString().data();
                            objs.push_back(obj);
                            break;
                        }

                    case StTextMessage::EM_IMAGE:
                    case StTextMessage::EM_EMOTICON:
                        {
                            mimeDataText.append(tr(" [图片] "));
                            nJson obj;
                            obj["type"] = 2; // 1  文字 2 图片 ...
                            obj["imageLink"] = msg.imageLink.toStdString().data();
                            obj["image"] = msg.content.toStdString().data();
                            objs.push_back(obj);
                            break;
                        }

                    default:
                        break;
                }
            }
        }

        std::string userData = objs.dump();

        if(mimeDataText == tr(" [图片] ") && _msgs.size() == 1 && !_msgs[0].content.isEmpty())
        {
            std::string tmpImgPath = _msgs[0].content.toStdString();
            std::string srcImgPath = QTalk::GetSrcImagePathByUrl(_msgs[0].imageLink.toStdString());
            QFileInfo srcInfo(srcImgPath.data());
            QString imagePath = "";

            if(srcInfo.exists() && srcInfo.isFile())
                imagePath = srcImgPath.data();
            else
                imagePath = _msgs[0].content;

            QPixmap pixmap = QTalk::qimage::loadImage(imagePath, false);
            mimeData->setImageData(pixmap.toImage());
        }
        else
            mimeData->setText(mimeDataText);

        mimeData->setData("userData", userData.data());
        QApplication::clipboard()->setMimeData(mimeData);
    }
}

/**
 *
 * @param imageLink
 */
void TextMessItem::onImageClicked(int index)
{
    const QString &messageId = _msgInfo.msg_id;
    const QString &msgContent = _msgInfo.body;
    emit g_pMainPanel->showChatPicture(messageId, msgContent, index);
}

bool TextMessItem::event(QEvent *e)
{
    switch (e->type())
    {
        case QEvent::Show:
            startMovies(_mapMovies, this);
            break;

        case QEvent::Hide:
            deleteMovies(_mapMovies);
            break;

        case QEvent::Paint:
            {
                _paintTime = QDateTime::currentMSecsSinceEpoch();

                for(auto &item : _mapMovies)
                {
                    if(nullptr != item.second)
                    {
                        auto *mov = item.second;

                        if(mov->state() == QMovie::NotRunning || mov->state() == QMovie::Paused)
                            mov->start();
                    }
                }

                break;
            }

        default:
            break;
    }

    return MessageItemBase::event(e);
}

bool TextMessItem::eventFilter(QObject *o, QEvent *e)
{
    if (o == _textBrowser && e->type() == QEvent::KeyPress)
    {
        auto evt = (QKeyEvent *) e;

        if (evt == QKeySequence::Copy || (evt->modifiers() == Qt::ControlModifier && evt->key() == Qt::Key_C))
        {
            copyText();
            e->accept();
            return false;
        }
    }

    return MessageItemBase::eventFilter(o, e);
}

/**
 *
 */
bool TextMessItem::isImageContext()
{
    auto fmt = _textBrowser->getCurrentCursor().charFormat();
    bool ret = fmt.isImageFormat();
    auto type = fmt.property(imagePropertyType).toInt();
    ret = type == StTextMessage::EM_IMAGE;

    if(ret)
        _strImagePath = fmt.property(imagePropertyLink).toString();
    else
        _strImagePath = QString();

    return ret;
}

QString TextMessItem::getImageLink()
{
    return _strImagePath;
}



/**
 *
 * @param link
 */
void TextMessItem::onImageDownloaded(const QString &link)
{
    for(auto &msg : _msgs)
    {
        if(msg.type == StTextMessage::EM_IMAGE && msg.imageLink == link)
        {
            msg.content = QTalk::GetImagePathByUrl(link.toStdString()).data();
            QTalk::Image::scaImageSizeByPath(msg.content, msg.imageWidth, msg.imageHeight);
        }
        else if(msg.type == StTextMessage::EM_EMOTICON)
        {
            QFileInfo info(link);

            if(info.baseName() == msg.imageLink)
            {
                msg.content = link;
                QTalk::Image::scaImageSizeByPath(msg.content, msg.imageWidth, msg.imageHeight);
            }
        }
    }

    //
    setMessageContent();
}

void TextMessItem::onMovieChanged(int cur)
{
    auto *mov = qobject_cast<QMovie *>(sender());

    if(mov)
    {
        auto itFind = std::find_if(_mapMovies.begin(), _mapMovies.end(), [mov](const auto & pair)
        {
            return pair.second == mov;
        });

        if(itFind != _mapMovies.end())
        {
            _textBrowser->document()->addResource(QTextDocument::ImageResource,
                                                  itFind->first, mov->currentImage());
            _textBrowser->setLineWrapColumnOrWidth(_textBrowser->lineWrapColumnOrWidth());
        }

        //
        if(QDateTime::currentMSecsSinceEpoch() - _paintTime > 3000)
            mov->stop();
    }
}
