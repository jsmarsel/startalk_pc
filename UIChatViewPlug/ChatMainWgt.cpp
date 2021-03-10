#include "ChatMainWgt.h"
#include <QDateTime>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QScrollBar>
#include <QFileDialog>
#include <QJsonObject>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QStandardPaths>
#include <QEvent>
#include <QJsonArray>
#include <QPointer>
#include <QUrlQuery>
#include "ChatViewMainPanel.h"
#include "MessageItems/TextMessItem.h"
#include "MessageItems/ImageMessItem.h"
#include "MessageItems/FileSendReceiveMessItem.h"
#include "MessageItems/TipMessageItem.h"
#include "MessageItems/NoteMessageItem.h"
#include "MessageItems/VoiceMessageItem.h"
#include "MessageItems/EmojiMessItem.h"
#include "MessageItems/VideoMessageItem.h"
#include "../Emoticon/EmoticonMainWgt.h"
#include "../QtUtil/Entity/JID.h"
#include "InputWgt.h"
#include "../Platform/Platform.h"
#include "../CustomUi/LiteMessageBox.h"
#include "../CustomUi/QtMessageBox.h"
#include "../QtUtil/nJson/nJson.h"
#include "../QtUtil/Utils/utils.h"
#include "../UICom/qimage/qimage.h"
#include "../include/perfcounter.h"
#include "search/LocalSearchMainWgt.h"
#include "ChatUtil.h"
#include "MessageItems/HotLineAnswerItem.h"
#include "ShareMessageFrm.h"

#define SWITCH_SAVE_MESSAGE_NUM 15
#define SWITCH_DELETE_MESSAGE_MAX_NUM 20

extern ChatViewMainPanel *g_pMainPanel;

/** ChatMainWgt  */
ChatMainWgt::ChatMainWgt(ChatViewItem *pViewItem)
    : QListView(pViewItem),
      _pViewItem(pViewItem),
      _hasnotHistoryMsg(true),
      _oldScrollBarVal(0),
      _selectEnable(false)
{
    // 设置滚动方式
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->verticalScrollBar()->setSingleStep(15);
    this->setObjectName("ChatMainWgt");
    this->setFrameShape(QFrame::NoFrame);
    this->setFocusPolicy(Qt::NoFocus);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->verticalScrollBar()->setVisible(false);
    //
    _pSrcModel = new QStandardItemModel(pViewItem);
    _pModel = new ChatMainSoreModel(this);
    _pModel->setSourceModel(_pSrcModel);
    this->setModel(_pModel);
    //
    _pDelegate = new ChatMainDelegate(_mapItemWgt, this);
    this->setItemDelegate(_pDelegate);
    //
    _pNewMessageTipItem = new NewMessageTip(this);
    _pNewMessageTipItem->setVisible(false);
    _pAtMessageTipItem = new AtMessageTip(this);
    _pAtMessageTipItem->setVisible(false);
    //
    _pMenu = new QMenu(this);
    _pMenu->setAttribute(Qt::WA_TranslucentBackground, true);
    saveAsAct = new QAction(tr("另存为"), _pMenu);
    copyAct = new QAction(tr("拷贝"), _pMenu);
    quoteAct = new QAction(tr("引用"), _pMenu);
    forwardAct = new QAction(tr("转发"), _pMenu);
    revokeAct = new QAction(tr("撤回"), _pMenu);
    collectionAct = new QAction(tr("收藏为表情"), _pMenu);
    shareMessageAct = new QAction(tr("多选"), _pMenu);
    qrcodeAct = new QAction(tr("识别图片二维码"), _pMenu);
    _pMenu->addAction(copyAct);
    _pMenu->addAction(saveAsAct);
    _pMenu->addAction(collectionAct);
    _pMenu->addAction(qrcodeAct);
    _pMenu->addAction(forwardAct);
    _pMenu->addSeparator();
    _pMenu->addAction(quoteAct);
    _pMenu->addAction(revokeAct);
    _pMenu->addSeparator();
    _pMenu->addAction(shareMessageAct);
    //
    _downloadTask = new DownloadImageTask;
    _downloadTask->moveToThread(&g_pMainPanel->downloadImageThread());
    connect(this, &ChatMainWgt::sgDownloadImageOperate, _downloadTask, &DownloadImageTask::downloadImage);
    connect(_downloadTask, &DownloadImageTask::downloadFinish, this, &ChatMainWgt::onImageDownloaded);
    connect(this, &ChatMainWgt::sgDownloadEmoOperate, _downloadTask, &DownloadImageTask::downloadEmo);
    //
    setContextMenuPolicy(Qt::CustomContextMenu);
    qRegisterMetaType<std::map<std::string, QInt32> >("std::map<std::string, QInt32 >");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<QInt64>("QInt64");
    connects();
    //
    QPointer<ChatMainWgt> pThis(this);
    std::function<int(STLazyQueue<bool>*)> adjustItemsFunc = [pThis](STLazyQueue<bool> *q) ->int
    {
        int runningCount = 0;
        bool isOk = true;

        if (q != nullptr && !q->empty())
        {
            while (!q->empty())
            {
                isOk &= q->front();
                q->pop();
                runningCount++;
            }

            if(pThis)
                emit pThis->adjustItems(isOk);
        }

        return runningCount;
    };
    std::function<int(STLazyQueue<bool>*)> selectItemFun = [pThis](STLazyQueue<bool> *q) ->int
    {
        int runningCount = 0;

        if (q != nullptr && !q->empty())
        {
            while (!q->empty())
            {
                q->pop();
                runningCount++;
            }

            if(pThis)
                emit pThis->sgSelectItem();
        }

        return runningCount;
    };
    _resizeQueue = new STLazyQueue<bool>(100, adjustItemsFunc);
    _selectItemQueue = new STLazyQueue<bool>(50, selectItemFun);
}

ChatMainWgt::~ChatMainWgt()
{
    _resizeQueue->clear();
    delete _resizeQueue;
    _selectItemQueue->clear();
    delete _selectItemQueue;

    if(_pSrcModel)
        _pSrcModel->clear();
};

void ChatMainWgt::connects()
{
    //
    connect(_pViewItem->_pShareMessageFrm, &ShareMessageFrm::sgShareMessage, this, &ChatMainWgt::onShareMessage);
    connect(this, &ChatMainWgt::sgSelectedSize, _pViewItem->_pShareMessageFrm, &ShareMessageFrm::setSelectCount);
    connect(this, &ChatMainWgt::sgUploadShareMsgSuccess, g_pMainPanel, &ChatViewMainPanel::sendShareMessage);
    //
    connect(this, SIGNAL(sgRecvFRileProcess(double, double, double, double, double, std::string)),
            this, SLOT(onRecvFRileProcess(double, double, double, double, double, std::string)));
    connect(this, &ChatMainWgt::gotReadStatueSignal, this, &ChatMainWgt::onRecvReadState);
    connect(this, &ChatMainWgt::sgGotMState, this, &ChatMainWgt::onMState);
    connect(this, &ChatMainWgt::sgDisConnected, this, &ChatMainWgt::onDisconnected);
    connect(this, &ChatMainWgt::sgSendFailed, this, &ChatMainWgt::onSendMessageFailed);
    connect(this, &ChatMainWgt::sgDownloadFileFailed, this, &ChatMainWgt::onDownloadFileFailed);
    connect(this, &ChatMainWgt::sgUploadFileSuccess, this, &ChatMainWgt::onUploadFileSuccess);
    connect(this, &ChatMainWgt::updateRevokeSignal, this, &ChatMainWgt::updateRevokeMessage, Qt::QueuedConnection);
//    connect(this, &ChatMainWgt::sgImageDownloaded, this, &ChatMainWgt::onImageDownloaded, Qt::QueuedConnection);
    connect(this, &ChatMainWgt::customContextMenuRequested, this, &ChatMainWgt::onCustomContextMenuRequested);
    connect(this, &ChatMainWgt::sgDownloadFileSuccess, this, &ChatMainWgt::onDownloadFile);
    connect(copyAct, &QAction::triggered, this, &ChatMainWgt::onCopyAction);
    connect(saveAsAct, &QAction::triggered, this, &ChatMainWgt::onSaveAsAction);
    connect(revokeAct, &QAction::triggered, this, &ChatMainWgt::onRevokeAction);
    connect(quoteAct, &QAction::triggered, this, &ChatMainWgt::onQuoteAct);
    connect(collectionAct, &QAction::triggered, this, &ChatMainWgt::onCollectionAct);
    connect(qrcodeAct, &QAction::triggered, this, &ChatMainWgt::onQRCodeAct);
    connect(shareMessageAct, &QAction::triggered, [this]()
    {
        if(!_selectEnable)
            _pViewItem->setShareMessageState(true);
    });
    connect(this, &ChatMainWgt::showTipMessageSignal,
            this, &ChatMainWgt::showTipMessage);
    connect(this, &ChatMainWgt::sgJumTo,
            this, &ChatMainWgt::jumTo);
    connect(forwardAct, &QAction::triggered, this, &ChatMainWgt::onForwardAct);
    QScrollBar *scrolbar = this->verticalScrollBar();
    scrolbar->installEventFilter(this);
    connect(scrolbar, &QScrollBar::valueChanged, this, &ChatMainWgt::onScrollBarChanged);
//    connect(scrolbar, &QScrollBar::rangeChanged, [this](int , int ){
//        emit this->sgJumTo();
//    });
    connect(this, &ChatMainWgt::adjustItems, this, &ChatMainWgt::onAdjustItems, Qt::QueuedConnection);
//    connect(this, &QListView::selectionChanged, this, &ChatMainWgt::onItemSelectionChanged, Qt::QueuedConnection);
    connect(this, &ChatMainWgt::sgSelectItem, this, &ChatMainWgt::onItemChanged, Qt::QueuedConnection);
}

//
void ChatMainWgt::onCustomContextMenuRequested(const QPoint &pos)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));
    bool isItem = (nullptr != itemWgt);
    saveAsAct->setVisible(isItem);
    revokeAct->setVisible(isItem);
    quoteAct->setVisible(isItem);
    copyAct->setVisible(isItem);
    forwardAct->setVisible(isItem);
    collectionAct->setVisible(false);
    shareMessageAct->setVisible(true);
    qrcodeAct->setVisible(false);

    if (nullptr != itemWgt)
    {
        saveAsAct->setEnabled(true);
        revokeAct->setEnabled(true);
        quoteAct->setEnabled(true);
        copyAct->setEnabled(true);

        switch (itemWgt->messageType())
        {
            case QTalk::Entity::MessageTypeText:
            case QTalk::Entity::MessageTypeRobotAnswer:
            case QTalk::Entity::MessageTypeGroupAt:
            case QTalk::Entity::MessageTypeImageNew:
            case QTalk::Entity::MessageTypeFile:
            case QTalk::Entity::MessageTypePhoto:
            case QTalk::Entity::MessageTypeCommonTrdInfo:
            case QTalk::Entity::MessageTypeCommonTrdInfoV2:
            case QTalk::Entity::MessageTypeSourceCode:
            case QTalk::Entity::MessageTypeSmallVideo:
                if (!itemWgt->contains(QCursor::pos())) return;

                switch (itemWgt->messageType())
                {
                    case QTalk::Entity::MessageTypeFile:
                        saveAsAct->setEnabled(true);
                        quoteAct->setEnabled(false);
                        break;

                    case QTalk::Entity::MessageTypeText:
                    case QTalk::Entity::MessageTypeRobotAnswer:
                    case QTalk::Entity::MessageTypeGroupAt:
                        {
                            auto textItem = qobject_cast<TextMessItem *>(itemWgt);
                            bool isImage = textItem && textItem->isImageContext();
                            saveAsAct->setEnabled(isImage);
                            collectionAct->setVisible(isImage);
                            qrcodeAct->setVisible(isImage);
                            quoteAct->setEnabled(true);
                            break;
                        }

                    case QTalk::Entity::MessageTypeSourceCode:
                        {
                            saveAsAct->setEnabled(false);
                            copyAct->setEnabled(true);
                            quoteAct->setEnabled(false);
                            break;
                        }

                    case QTalk::Entity::MessageTypePhoto:
                    case QTalk::Entity::MessageTypeImageNew:
                        {
                            auto *imgItem = qobject_cast<ImageMessItem *>(itemWgt);

                            if(imgItem)
                            {
                                copyAct->setEnabled(true);
                                saveAsAct->setEnabled(true);
                                quoteAct->setEnabled(true);
                                collectionAct->setVisible(true);
                                qrcodeAct->setVisible(true);
                                break;
                            }
                        }

                    case QTalk::Entity::MessageTypeCommonTrdInfo:
                    case QTalk::Entity::MessageTypeCommonTrdInfoV2:
                    case QTalk::Entity::MessageTypeSmallVideo:
                        copyAct->setEnabled(false);
                        quoteAct->setEnabled(false);

                    default:
                        saveAsAct->setEnabled(false);
                        break;
                }

                break;

            default:
                return;
        }

        bool isSelfMsg = itemWgt->_msgInfo.from.toStdString() == g_pMainPanel->getSelfUserId();
        revokeAct->setEnabled(isSelfMsg);
    }

    _pMenu->exec(mapToGlobal(pos));
}

/**
  * @函数名   resizeEvent
  * @功能描述
  * @参数
  * @author   cc
  * @date     2018/09/18
  */
void ChatMainWgt::resizeEvent(QResizeEvent *e)
{
    _resizeQueue->push(false);
    QListView::resizeEvent(e);
}

/**
  * @函数名
  * @功能描述 业务传入的消息可能跨线程 用信号槽跨线程
  * @参数
  * @date 2018.10.22
  */
void
ChatMainWgt::onRecvFRileProcess(double speed, double dtotal, double dnow, double utotal, double unow, std::string key)
{
    if(! _mapItemWgt.contains(key.data()))
        return;

    double total;
    double now;

    if (dtotal > utotal)
    {
        total = dtotal;
        now = dnow;
    }
    else
    {
        total = utotal;
        now = unow;
    }

    auto milliseconds_since_epoch = QDateTime::currentMSecsSinceEpoch();

    // 最多100ms 更新一次界面
    if (milliseconds_since_epoch - downloadProcess < 500 && (total - now) > 0.000001)
        return;

    downloadProcess = milliseconds_since_epoch;
    auto *item = _mapItemWgt[key.data()];
    {
        auto *pFileItem = qobject_cast<FileSendReceiveMessItem *>(item);

        if(pFileItem)
            pFileItem->setProcess(speed, dtotal, dnow, utotal, unow);
    }
    //
    {
        auto *pVideoItem = qobject_cast<VideoMessageItem *>(item);

        if(pVideoItem)
            pVideoItem->setProcess(speed, dtotal, dnow, utotal, unow);
    }
}


//
void ChatMainWgt::showMessageTime(const QString &messageId, const QInt64 &nCurTime)
{
    _times[nCurTime] = messageId;
    auto it = _times.find(nCurTime);

    if(it != _times.begin())
    {
        it--;

        if((nCurTime - it.key()) < 2 * 60 * 1000)
        {
            _times.remove(nCurTime);
            return;
        }
    }

    auto *item = new QStandardItem();
    item->setData(QTalk::Entity::MessageTypeTime, EM_USER_MSG_TYPE);
    item->setData(nCurTime, EM_USER_MSG_TIME);
    _pSrcModel->appendRow(item);
    _mapTimeItem.insert(messageId, item);
    _pDelegate->dealWidget(_pModel->mapFromSource(item->index()));
    item->setSizeHint({this->width(), 40});
    _pModel->sort(0);
}

/**
 *
 * @param info
 */
void ChatMainWgt::onShowMessage(StNetMessageResult info, int jumType)
{
    if(!_isLeave)
        _request_time = qMin(_request_time, info.time);

    QTalk::Entity::UID uid(info.xmpp_id.section("/", 0, 0), info.real_id.section("/", 0, 0));

    if(uid == _pViewItem->_uid)
    {}
    else
    {
        warn_log("uid error {0} -> {1} mid:{2}", info.xmpp_id.toStdString(), info.real_id.toStdString(), info.msg_id.toStdString());
        return;
    }

//    info_log("recv message {0} -> {1}", info.xmpp_id.toStdString(), info.msg_id.toStdString());
    if(info.msg_type == QTalk::Entity::MessageTypeEmpty)
        return;

    QStandardItem *item = nullptr;

    if(_mapItemWgt.contains(info.msg_id))
    {
        auto *base = qobject_cast<MessageItemBase *>(_mapItemWgt[info.msg_id]);

        if(base)
            base->updateMessageInfo(info);
    }

    if(_mapItem.contains(info.msg_id))
    {
        item = _mapItem[info.msg_id];
        StNetMessageResult oinfo = item->data(EM_USER_INFO).value<StNetMessageResult>();
        info.state = qMax(oinfo.state, info.state);
        info.read_flag = qMax(oinfo.read_flag, info.read_flag);

        if(info.body.isEmpty() && !oinfo.body.isEmpty())
            info.body = oinfo.body;

        item->setData(QVariant::fromValue(info), EM_USER_INFO);
    }
    else
    {
        showMessageTime(info.msg_id, info.time);
        //
        item = new QStandardItem();
        _mapItem.insert(info.msg_id, item);
        item->setData(QVariant::fromValue(info), EM_USER_INFO);
        _pSrcModel->appendRow(item);

        if (QTalk::Enum::GroupChat == info.type &&
                ((info.read_flag & 0x02) == 0) &&
                QTalk::Entity::MessageDirectionReceive == info.direction )
        {
            if (QTalk::Entity::MessageTypeText == info.msg_type && info.body.contains("@all"))
            {
                StShowAtInfo atInfo(true, info.user_name, item);
                _pAtMessageTipItem->addAt(atInfo);
            }
            else if(QTalk::Entity::MessageTypeGroupAt == info.msg_type)
            {
                if(info.at_users.find(PLAT.getSelfXmppId().data()) != info.at_users.end())
                {
                    StShowAtInfo atInfo(false, info.user_name, item);
                    _pAtMessageTipItem->addAt(atInfo);
                }
            }
        }
    }

    {
        item->setData(info.type, EM_USER_TYPE);
        item->setData(info.msg_type, EM_USER_MSG_TYPE);
        item->setData(info.time, EM_USER_MSG_TIME);
        item->setData(_selectEnable, EM_USER_SHOW_SHARE);
    }

    //

    //
    if(!info.text_messages.empty())
    {
        for(const auto &it : info.text_messages)
        {
            if(it.type == StTextMessage::EM_EMOTICON)
            {
                if(it.content.isEmpty())
                    downloadEmoticon(info.msg_id, it.pkgid, it.shortCut);
            }
            else if(it.type == StTextMessage::EM_IMAGE)
            {
                if(it.content.isEmpty())
                    downloadImage(info.msg_id, it.imageLink, it.imageWidth, it.imageHeight);
            }
            else {}
        }
    }

    //
    _pModel->sort(0);

    //
    if(!_isLeave)
    {
        auto index = _pModel->mapFromSource(item->index());
        _pDelegate->dealWidget(index);
        {
            // default size hint
            auto *base = dynamic_cast<MessageItemBase *>(this->indexWidget(index));

            if(base)
                item->setSizeHint({this->width(), base->itemWdtSize().height()});
            else
                item->setSizeHint({this->width(), 40});
        }
        {
            static unsigned char flag = 0;

            if(++flag % 2)
                this->resize(this->width() + 1, this->height());
            else
                this->resize(this->width() - 1, this->height());
        }

        if(info.direction == QTalk::Entity::MessageDirectionReceive && jumType == EM_JUM_INVALID)
        {
            int scrollBarVal = this->verticalScrollBar()->value();
            int maximum = this->verticalScrollBar()->maximum();
            int itemSize = item->sizeHint().height();

            if(_mapTimeItem.contains(info.msg_id))
                itemSize += 40;

            if(maximum != 0 && (maximum - scrollBarVal - itemSize) > 200)
                _pNewMessageTipItem->onNewMessage();
            else
                this->scrollToBottom();
        }
        else
        {
            _jumType = jumType;
            jumTo();
        }

        this->repaint();
//        this->setRowHidden(index.row(), false);
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.10.22
  */
void ChatMainWgt::recvFileProcess(const double &speed, const double &dtotal, const double &dnow, const double &utotal,
                                  const double &unow, const std::string &key)
{
    if(_mapItemWgt.contains(key.data()))
        emit sgRecvFRileProcess(speed, dtotal, dnow, utotal, unow, key);
}

//
void ChatMainWgt::wheelEvent(QWheelEvent *e)
{
    if(!_wheelFlag)
        _wheelFlag = true;

    int scrollBarVal = this->verticalScrollBar()->value();
    int maximum = this->verticalScrollBar()->maximum();
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    int direct = e->angleDelta().y();
#else
    int direct = e->delta();
#endif

    //上翻
    if((maximum == 0 || scrollBarVal == 0) && direct > 0)
    {
        qint64 time = 0;
        int i = 0;

        if (_pModel->rowCount() > 0)
        {
            while (true)
            {
                time = _pModel->index(i, 0).data(EM_USER_MSG_TIME).toLongLong();

                if (time != 0)
                {
                    _jumIndex = _pModel->mapToSource(_pModel->index(i, 0));
                    break;
                }

                if (i == _pModel->rowCount() - 1)
                    break;

                i++;
            }
        }

        if(_request_time != time)
        {
//            StNetMessageResult info = _jumIndex.data(EM_USER_INFO).value<StNetMessageResult>();
//            error_log("--- get min message time error {0} -> (his){1}. message info type:{2} id:{3}",
//                      time , _request_time, info.type, info.msg_id.toStdString());
            _jumIndex = {};
            time = _request_time;
        }

        if(_hasnotHistoryMsg)
        {
            if (time == LLONG_MAX)
                time = 0;

            g_pMainPanel->getHistoryMsg(time, _pViewItem->_chatType, _pViewItem->getPeerId());
        }
    }

    QListView::wheelEvent(e);
}

void ChatMainWgt::setHasNotHistoryMsgFlag(bool hasHistory)
{
    _hasnotHistoryMsg = hasHistory;

    if (!_hasnotHistoryMsg)
        emit showTipMessageSignal(QTalk::utils::getMessageId().data(), QTalk::Entity::MessageTypeGroupNotify, tr("没有更多消息了"), 0);
}

// 阅读状态
void ChatMainWgt::onRecvReadState(const std::map<std::string, QInt32> &readStates)
{
    auto it = readStates.begin();

    for (; it != readStates.end(); it++)
    {
        QString messageId = QString::fromStdString(it->first);

        if(_mapItem.contains(messageId))
        {
            StNetMessageResult info = _mapItem[messageId]->data(EM_USER_INFO).value<StNetMessageResult>();
            info.read_flag = it->second;
            _mapItem[messageId]->setData(QVariant::fromValue(info), EM_USER_INFO);
        }

        if(_mapItemWgt.contains(messageId))
        {
            auto *base = qobject_cast<MessageItemBase *>(_mapItemWgt[messageId]);

            if(base)
                base->setReadState(it->second);
        }
    }
}

void ChatMainWgt::updateRevokeMessage(const QString &fromId, const QString &messageId, const long long &time)
{
    if (_mapItem.contains(messageId) && !fromId.isEmpty())
    {
        QString userName = tr("你");

        if (g_pMainPanel->getSelfUserId() != fromId.toStdString())
        {
            std::shared_ptr<QTalk::Entity::ImUserInfo> userInfo = DB_PLAT.getUserInfo(
                        fromId.toStdString());

            if (userInfo)
                userName = QString::fromStdString(QTalk::getUserName(userInfo));
        }

        auto *item = _mapItem[messageId];

        if(item)
        {
            StNetMessageResult info = item->data(EM_USER_INFO).value<StNetMessageResult>();
            _pSrcModel->removeRow(item->row());
            _mapItem.remove(messageId);
            _mapItemWgt.remove(messageId);

            if(_mapTimeItem.contains(messageId) && _mapTimeItem[messageId])
            {
                _pSrcModel->removeRow(_mapTimeItem[messageId]->row());
                _mapTimeItem.remove(messageId);
            }

            QString content = tr("%1撤回了一条消息").arg(fromId == PLAT.getSelfXmppId().data() ?
                              tr("你") : info.user_name);
            emit showTipMessageSignal(messageId, QTalk::Entity::MessageTypeRevoke, content, time); // time error
        }
    }
}

//
void ChatMainWgt::showTipMessage(const QString &messageId, int type, const QString &content, QInt64 t)
{
    auto *item = new QStandardItem;
    StNetMessageResult info;
    info.msg_type = type;
    info.time = t;
    info.body = content;
    info.msg_id = messageId;
    item->setData(QVariant::fromValue(info), EM_USER_INFO);
    item->setData(type, EM_USER_MSG_TYPE);
    item->setData(t, EM_USER_MSG_TIME);
    item->setData(_selectEnable, EM_USER_SHOW_SHARE);
    _pSrcModel->appendRow(item);
    _mapItem[messageId] = item;
    _pDelegate->dealWidget(_pModel->mapFromSource(item->index()));
    item->setSizeHint({this->width(), 40});
    _pModel->sort(0);
    scrollToItem(item);
}

void ChatMainWgt::saveAsImage(const QString &imageLink)
{
    if(imageLink.isEmpty())
    {
        LiteMessageBox::success(QString(tr("无效图片:%1")).arg(imageLink));
        return;
    }

    std::string imageLocalFile = ChatMsgManager::getSouceImagePath(imageLink.toStdString());
    QString oldFilePath = QString::fromStdString(imageLocalFile);
    QFileInfo oldFileInfo(oldFilePath);

    if (!oldFilePath.isEmpty() && oldFileInfo.exists())
    {
        QString suffix = QTalk::qimage::getRealImageSuffix(oldFilePath).toLower();
        QString hisDir = QString::fromStdString(PLAT.getHistoryDir());

        if(suffix.toLower() == "webp")
        {
            QString newPath = QFileDialog::getSaveFileName(this, tr("请选择文件保存路径"),
                              QString("%1/%2").arg(hisDir, oldFileInfo.baseName()),
                              QString("(*.png);;(*.jpg);;(*.webp)"));

            if (!newPath.isEmpty())
            {
                PLAT.setHistoryDir(QFileInfo(newPath).absoluteDir().absolutePath().toStdString());
                QString newSuffix = QFileInfo(newPath).suffix().toUpper();
                //
                auto tmpPix = QTalk::qimage::loadImage(oldFilePath, false);

                if(!tmpPix.isNull())
                {
                    auto format = newSuffix.toUtf8().data();
                    tmpPix.save(newPath, format, 100);
                    LiteMessageBox::success(QString(tr("文件已另存为:%1")).arg(newPath));
                }
            }
        }
        else
        {
            QString saveDir = QFileDialog::getSaveFileName(this, tr("请选择文件保存路径"),
                              QString("%1/%2").arg(hisDir, oldFileInfo.baseName()),
                              QString("%1 (*.%1)").arg(suffix.isEmpty() ? "*" : suffix));

            if (!saveDir.isEmpty())
            {
                PLAT.setHistoryDir(QFileInfo(saveDir).absoluteDir().absolutePath().toStdString());
                QString newPath = QString("%1").arg(saveDir);

                if(QFileInfo(newPath).suffix().isEmpty() && !oldFileInfo.suffix().isEmpty())
                    newPath += QString(".%1").arg(oldFileInfo.suffix());

                QFile::copy(oldFilePath, newPath);
                LiteMessageBox::success(QString(tr("文件已另存为:%1")).arg(newPath));
            }
        }
    }
}

void copyImage(const QString &imageLink, const QString &imagePath)
{
    Q_UNUSED(imagePath)
    std::string srcImgPath = ChatMsgManager::getSouceImagePath(imageLink.toStdString());
    QFileInfo srcInfo(srcImgPath.data());
    auto *mimeData = new QMimeData;
//    QList<QUrl> urls;
//
//    if(srcInfo.exists() && srcInfo.isFile())
//        urls << QUrl::fromLocalFile(srcImgPath.data());
//    else
//        urls << QUrl::fromLocalFile(imagePath);
//    mimeData->setUrls(urls);
    QPixmap pixmap = QTalk::qimage::loadImage(srcImgPath.data(), false);
    mimeData->setImageData(pixmap.toImage());
    QApplication::clipboard()->setMimeData(mimeData);
}

// action 处理相关
void ChatMainWgt::onCopyAction(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    switch (itemWgt->messageType())
    {
        case QTalk::Entity::MessageTypeText:
        case QTalk::Entity::MessageTypeRobotAnswer:
        case QTalk::Entity::MessageTypeGroupAt:
            {
                auto *textWgt = qobject_cast<TextMessItem *>(itemWgt);

                if (nullptr == textWgt) return;

                textWgt->copyText();
                break;
            }

        case QTalk::Entity::MessageTypePhoto:
        case QTalk::Entity::MessageTypeImageNew:
            {
                auto *item = qobject_cast<ImageMessItem *>(itemWgt);

                if (item)
                {
                    copyImage(item->_imageLink, item->_imagePath);
                    break;
                }
            }

        default:
            return;
    }
}

// action 处理相关
void ChatMainWgt::onSaveAsAction(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    switch (itemWgt->messageType())
    {
        case QTalk::Entity::MessageTypeText:
        case QTalk::Entity::MessageTypeRobotAnswer:
        case QTalk::Entity::MessageTypeGroupAt:
            {
                auto *textItem = qobject_cast<TextMessItem *>(itemWgt);

                if (textItem)
                {
                    QString imageLink = textItem->getImageLink();
                    saveAsImage(imageLink);
                }

                break;
            }

        case QTalk::Entity::MessageTypePhoto:
        case QTalk::Entity::MessageTypeImageNew:
            {
                auto *imgItem = qobject_cast<ImageMessItem *>(itemWgt);

                if (imgItem)
                    saveAsImage(imgItem->_imageLink);

                break;
            }

        case QTalk::Entity::MessageTypeFile:
            {
                auto *fileItem = qobject_cast<FileSendReceiveMessItem *>(itemWgt);

                if (fileItem)
                    fileItem->onSaveAsAct();

                break;
            }

        default:
            return;
    }
}

// action 处理相关
void ChatMainWgt::onRevokeAction(bool)
{
    emit g_pMainPanel->sgOperator(tr("撤回消息"));
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    QInt64 now = QDateTime::currentDateTime().toMSecsSinceEpoch() - PLAT.getServerDiffTime() * 1000;

    if (now - itemWgt->_msgInfo.time > 2 * 60 * 1000)
    {
        showTipMessage(QTalk::utils::getMessageId().data(), QTalk::Entity::MessageTypeGroupNotify, tr("超过2分钟的消息不能撤回"), now);
        return;
    }

    QString messageId = itemWgt->_msgInfo.msg_id;
    auto time = QDateTime::currentMSecsSinceEpoch() - PLAT.getServerDiffTime() * 1000;
    updateRevokeMessage(QString::fromStdString(g_pMainPanel->getSelfUserId()), messageId, time);

    if (nullptr != g_pMainPanel)
    {
        ChatMsgManager::sendRevokeMessage(_pViewItem->_uid, g_pMainPanel->getSelfUserId(),
                                          messageId.toStdString(), _pViewItem->_chatType);
    }
}

void ChatMainWgt::onQuoteAct(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    //
//    QString userName = QString::fromStdString(itemWgt->_msgInfo.UserName);
    QTalk::Entity::JID jid(itemWgt->_msgInfo.from.toStdString());
    QString userName = QTalk::getUserNameNoMask(jid.basename()).data();
    QString source = itemWgt->_msgInfo.body;
    _pViewItem->_pInputWgt->insertQuote(userName, source);
    emit g_pMainPanel->sgOperator(tr("引用"));
}

void ChatMainWgt::onCollectionAct(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    //
    auto type = itemWgt->messageType();
    QString netPath;

    switch(type)
    {
        case QTalk::Entity::MessageTypeText:
        case QTalk::Entity::MessageTypeRobotAnswer:
        case QTalk::Entity::MessageTypeGroupAt:
            {
                auto *textItem = qobject_cast<TextMessItem *>(itemWgt);
                netPath = textItem->getImageLink();
                break;
            }

        case QTalk::Entity::MessageTypeImageNew:
        case QTalk::Entity::MessageTypePhoto:
            {
                auto *imgItem = qobject_cast<ImageMessItem *>(itemWgt);

                if(imgItem)
                    netPath = imgItem->_imageLink;

                break;
            }

        default:
            break;
    }

    QString name = QString::fromStdString(QTalk::GetFileNameByUrl(netPath.toStdString()));
    QString baseName = QFileInfo(name).baseName();
    //
    g_pMainPanel->addConllection(baseName, netPath);
}

void ChatMainWgt::onQRCodeAct(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    //
    auto type = itemWgt->messageType();
    QString netPath;

    switch(type)
    {
        case QTalk::Entity::MessageTypeText:
        case QTalk::Entity::MessageTypeRobotAnswer:
        case QTalk::Entity::MessageTypeGroupAt:
            {
                auto *textItem = qobject_cast<TextMessItem *>(itemWgt);
                netPath = textItem->getImageLink();
                break;
            }

        case QTalk::Entity::MessageTypeImageNew:
        case QTalk::Entity::MessageTypePhoto:
            {
                auto *imgItem = qobject_cast<ImageMessItem *>(itemWgt);
                netPath = imgItem->_imageLink;
                break;
            }

        default:
            break;
    }

    //
    std::string imageLocalFile = ChatMsgManager::getSouceImagePath(netPath.toStdString());

    if(!imageLocalFile.empty() && QFile::exists(imageLocalFile.data()) && g_pMainPanel)
        g_pMainPanel->scanQRcCodeImage(imageLocalFile.data());
}

void ChatMainWgt::recvBlackListMessage(const QString &messageId)
{
    if (_mapItemWgt.contains(messageId))
    {
        auto *item = _mapItemWgt[messageId];
        item->setReadState(MessageItemBase::EM_BLACK_RET);
        emit showTipMessageSignal(QTalk::utils::getMessageId().data(),
                                  QTalk::Entity::MessageTypeGroupNotify,
                                  tr("对方已将你的消息屏蔽, 消息已被拦截"),
                                  QDateTime::currentMSecsSinceEpoch() - PLAT.getServerDiffTime() * 1000);
    }
}

/**
 *
 */
void ChatMainWgt::onForwardAct(bool)
{
    auto *itemWgt = qobject_cast<MessageItemBase *>(indexWidget(currentIndex()));

    if (nullptr == itemWgt) return;

    auto stMsg = itemWgt->_msgInfo;
    g_pMainPanel->forwardMessage(stMsg.msg_id.toStdString());
    emit g_pMainPanel->sgOperator(tr("转发消息"));
}

/**
 *
 */
void ChatMainWgt::onScrollBarChanged(int val)
{
    {
        // 防止刷新ui时触发
        if(!_wheelFlag)
            return;
    }

    if (_oldScrollBarVal - val > 0)
    {
        if (val > 10)
        {
            _oldScrollBarVal = val;
            return;
        }

        QInt64 time = 0;
        int i = 0;

        if (_pModel->rowCount() > 0)
        {
            while (true)
            {
                time = _pModel->index(i, 0).data(EM_USER_MSG_TIME).toLongLong();

                if (time != 0)
                {
                    _jumIndex = _pModel->mapToSource(_pModel->index(i, 0));
                    break;
                }

                if (i == _pModel->rowCount() - 1)
                    break;

                i++;
            }
        }

        if(_request_time != time)
        {
//            auto msgType = _jumIndex.data(EM_USER_MSG_TYPE).toInt();
//            error_log("--- get min message time error {0} -> (his){1}. message info type:{2} str:{3}",
//                    time , _request_time, msgType);
            _jumIndex = {};
            time = _request_time;
        }

        if(_hasnotHistoryMsg)
        {
            if (time == LLONG_MAX)
                time = 0;

            g_pMainPanel->getHistoryMsg(time, _pViewItem->_chatType, _pViewItem->getPeerId());
        }
    }
    else
    {
        int maximum = this->verticalScrollBar()->maximum();

        if (_pNewMessageTipItem && maximum - val < 5)
            _pNewMessageTipItem->onResetWnd();
    }

    _oldScrollBarVal = val;
}

void ChatMainWgt::onAdjustItems(bool isOk)
{
    if(isOk)
    {
    }
    else
    {
        for (const auto &itemWgt : _mapItemWgt)
        {
            //
            if (itemWgt)
            {
                long long itemType = itemWgt->_msgInfo.msg_type;

                if (QTalk::Entity::MessageTypeText == itemType ||
                        QTalk::Entity::MessageTypeGroupAt == itemType ||
                        QTalk::Entity::MessageTypeRobotAnswer == itemType)
                {
                    auto *chatItem = qobject_cast<TextMessItem *>( itemWgt);

                    if(chatItem)
                        chatItem->setMessageContent(false);
                }
            }
        }

        // 重刷一下高度
        for(const auto &item : _mapItem)
            item->setData(Qt::UserRole, QDateTime::currentMSecsSinceEpoch());
    }

    _pAtMessageTipItem->showAtInfo();
    _pNewMessageTipItem->onResize();
    this->repaint();
}

void ChatMainWgt::setShareMessageState(bool flag)
{
    if(_selectEnable == flag)
        return;

    _selectEnable = flag;

    if(flag)
        this->setSelectionMode(QAbstractItemView::MultiSelection);
    else
    {
        this->clearSelection();
        this->setSelectionMode(QAbstractItemView::NoSelection);
    }

    for(const auto &item : _mapItemWgt)
    {
        item->showShareCheckBtn(flag);
        item->checkShareCheckBtn(false);
    }
}

void ChatMainWgt::onItemChanged()
{
    auto selectInxs = this->selectedIndexes();

    for(int row = 0; row < _pModel->rowCount(); row++)
    {
        auto index = _pModel->index(row, 0);
        auto *wgt = this->indexWidget(index);
        auto *base = qobject_cast<MessageItemBase *>(wgt);

        if(base)
            base->checkShareCheckBtn(selectInxs.contains(index));
    }

    emit sgSelectedSize(selectedIndexes().size());
}

void ChatMainWgt::onItemCheckChanged(bool)
{
    auto *base = qobject_cast<MessageItemBase *>(sender());

    if(base)
    {
        auto id = base->_msgInfo.msg_id;

        if(_mapItem.contains(id))
            selectionModel()->select(_pModel->mapFromSource(_mapItem[id]->index()), QItemSelectionModel::Toggle);
    }
}

void ChatMainWgt::selectionChanged(const QItemSelection &, const QItemSelection &)
{
    if(_selectItemQueue)
        _selectItemQueue->push(true);
}

/**
 *
 */
void ChatMainWgt::onShareMessage()
{
    if(selectedIndexes().empty())
    {
        QtMessageBox::warning(this, tr("警告"), tr("请选择需要转发的消息"));
        return;
    }

    std::vector<StNetMessageResult> arMsgs;
    //
    auto selects = selectedIndexes();
    auto it = selects.begin();

    for(; it != selects.end(); it++)
    {
        auto *wgt = qobject_cast<MessageItemBase *>(this->indexWidget(*it));

        if(wgt)
            arMsgs.push_back(wgt->_msgInfo);
    }

    if(arMsgs.empty())
    {
        QtMessageBox::warning(this, tr("警告"), tr("请选择有效的消息"));
        return;
    }

    //
    if(_pViewItem)
        _pViewItem->setShareMessageState(false);

    //
    auto id = _pViewItem->_uid.toQString();
    auto chatType = _pViewItem->_chatType;
    QT_CONCURRENT_FUNC([this, arMsgs, id, chatType]()
    {
        nJson objs;

        for(const auto &msg : arMsgs)
        {
            nJson obj;
            obj["d"] = msg.direction;

            if(msg.extend_info.isEmpty())
                obj["b"] = msg.body.toStdString().data();
            else
                obj["b"] = msg.extend_info.toStdString().data();

            obj["n"] = QTalk::getUserNameNoMask(msg.from.toStdString());
            obj["s"] = msg.time;
            obj["t"] = msg.msg_type;
            objs.push_back(obj);
        }

        std::string jsonMsg = objs.dump();
        QString jsonFilePath = QString("%1/json_msg").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        QFile tmpFile(jsonFilePath);

        if(tmpFile.open(QIODevice::WriteOnly))
        {
            tmpFile.resize(0);
            tmpFile.write(jsonMsg.data(), jsonMsg.size());
            tmpFile.close();
        }

        //
        if(g_pMainPanel)
        {
            std::string url = g_pMainPanel->uploadFile(jsonFilePath.toStdString());
            emit sgUploadShareMsgSuccess(id, chatType, QString::fromStdString(url));
        }
    });
}

bool ChatMainWgt::event(QEvent *e)
{
    if(e->type() == QEvent::Enter)
        this->verticalScrollBar()->setVisible(true);
    else if(e->type() == QEvent::Leave)
        this->verticalScrollBar()->setVisible(false);

    return QListView::event(e);
}

//
bool ChatMainWgt::eventFilter(QObject *o, QEvent *e)
{
    if(o == this->verticalScrollBar() && e->type() == QEvent::MouseButtonPress)
    {
        if(!_wheelFlag)
            _wheelFlag = true;
    }

    return QListView::eventFilter(o, e);
}

//
void ChatMainWgt::onUserMedalChanged(const std::set<std::string> &changedUser)
{
    auto items = _mapItemWgt.values();

    for(auto *wgt : items)
    {
        if(wgt && wgt->_msgInfo.direction == QTalk::Entity::MessageDirectionReceive)
        {
            const auto &id = wgt->_msgInfo.from.toStdString();

            if(changedUser.find(id) != changedUser.end())
                emit wgt->sgUpdateUserMedal();
        }
    }
}

/**
 *
 * @param link
 * @param msgid
 */
void ChatMainWgt::downloadImage(const QString &msgid, const QString &link, int, int )
{
    emit sgDownloadImageOperate(msgid, link);
}

void ChatMainWgt::onImageDownloaded(const QString &msgid, const QString &path)
{
    if(_mapItemWgt.contains(msgid))
    {
        auto *item = _mapItemWgt[msgid];
        {
            if( QTalk::Entity::MessageTypePhoto == item->messageType() ||
                    QTalk::Entity::MessageTypeImageNew == item->messageType())
            {
                auto *imgWgtItem = qobject_cast<ImageMessItem *>(item);

                if(imgWgtItem)
                    imgWgtItem->onImageDownloaded();
                else
                {
                    auto *emoWgtItem = qobject_cast<EmojiMessItem *>(item);

                    if(emoWgtItem)
                        emoWgtItem->onImageDownloaded(path);
                }
            }
            else if(QTalk::Entity::MessageTypeRobotAnswerList == item->messageType())
            {
                auto *ansItem = qobject_cast<HotLineAnswerItem *>(item);

                if(ansItem)
                    ansItem->onImageDownloaded(path);
            }
            // path 为下载地址
            else /*if(QTalk::Entity::MessageTypeText == item->messageType())*/
            {
                auto *textWgtItem = qobject_cast<TextMessItem *>(item);

                if(textWgtItem)
                    textWgtItem->onImageDownloaded(path);
            }
        }
//        _resizeQueue->push(true);
    }

    if(_mapItem.contains(msgid))
    {
        auto *item = _mapItem[msgid];
        StNetMessageResult info = item->data(EM_USER_INFO).value<StNetMessageResult>();

        for(auto &&it : info.text_messages)
        {
            if(it.type == StTextMessage::EM_IMAGE && it.imageLink == path)
            {
                it.content = QTalk::GetImagePathByUrl(path.toStdString()).data();
                QTalk::Image::scaImageSizeByPath(it.content, it.imageWidth, it.imageHeight);
            }
            else if(it.type == StTextMessage::EM_EMOTICON)
            {
                QFileInfo fileInfo(path);

                if(fileInfo.baseName() == QString("%1_%2").arg(it.pkgid, it.shortCut))
                {
                    it.content = path;
                    QTalk::Image::scaImageSizeByPath(it.content, it.imageWidth, it.imageHeight);
                }
            }
        }

        item->setData(QVariant::fromValue(info), EM_USER_INFO);
    }
}

/**
 *
 * @param pkgid
 * @param shortCut
 */
void ChatMainWgt::downloadEmoticon(const QString &msgId, const QString &pkgid, const QString &shortCut)
{
    emit sgDownloadEmoOperate(msgId, pkgid, shortCut);
}

void ChatMainWgt::onDisconnected()
{
    setConnectState(false);

    //
    for(auto *wgt : _mapItemWgt)
    {
        if(wgt)
            wgt->onDisconnected();
    }
}

void ChatMainWgt::onSendMessageFailed(const QString &msgId)
{
    if(_mapItemWgt.contains(msgId))
    {
        auto *wgt = _mapItemWgt[msgId];
        wgt->onDisconnected();
        auto *fileWgt = qobject_cast<FileSendReceiveMessItem *>(wgt);

        if(fileWgt)
            fileWgt->onUploadFailed();
    }
}

void ChatMainWgt::onDownloadFileFailed(const QString &msgId)
{
    if(_mapItemWgt.contains(msgId))
    {
        auto *wgt = _mapItemWgt[msgId];
        auto *fileWgt = qobject_cast<FileSendReceiveMessItem *>(wgt);

        if(fileWgt)
            fileWgt->onDownloadFailed();
    }
}

void ChatMainWgt::clearData()
{
    _hasnotHistoryMsg = true;
    _jumIndex = QModelIndex();
    _oldScrollBarVal = 0;
    _times.clear();
    _mapItemWgt.clear();
    _mapItem.clear();
    _pAtMessageTipItem->clear();
    _pSrcModel->clear();
}

//
void ChatMainWgt::onMState(const QString &msgId, const long long &time)
{
    if(_mapItem.contains(msgId) && _mapItem[msgId])
    {
        StNetMessageResult info = _mapItem[msgId]->data(EM_USER_INFO).value<StNetMessageResult>();
        info.state = 1;
        info.read_flag = 1;
        info.time = time;
        _mapItem[msgId]->setData(QVariant::fromValue(info), EM_USER_INFO);
        _mapItem[msgId]->setData(time, EM_USER_MSG_TIME);

        if(_mapTimeItem.contains(msgId))
            _mapTimeItem[msgId]->setData(time, EM_USER_MSG_TIME);

        _pModel->sort(0);
        this->scrollToBottom();
    }

    if(_mapItemWgt.contains(msgId))
    {
        if(_mapItemWgt[msgId])
            _mapItemWgt[msgId]->setReadState(1);
    }
    else
        g_pMainPanel->eraseSending(msgId.toStdString());
}

void ChatMainWgt::jumTo()
{
    if(_isLeave)
        return;

    switch (_jumType)
    {
        case EM_JUM_BOTTOM:
            {
                this->scrollToBottom();
                break;
            }

        case EM_JUM_ITEM:
            {
                if(_jumIndex.isValid())
                    this->scrollTo(_pModel->mapFromSource(_jumIndex), PositionAtTop);

                break;
            }

        case EM_JUM_ITEM_BOTTOM:
            {
                if(_reset_jumIndex.isValid())
                    this->scrollTo(_pModel->mapFromSource(_reset_jumIndex), PositionAtBottom);

                break;
            }

        case EM_JUM_TOP:
            {
                this->scrollToTop();
                break;
            }

        case EM_JUM_INVALID:
        default:
            {
                int scrollBarVal = this->verticalScrollBar()->value();
                int maximum = this->verticalScrollBar()->maximum();

                if(maximum != 0 && maximum - scrollBarVal <= 220)
                    this->scrollToBottom();

                break;
            }
    }

    _jumType = EM_JUM_INVALID;
}

void ChatMainWgt::showEvent(QShowEvent *e)
{
//    _jumType = EM_JUM_BOTTOM;
    _isLeave = false;
//    _oldScrollBarVal = 0;
//    {
//        static unsigned char flag = 0;
//        if(++flag % 2)
//            this->resize(this->width() + 1, this->height());
//        else
//            this->resize(this->width() - 1, this->height());
//    }
    _pNewMessageTipItem->onResetWnd();
    QWidget::showEvent(e);
}

//
void ChatMainWgt::scrollToItem(QStandardItem *pItem)
{
    if(pItem && _mapItem.values().contains(pItem))
        this->scrollTo(_pModel->mapFromSource(pItem->index()), QAbstractItemView::PositionAtTop);
}

//
void ChatMainWgt::onDownloadFile(const QString &key)
{
    if(!_mapItemWgt.contains(key))
        return;

    auto *item = _mapItemWgt[key];
    {
        auto *pFileItem = qobject_cast<FileSendReceiveMessItem *>(item);

        if(pFileItem)
            pFileItem->downloadOrUploadSuccess();
    }
    //
    {
        auto *pVideoItem = qobject_cast<VideoMessageItem *>(item);

        if(pVideoItem)
            pVideoItem->downloadSuccess();
    }
}

//
void ChatMainWgt::freeView()
{
//    this->setItemDelegate(nullptr);
//    _reset_jumIndex = _pModel->mapToSource(_pModel->index(_pModel->rowCount() - 1, 0));
    _isLeave = true;
    _wheelFlag = false;
    _jumIndex = {};
    _reset_jumIndex = {};

    if(_pSrcModel->rowCount() > SWITCH_SAVE_MESSAGE_NUM)
    {
        _hasnotHistoryMsg = true;
        std::list<qint64> allTimes ;

        for(auto idx = 0; idx < _pSrcModel->rowCount(); idx ++)
        {
            auto t = _pSrcModel->index(idx, 0).data(EM_USER_MSG_TIME).toULongLong();
            allTimes.push_back(t);
        }

        int delSize = (int)allTimes.size() - SWITCH_SAVE_MESSAGE_NUM;
        delSize = qMin(SWITCH_DELETE_MESSAGE_MAX_NUM, delSize);
        auto index = 0;
        allTimes.sort();

        while(!allTimes.empty() && index < delSize)
        {
            allTimes.erase(allTimes.begin());
            index++;
        }

        qint64 minTime = *allTimes.begin();
        _request_time = minTime;
        std::vector<QString> delIds;

        for(auto it : _mapItem)
        {
            auto time = it->data(EM_USER_MSG_TIME).toULongLong();
            StNetMessageResult info = it->data(EM_USER_INFO).value<StNetMessageResult>();

            if(time < minTime)
                delIds.push_back(info.msg_id);
        }

        for(const auto &id : delIds)
        {
            if (_mapItem.contains(id))
            {
                auto *item = _mapItem[id];
                _pSrcModel->removeRow(item->row());
                _mapItem.remove(id);
            }

//            _mapItemWgt;
            _mapItemWgt.remove(id);

            if(_mapTimeItem.contains(id))
            {
                auto *timeItem = _mapTimeItem[id];
                _mapTimeItem.remove(id);
                auto time = timeItem->data(EM_USER_MSG_TIME).toLongLong();

                if(_times.contains(time))
                    _times.remove(time);

                _pSrcModel->removeRow(timeItem->row());
            }
        }
    }
}

//
void ChatMainWgt::onUploadFileSuccess(const QString &key, const QString & )
{
    auto *item = _mapItemWgt[key];
    {
        auto *pFileItem = qobject_cast<FileSendReceiveMessItem *>(item);

        if(pFileItem)
            pFileItem->downloadOrUploadSuccess();
    }
}

/** DownloadImageTask  */
void DownloadImageTask::downloadImage(const QString &msgId, const QString &link)
{
    auto path = ChatMsgManager::getLocalFilePath(link.toStdString());
    emit downloadFinish(msgId, link);
}

void DownloadImageTask::downloadEmo(const QString &msgId, const QString &pkgid, const QString &shortCut)
{
    auto localPath = EmoticonMainWgt::instance()->downloadEmoticon(pkgid, shortCut);
    emit downloadFinish(msgId, localPath);
}
