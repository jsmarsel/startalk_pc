//
// Created by cc on 2018/11/16.
//

#include "PictureBrowser.h"
#include "TitleFrm.h"
#include "../Platform/Platform.h"
#include "../CustomUi/QtMessageBox.h"
#include "../UICom/uicom.h"
#include "../Platform/NavigationManager.h"
#include <QHBoxLayout>
#include <QWindow>
#include <QKeyEvent>
#include <QApplication>
#include <QFileInfo>
#include <QtConcurrent>

PictureBrowser::PictureBrowser()
        : UShadowDialog(nullptr, true)
        , _curIndex(0)
        , _hasBefore(false)
        , _hasNext(false)
{
//    _msgManager = new PictureMsgManager;
    Qt::WindowFlags flags = Qt::Dialog | Qt::WindowContextHelpButtonHint | Qt::FramelessWindowHint
                            | Qt::WindowFullscreenButtonHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_QuitOnClose, false);
    initUi();

    connect(this, &PictureBrowser::sgGotSourceImg, this, &PictureBrowser::gotSourceImg);
}

void PictureBrowser::initUi() {
    this->setMinimumSize(480, 480);
    // title
    _pPTitleFrm = new TitleFrm(this);

    _pPicFrm = new PictureFrm(this);
    _pPicFrm->setContentsMargins(6, 6, 6, 6);
    _pPicFrm->setObjectName("bottomFrame");
    _pPicFrm->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(_pPTitleFrm);
    layout->addWidget(_pPicFrm);
    _pCenternWgt->setLayout(layout);

    setMoverAble(true, _pPTitleFrm);
}

void PictureBrowser::onShowChatPicture(const QString &messageId, const QString &messageContent, int index) {

    //
    {
        _pPTitleFrm->setBeforeBtnEnable(false);
        _pPTitleFrm->setNextBtnEnable(false);

        _hasBefore = true;
        _hasNext = true;
        _images.clear();
        _curIndex = index;
        analyseMessage(messageId.toStdString() ,messageContent, true);
        if(_images.empty())
        {
            QtMessageBox::warning(this, tr("警告"), tr("无法加载该图片!"));
            return;
        }
        loadImage(true);
    }
    getBeforeImgs(messageId.toStdString());
    getNextImgs(messageId.toStdString());
    QApplication::setActiveWindow(this);
    this->raise();
}

void PictureBrowser::showPicture(const QString &picPath, const QString& linkPath) {
    if(_pPicFrm->loadNewPicture(picPath, true))
    {
        _hasBefore = false;
        _hasNext = false;
        _images.clear();
        _curIndex = 0;
        if(this->isVisible())
        {
            this->setVisible(false);
        }
        this->showNormal();
        activateWindow();

        _curLink = linkPath.toStdString();
        QtConcurrent::run([this, linkPath](){
            std::string srcImg =PictureMsgManager::getSourceImage(linkPath.toStdString());
            if(!srcImg.empty())
                emit sgGotSourceImg(linkPath, QString::fromStdString(srcImg), true);
        });
    }
}

void PictureBrowser::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Down || e->key() == Qt::Key_Right)
    {
        turnNext();
    }
    else if(e->key() == Qt::Key_Up || e->key() == Qt::Key_Left)
    {
        turnBefore();
    }
    UShadowDialog::keyPressEvent(e);
}


//
QString getEmotionUrl(const QString& content)
{
    QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");

    QString url ;

    if (regExp.indexIn(content) != -1) {

        QString item = regExp.cap(0); // 符合条件的整个字符串
        QString type = regExp.cap(1); // 多媒体类型
        QString val = regExp.cap(2); // 路径
        QString strWidth = regExp.cap(3); // 宽高

        if ("image" == type) {
        } else if ("emoticon" == type) {
            QString shortCut = val, pkgid;
            shortCut.remove("[").remove("]");
            if (!strWidth.isEmpty()) {
                QRegExp exp("width=(.+) height=(.+)");
                if (exp.indexIn(strWidth) >= 0) {
                    pkgid = exp.cap(1);
                }
            }

            if(!shortCut.isEmpty() && !pkgid.isEmpty())
            {
                url = QString("%1/file/v2/emo/d/e/%2/%3/org").arg(NavigationManager::instance().getFileHttpHost().data())
                        .arg(pkgid, shortCut);
            }
        }
    }

    return url;
}

/**
 * analyse message content
 * Don't lock it.
 */
void PictureBrowser::analyseMessage(const std::string& msgId, const QString& msg, bool next)
{
    QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");
    regExp.setMinimal(true);

    std::vector<std::pair<std::string, std::string> > tmpImages;

    int pos = 0;
    while ((pos = regExp.indexIn(msg, pos)) != -1) {
        QString item = regExp.cap(0); // 符合条件的整个字符串
        QString type = regExp.cap(1); // 多媒体类型
        QString val = regExp.cap(2); // 路径

        if ("image" == type)
        {

            if(!val.isEmpty())
            {
                QString rawUrl = QUrl(val).toEncoded();
                if(next)
                    _images.emplace_back(msgId, rawUrl.toStdString());
                else
                    tmpImages.insert(tmpImages.begin(),
                            std::make_pair(msgId, rawUrl.toStdString()));
            }
        }
        else if("emoticon" == type)
        {
            // 暂不显示表情
//            auto emoUrl = getEmotionUrl(item);
//            if(!emoUrl.isEmpty())
//            {
//                QString rawUrl = QUrl(emoUrl).toEncoded();
//                if(next)
//                    _images.emplace_back(msgId, rawUrl.toStdString());
//                else
//                    tmpImages.insert(tmpImages.begin(),
//                                     std::make_pair(msgId, rawUrl.toStdString()));
//            }
        }
        //
        pos += item.length();
    }

    if(!next && !tmpImages.empty())
    {
        for(const auto& tmpImg : tmpImages)
        {
            _curIndex++;
            _images.insert(_images.begin(), tmpImg);
        }
    }

}

void PictureBrowser::gotSourceImg(const QString &link, const QString &srcImg, bool isFirst)
{
    if(this->isVisible() && _curLink == link.toStdString())
    {
        _pPicFrm->setEnabled(true);
        _pPicFrm->loadNewPicture(srcImg, isFirst);
    }

}

void PictureBrowser::turnBefore()
{
    {
//        QMutexLocker locker(&_mutex);
        if(!_images.empty() && _curIndex > 0)
        {
            _curIndex--;
            loadImage(false);
            return;
        }
        else
        {
            if(_hasBefore){
                getBeforeImgs(_images[_curIndex].first);
                return;
            }
        }
    }
    //
    QtMessageBox::warning(this, tr("警告"), tr("无法加载更早的图片了!"));
}

void PictureBrowser::turnNext()
{
    {
//        QMutexLocker locker(&_mutex);
        if(!_images.empty() && _curIndex < _images.size() - 1)
        {
            _curIndex++;
            loadImage(false);
            return;
        }
        else
        {
            if(_hasNext)
            {
                getNextImgs(_images[_curIndex].first);
                return;
            }
        }
    }
    //
    QtMessageBox::warning(this, tr("警告"), tr("已看完最后一张图片!"));
}

/**
 * load image
 * Don't lock it.
 */
void PictureBrowser::loadImage(bool isFirst)
{
    if(_images.size() <= _curIndex)
    {
        QtMessageBox::error(this, tr("错误"), tr("消息解析错误!"));
        return;
    }
    auto item = _images[_curIndex];
    _curLink = item.second;
    auto imgPath = QString::fromStdString(QTalk::GetSrcImagePathByUrl(_curLink));
    QFileInfo info(imgPath);
    if(!info.exists() || info.isDir())
    {
        imgPath = QString::fromStdString(QTalk::GetImagePathByUrl(_curLink));
        _pPicFrm->setEnabled(false);
        QtConcurrent::run([isFirst, this](){
            std::string srcImg =PictureMsgManager::getSourceImage(_curLink);
            if(!srcImg.empty())
                emit sgGotSourceImg(QString::fromStdString(_curLink), QString::fromStdString(srcImg), isFirst);
        });
    }
    info = QFileInfo(imgPath);
    if(info.exists() && info.isFile())
    {
        if(_pPicFrm->loadNewPicture(imgPath, isFirst) && !isVisible())
        {
//            this->setWindowModality(Qt::WindowModal);
#ifdef _MACOS
            QWidget *wgt = UICom::getInstance()->getAcltiveMainWnd();
            if(wgt && wgt->isFullScreen())
                this->setWindowFlags(this->windowFlags() | Qt::Tool);
#endif
            this->showNormal();
            this->raise();
        }
    }
}

/**
 *
 * @param msgId
 */
void PictureBrowser::getBeforeImgs(const std::string &msgId)
{
    static bool isGetting = false;
    if(isGetting)
        return;
    if(!_hasBefore)
        return;

    isGetting = true;

    QtConcurrent::run([msgId, this](){
        std::vector<std::pair<std::string, std::string>> msgs;
        PictureMsgManager::getBeforeImgMessages(msgId, msgs);
        QMutexLocker locker(&_mutex);
        if(msgs.empty())
        {
            _hasBefore = false;
        }
        else
        {
            if(msgs.size() < 20)
                _hasBefore = false;

            for(const auto& msg : msgs)
            {
                analyseMessage(msg.first, QString::fromStdString(msg.second), false);
            }
        }
        _pPTitleFrm->setBeforeBtnEnable(true);

        isGetting = false;
    });
}

/**
 *
 * @param msgId
 */
void PictureBrowser::getNextImgs(const std::string &msgId)
{
    static bool isGetting = false;
    if(isGetting)
        return;
    if(!_hasNext)
        return;

    isGetting = true;

    QtConcurrent::run([msgId, this](){
        std::vector<std::pair<std::string, std::string>> msgs;
        PictureMsgManager::getNextImgMessages(msgId, msgs);
        if(msgs.empty())
        {
            _hasNext = false;
        }
        else
        {
            if(msgs.size() < 20)
                _hasNext = false;

            QMutexLocker locker(&_mutex);
            for(const auto& msg : msgs)
            {
                analyseMessage(msg.first, QString::fromStdString(msg.second), true);
            }

        }
        _pPTitleFrm->setNextBtnEnable(true);

        isGetting = false;
    });
}

void PictureBrowser::changeEvent(QEvent *event)
{
    if(event->type() == QEvent::WindowStateChange)
    {
#ifdef _MACOS
        auto sts = this->windowState();
        if((sts & Qt::WindowFullScreen))
            setWindowFlags(this->windowFlags() | Qt::Tool);
        else
            setWindowFlags(this->windowFlags() & ~Qt::Tool);
#endif
    }

    UShadowDialog::changeEvent(event);
}

void PictureBrowser::onCloseWnd() {
    _hasBefore = false;
    _hasNext = false;
    _images.clear();
    if(_pPicFrm)
        _pPicFrm->onCloseWnd();
}