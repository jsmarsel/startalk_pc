//
// Created by cc on 2019-06-25.
//

#include "LocalSearchMainWgt.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QScrollBar>
#include <QDateTime>
#include <QFileInfo>
#include <QHeaderView>
#include <QEvent>
#include <QWheelEvent>
#include <QtConcurrent>
#include "../ChatUtil.h"
#include "../ChatViewMainPanel.h"
#include "../../CustomUi/TitleBar.h"
#include "MessageAnalysis.h"
#include "MessageDelegate.h"
#include "ImageDelegate.h"

extern ChatViewMainPanel *g_pMainPanel;

LocalSearchMainWgt::LocalSearchMainWgt(QWidget* parent)
    : UShadowDialog(parent, true)
    , _pAllListWgt(nullptr)
    , _pFileListWgt(nullptr)
    , _lazyQueue(nullptr)
{
    initUi();
    initList();
    setWindowFlags(this->windowFlags() | Qt::Tool);
    connect(this, &LocalSearchMainWgt::sgGetBeforeAllMessage,
            this, &LocalSearchMainWgt::onGetBeforeAllMessage, Qt::QueuedConnection);

    connect(this, &LocalSearchMainWgt::sgPositionMessage,
            this, &LocalSearchMainWgt::onPositionMessage, Qt::QueuedConnection);

    connect(this, &LocalSearchMainWgt::sgSearch,
            this, &LocalSearchMainWgt::onSearch, Qt::QueuedConnection);
//
    connect(this, &LocalSearchMainWgt::sgGetBeforeFileMessage,
            this, &LocalSearchMainWgt::onGetBeforeFileMessage, Qt::QueuedConnection);

    connect(this, &LocalSearchMainWgt::sgGetImageMessage,
            this, &LocalSearchMainWgt::onGetImageMessage, Qt::QueuedConnection);
    connect(this, &LocalSearchMainWgt::sgGetLinkMessage,
            this, &LocalSearchMainWgt::onGetLinkMessage, Qt::QueuedConnection);

    std::function<int(STLazyQueue<std::string> *)> searchFun =

            [this](STLazyQueue<std::string> *queue) -> int {

                int runningCount = 0;
                if (queue != nullptr && !queue->empty()) {

                    std::string searKey = queue->tail();
                    while (!queue->empty()) {
                        queue->pop();
                        runningCount++;
                    }
                    emit sgSearch(0, searKey.data());
                }

                return runningCount;
            };

    _lazyQueue = new STLazyQueue<std::string>(200, searchFun);


}

LocalSearchMainWgt::~LocalSearchMainWgt()
{
    if(_lazyQueue)
        delete _lazyQueue;
}

void LocalSearchMainWgt::initStyle(const QString& qss)
{
    Q_INIT_RESOURCE(image1);
    this->setStyleSheet(qss.toUtf8());
}

void LocalSearchMainWgt::initUi()
{
    setFixedWidth(360);
    setFixedHeight(g_pMainPanel->height());
    _pTitleLabel = new TitleBar(tr(""), nullptr, this);
    _pTitleLabel->setFixedHeight(50);
    connect(_pTitleLabel->getCloseBtn(), &QToolButton::clicked, [this](){this->close();});
    setMoverAble(true, _pTitleLabel);
    //
    _searEdit = new Search_Edit(this);
    _searEdit->setFixedHeight(30);
    _pCalendarBtn = new QPushButton(this);
    _pCalendarBtn->setFixedSize(30, 30);
    _pCalendarBtn->setObjectName("CalendarBtn");
    _calendarWgt = new QCalendarWidget(this);
    _calendarWgt->setWindowFlags(this->windowFlags() | Qt::Popup);
    _calendarWgt->setAttribute(Qt::WA_QuitOnClose, false);
    auto* toolFrm = new QFrame(this);
    toolFrm->setObjectName("toolFrm");
    toolFrm->setFixedHeight(40);
    auto* toolLay = new QHBoxLayout(toolFrm);
    toolLay->setContentsMargins(14, 0, 14, 0);
    toolLay->addWidget(_searEdit, 1);
    toolLay->addWidget(_pCalendarBtn, 0);
    //
    auto* allBtn = new QPushButton(tr("全部"), this);
    auto* imgBtn = new QPushButton(tr("图片"), this);
    auto* fileBtn = new QPushButton(tr("文件"), this);
    auto* linkBtn = new QPushButton(tr("链接"), this);
    allBtn->setObjectName("SearchWgtBtn");
    imgBtn->setObjectName("SearchWgtBtn");
    fileBtn->setObjectName("SearchWgtBtn");
    linkBtn->setObjectName("SearchWgtBtn");
    allBtn->setCheckable(true);
    imgBtn->setCheckable(true);
    fileBtn->setCheckable(true);
    linkBtn->setCheckable(true);
    _btnGroup = new QButtonGroup(this);
    _btnGroup->addButton(allBtn, EM_ALL);
    _btnGroup->addButton(imgBtn, EM_IMG);
    _btnGroup->addButton(fileBtn, EM_FILE);
    _btnGroup->addButton(linkBtn, EM_LINK);
    allBtn->setChecked(true);
    _btnFrm = new QFrame(this);
    _btnFrm->setObjectName("btnFrm");
    auto* btnLay = new QHBoxLayout(_btnFrm);
    btnLay->setSpacing(30);
    btnLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    btnLay->addWidget(allBtn);
    btnLay->addWidget(imgBtn);
    btnLay->addWidget(fileBtn);
    btnLay->addWidget(linkBtn);
    btnLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    //
    _pSearchCountLabel = new QLabel(this);
    _pSearchCountLabel->setVisible(false);
    _pSearchCountLabel->setFixedHeight(46);
    _pSearchCountLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    //
    _pStackedWidget = new QStackedWidget(this);
    auto* bodyFrm = new QFrame(this);
    bodyFrm->setObjectName("LocalSearchBodyFrm");
    auto* bodyLay = new QHBoxLayout(bodyFrm);
    bodyLay->setContentsMargins(0, 0, 0, 10);
    bodyLay->addWidget(_pStackedWidget);
    //
    _pLoadingContent = QTalk::makeLoadingLabel(false, {0, 0}, this);
    _pLoadingMoreR_T = QTalk::makeLoadingLabel(true, {50, 50}, this);
    _pLoadingMoreR_B = QTalk::makeLoadingLabel(true, {50, 50}, this);
    _pStackedWidget->addWidget(_pLoadingContent);
    _pLoadingMoreR_T->setVisible(false);
    _pLoadingMoreR_B->setVisible(false);
    //
    auto* mainFrm = new QFrame(this);
    mainFrm->setObjectName("LocalSearchMainFrm");
    auto* mainLay = new QVBoxLayout(mainFrm);
    mainLay->setMargin(0);
    mainLay->setSpacing(0);
    mainLay->addWidget(_pTitleLabel, 0);
    mainLay->addWidget(toolFrm, 0);
    mainLay->addWidget(_btnFrm, 0);
    mainLay->addWidget(_pSearchCountLabel, 0);
    mainLay->addWidget(new Line(this));
    mainLay->addWidget(_pLoadingMoreR_T);
    mainLay->addWidget(bodyFrm, 1);
    mainLay->addWidget(_pLoadingMoreR_B);
    //
    auto* lay = new QVBoxLayout(_pCenternWgt);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(mainFrm);
    //
    connect(_btnGroup, SIGNAL(buttonClicked(int)), this, SLOT(onButtonClicked(int)));
    qRegisterMetaType<StData>("StData");
    //
    connect(_searEdit, &Search_Edit::textChanged, [this](const QString& text){
        _lazyQueue->push(text.toLower().toStdString());
    });

    connect(this, &LocalSearchMainWgt::sgSearch, this, &LocalSearchMainWgt::starSearch);

    connect(_pCalendarBtn, &QPushButton::clicked, [this](){
        if(_calendarWgt)
        {
            QPoint pos = this->geometry().topRight();
            _calendarWgt->setVisible(true);
            _calendarWgt->move(pos.x() - _calendarWgt->width() - 20, pos.y() + 100);
            _calendarWgt->raise();
        }
    });
    connect(_calendarWgt, &QCalendarWidget::clicked, [this](const QDate &date){
        auto time = QDateTime(date).toMSecsSinceEpoch();
        emit sgPositionMessage(time, true);
        _calendarWgt->setVisible(false);
    });
    connect(this, &LocalSearchMainWgt::sgUpdateName, this, &LocalSearchMainWgt::onSetName, Qt::QueuedConnection);
//    connect(this, &LocalSearchMainWgt::sgInitStyle, this, &LocalSearchMainWgt::initStyle, Qt::QueuedConnection);
}

//
int dealWheel(QAbstractItemView *wgt, QWheelEvent* evt)
{
    static qint64 _historyTime = 0;
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();
    if(curTime - _historyTime < 500)
        return 0;
    else
        _historyTime = curTime;

    int scroBarval = wgt->verticalScrollBar()->value();
    int maximum = wgt->verticalScrollBar()->maximum();
    int direct = evt->angleDelta().y();

    if(scroBarval < 5 && direct > 0)
        return 1;
    else if(scroBarval > maximum -5 && direct < 0)
        return 2;

    return 0;
}

/**
 *
 * @param o
 * @param e
 * @return
 */
bool LocalSearchMainWgt::eventFilter(QObject *o, QEvent *e) {

    if(o == _pAllListWgt)
    {
        if(QEvent::Wheel == e->type())
        {
            auto* evt = (QWheelEvent*)e;
            int ret = dealWheel(_pAllListWgt, evt);
            if((ret == 1) && _hasBefore[EM_ALL])
            {
                QModelIndex index = _pAllSortModel->index(0, 0);
                if(index.isValid())
                {
                    auto time = index.data(SEARCH_USER_MSG_TIME).toLongLong();
                    emit sgGetBeforeAllMessage(time);
                }
            }
            else if(ret == 2)
            {
                if(_hasNew[EM_ALL])
                {
                    QModelIndex index = _pAllSortModel->index(_pAllSortModel->rowCount() - 1, 0);
                    if(index.isValid())
                    {
                        _pLoadingMoreR_B->movie()->start();
                        _pLoadingMoreR_B->setVisible(true);

                        auto time = index.data(SEARCH_USER_MSG_TIME).toLongLong();
                        emit sgPositionMessage(time + 1, false);
                    }
                }
            } else {}
        }
    }
    else if(o == _pFileListWgt)
    {
        if(QEvent::Wheel == e->type())
        {
            auto* evt = (QWheelEvent*)e;
            if(_hasBefore[EM_FILE] && dealWheel(_pFileListWgt, evt) == 1)
            {
                QModelIndex index = _pFileSortModel->index(0, 0);
                if(index.isValid())
                {
                    auto time = index.data(SEARCH_USER_MSG_TIME).toLongLong();
                    emit sgGetBeforeFileMessage(time);
                }
            }
        }
    }
    else if(o == _pImageView)
    {
        if(QEvent::Wheel == e->type())
        {
            auto* evt = (QWheelEvent*)e;
            if(_hasBefore[EM_IMG])
            {
                if(dealWheel(_pImageView, evt) == 1)
                {
                    QModelIndex index = _pImageModel->index(0, 1);
                    if(index.isValid())
                    {
                        auto time = index.data(SEARCH_USER_MSG_TIME).toLongLong();
                        emit sgGetImageMessage(time);
                    }
                }
            }
        }
    }
    return QDialog::eventFilter(o, e);
}

/**
 *
 */
void LocalSearchMainWgt::initList()
{
    // all
    _pAllListWgt = new QListView(this);
    _pAllListWgt->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pAllListWgt->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pAllListWgt->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pAllListWgt->verticalScrollBar()->setSingleStep(10);
    _pAllListWgt->setFrameShape(QFrame::NoFrame);
    _pAllListWgt->setAutoScroll(false);
    _pAllLstModel = new QStandardItemModel(this);
    _pAllSortModel = new MessageSortModel(this);
    _pAllSortModel->setSourceModel(_pAllLstModel);
    _pAllListWgt->setModel(_pAllSortModel);
    auto* messageDelegate = new MessageDelegate(_pAllListWgt);
    _pAllListWgt->setItemDelegate(messageDelegate);
    _pStackedWidget->addWidget(_pAllListWgt);
    _pStackedWidget->setCurrentWidget(_pAllListWgt);

    // file
    _pFileListWgt = new QListView(this);
    _pFileListWgt->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pFileListWgt->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pFileListWgt->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pFileListWgt->verticalScrollBar()->setSingleStep(10);
    _pFileListWgt->setFrameShape(QFrame::NoFrame);
    _pFileListWgt->setAutoScroll(false);
    _pFileModel = new QStandardItemModel(this);
    _pFileSortModel = new MessageSortModel(this);
    _pFileSortModel->setSourceModel(_pFileModel);
    _pFileListWgt->setModel(_pFileSortModel);
    auto* fileDelegate = new MessageDelegate(_pFileListWgt);
    _pFileListWgt->setItemDelegate(fileDelegate);
    _pStackedWidget->addWidget(_pFileListWgt);

    // search
    _pSearchListWgt = new SearchUserView(this);
    _pSearchListWgt->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pSearchListWgt->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pSearchListWgt->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pSearchListWgt->verticalScrollBar()->setSingleStep(10);
    _pSearchListWgt->setFrameShape(QFrame::NoFrame);
    _pSearchListWgt->setAutoScroll(false);
    _pSearchModel = new QStandardItemModel(this);
    _pSearchSortModel = new MessageSortModel(this);
    _pSearchSortModel->setSourceModel(_pSearchModel);
    _pSearchListWgt->setModel(_pSearchSortModel);
    auto* searchDelegate = new MessageDelegate(_pSearchListWgt);
    _pSearchListWgt->setItemDelegate(searchDelegate);
    _pStackedWidget->addWidget(_pSearchListWgt);
    // image
    _pImageView = new QTableView(this);
    _pImageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pImageView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pImageView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pImageView->verticalScrollBar()->setSingleStep(10);
    _pImageView->setFrameShape(QFrame::NoFrame);
    _pImageView->setAutoScroll(false);
    _pImageView->setSelectionMode(QAbstractItemView::SingleSelection);
    _pImageView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _pImageView->verticalHeader()->setMinimumSectionSize(20);
    _pImageView->horizontalHeader()->setMinimumSectionSize(20);
    _pImageView->horizontalHeader()->setVisible(false);
    _pImageView->verticalHeader()->setVisible(false);
    _pImageView->setShowGrid(false);
    _pImageView->setFocusPolicy(Qt::NoFocus);
    _pImageModel = new QStandardItemModel(this);
    _pImageModel->setColumnCount(3);
    _pImageView->setModel(_pImageModel);
    auto* imageDelegate = new ImageDelegate(_pImageView);
    _pImageView->setItemDelegate(imageDelegate);
    _pStackedWidget->addWidget(_pImageView);

    connect(searchDelegate, &MessageDelegate::sgGetMessageDetail, [this](qint64 time, const QString&, int){
        emit sgPositionMessage(time, true);
    });
    // link
    _pLinkListView = new QListView(this);
    _pLinkListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _pLinkListView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _pLinkListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _pLinkListView->verticalScrollBar()->setSingleStep(10);
    _pLinkListView->setFrameShape(QFrame::NoFrame);
    _pLinkListView->setAutoScroll(false);
    _pLinkModel = new QStandardItemModel(this);
    _pLinkSortModel = new MessageSortModel(this);
    _pLinkSortModel->setSourceModel(_pLinkModel);
    _pLinkListView->setModel(_pLinkSortModel);
    auto* linkDelegate = new MessageDelegate(_pLinkListView);
    _pLinkListView->setItemDelegate(linkDelegate);
    _pStackedWidget->addWidget(_pLinkListView);

    // event
    _pAllListWgt->installEventFilter(this);
    _pFileListWgt->installEventFilter(this);
    _pSearchListWgt->installEventFilter(this);
    _pImageView->installEventFilter(this);
}

void LocalSearchMainWgt::onButtonClicked(int id)
{
    switch(id)
    {
        case EM_ALL:
            _pStackedWidget->setCurrentWidget(_pAllListWgt);
            break;
        case EM_IMG:
            if(_pImageModel->rowCount() == 0)
                onGetImageMessage(0);
            else
                _pStackedWidget->setCurrentWidget(_pImageView);
            break;
        case EM_FILE:
        {
            if(_pFileModel->rowCount() == 0)
                onGetBeforeFileMessage(0);
            else
                _pStackedWidget->setCurrentWidget(_pFileListWgt);
            break;
        }
        case EM_LINK:
        {
            if(_pLinkModel->rowCount() == 0)
                onGetLinkMessage(0);
            else
                _pStackedWidget->setCurrentWidget(_pLinkListView);
            break;
        }
        case EM_SEARCH:
        default:
            break;
    }
}

QString getLocalFilePath(const QTalk::Entity::ImMessageInfo &msgInfo)
{
    QString fullFileName = QString::fromStdString(g_pMainPanel->getFileMsgLocalPath(msgInfo.MsgId));
    if(fullFileName.isEmpty() || !QFile::exists(fullFileName))
    {
        QString linkFile = g_pMainPanel->getFileLink(msgInfo.FileMd5.data());
#ifdef _WINDOWS
        linkFile.append(".lnk");
#endif
        QFileInfo linkFileInfo(linkFile);
        if (linkFileInfo.exists() && !linkFileInfo.canonicalFilePath().isEmpty())
#ifdef _WINDOWS
            return linkFileInfo.symLinkTarget();
#else
            return linkFileInfo.canonicalFilePath();
#endif // _WINDOWS
        else
            return QString();
    }
    else
    {
        return fullFileName;
    }
}

/**
 * @param msg
 */
void LocalSearchMainWgt::showAllMessage(const QTalk::Entity::UID& uid)
{
    _uid = uid;
    _btnGroup->button(EM_ALL)->click();
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    //
    dealAllMessages(true,
            std::bind(&ChatMsgManager::getUserLocalHistoryMessage, 0, uid));
    _hasNew[EM_ALL] = false;
    //
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->stop();
        _pStackedWidget->setCurrentWidget(_pAllListWgt);
    }
}

bool LocalSearchMainWgt::dealMessages(const std::function<std::vector<QTalk::Entity::ImMessageInfo>()> &func,
                                      QStandardItemModel *model, QSortFilterProxyModel* sortModel) {

    std::vector<StNetMessageResult> _messages;
    auto future =  QtConcurrent::run([ func, &_messages](){

        auto allMessage = func();

        for(const auto& message : allMessage)
        {
            if(message.Type == INT_MIN)
                continue;
            StNetMessageResult info;
            info.msg_id = message.MsgId.data();
            info.msg_type = message.Type;
            info.type = message.ChatType ;
            info.time = message.LastUpdateTime;
            info.from = message.From.data();
            info.body = message.Content.data();
            info.extend_info = message.ExtendedInfo.data();
            info.xmpp_id = message.XmppId.data();
            QTalk::analysisMessage(info);
            _messages.push_back(info);
        }
    });

//    future.waitForFinished();
    while (!future.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);
    // set new data
    for(const auto& info : _messages)
    {
        auto* item = new QStandardItem();
        item->setData(info.type, SEARCH_USER_TYPE);
        item->setData(info.msg_type, SEARCH_USER_MSG_TYPE);
        item->setData(info.time, SEARCH_USER_MSG_TIME);
        item->setData(QVariant::fromValue(info), SEARCH_USER_INFO);

        model->appendRow(item);
    }
    sortModel->sort(0);
    return _messages.empty();
}

//
void LocalSearchMainWgt::dealAllMessages(bool up,
        const std::function<std::vector<QTalk::Entity::ImMessageInfo>()>& func) {

    bool isEmpty = dealMessages(func, _pAllLstModel, _pAllSortModel);
    if(isEmpty)
    {
        if(up)
            _hasBefore[EM_ALL] = false;
        else
            _hasNew[EM_ALL] = false;
    }
}

//
void LocalSearchMainWgt::dealSearchMessages(const std::function<std::vector<QTalk::Entity::ImMessageInfo>()>& func) {

    bool isEmpty = dealMessages(func, _pSearchModel, _pSearchSortModel);
    if(isEmpty)
        _hasNew[EM_SEARCH] = false;
}

//
void LocalSearchMainWgt::dealFileMessages(const std::function<std::vector<QTalk::Entity::ImMessageInfo>()>& func) {

    bool isEmpty = dealMessages(func, _pFileModel, _pFileSortModel);
    if(isEmpty)
        _hasBefore[EM_FILE] = false;
}

//
void LocalSearchMainWgt::onGetBeforeAllMessage(qint64 time)
{
    if(_pLoadingMoreR_T)
    {
        _pLoadingMoreR_T->movie()->start();
        _pLoadingMoreR_T->setVisible(true);
    }
    dealAllMessages(true, std::bind(&ChatMsgManager::getUserLocalHistoryMessage, time, _uid));
    //
    if(_pLoadingMoreR_T)
    {
        _pLoadingMoreR_T->movie()->stop();
        _pLoadingMoreR_T->setVisible(false);
    }
    //
    auto index = _pAllSortModel->index(0, 0);
    _pAllListWgt->scrollTo(index, QAbstractItemView::PositionAtTop);
}

//
void LocalSearchMainWgt::onPositionMessage(qint64 time, bool clearData)
{
    if(clearData)
    {
        _pAllLstModel->clear();
        _searEdit->clear();
        //
        _hasBefore[EM_ALL] = true;
        _hasNew[EM_ALL] = true;

        _btnFrm->setVisible(true);
        _pSearchCountLabel->setVisible(false);
        _btnGroup->button(EM_ALL)->click();
    }

    if(clearData && _pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    //
    dealAllMessages(false, std::bind(&ChatMsgManager::getAfterMessage, time, _uid));
    //
    if(clearData && _pLoadingContent)
    {
        _pLoadingContent->movie()->stop();
    }
    _pStackedWidget->setCurrentWidget(_pAllListWgt);
    if(_pLoadingMoreR_B)
    {
        _pLoadingMoreR_B->movie()->stop();
        _pLoadingMoreR_B->setVisible(false);
    }
    //
    if(clearData)
    {
        auto index = _pAllSortModel->index(0, 0);
        _pAllListWgt->scrollTo(index, QAbstractItemView::PositionAtTop);
    }
}

/**
 *
 */
void LocalSearchMainWgt::resetUi()
{
    _pAllLstModel->clear();
    _pSearchModel->clear();
    _pImageModel->clear();
    _pFileModel->clear();
    _pLinkModel->clear();

    QMutexLocker locker(&_mutex);

    _hasBefore[EM_ALL] = true;
    _hasBefore[EM_FILE] = true;
    _hasBefore[EM_IMG] = true;
    _hasNew[EM_ALL] = true;
}

void setSearchLabel(QLabel* label, int count)
{
    label->setText(QObject::tr("共找到<span style='color:rgba(0,202,190,1)'> %1 </span>条相关消息").arg(count));
}

/**
 *
 * @param time
 */
void LocalSearchMainWgt::onSearch(qint64 time, const QString &text)
{
    if(text.isEmpty())
        return;

    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    //
    dealSearchMessages(std::bind(&ChatMsgManager::getSearchMessage,
            time, _uid, text.toLower().toStdString()));
    //
    if(_pLoadingContent)
    {
        setSearchLabel(_pSearchCountLabel, _pSearchModel->rowCount());
        _pLoadingContent->movie()->stop();
        _pStackedWidget->setCurrentWidget(_pSearchListWgt);
    }
    auto index = _pSearchSortModel->index(0, 0);
    _pSearchListWgt->scrollTo(index, QAbstractItemView::PositionAtTop);
}

void LocalSearchMainWgt::starSearch(qint64, const QString& key)
{
    bool isEmpty = key.isEmpty();
    _btnFrm->setVisible(isEmpty);
    _pSearchCountLabel->setVisible(!isEmpty);

    if(isEmpty)
        _btnGroup->button(EM_ALL)->click();
    else
    {
        _pSearchModel->clear();
        _hasNew[EM_SEARCH] = false;
        setSearchLabel(_pSearchCountLabel, 0);
    }
}

void LocalSearchMainWgt::onSetName(const QString& name) {
    QFontMetricsF uf(_pTitleLabel->font());
    QString userName = uf.elidedText(name, Qt::ElideRight, 200);
    _pTitleLabel->setLabelText(name);
}

void LocalSearchMainWgt::onGetBeforeFileMessage(qint64 time)
{
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    //
    QModelIndex index{};
    if(time != 0 && _pFileModel->rowCount() > 0)
        index = _pFileSortModel->index(0, 0);
    //
    dealFileMessages(std::bind(&ChatMsgManager::getUserFileHistoryMessage,
                                 time, _uid));
    //
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->stop();
        _pStackedWidget->setCurrentWidget(_pFileListWgt);
    }
    if(time == 0)
        _pFileListWgt->scrollToBottom();
    else if(index.isValid())
        _pFileListWgt->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

//
void LocalSearchMainWgt::onGetImageMessage(qint64 time) {

    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    //
    std::vector<StImageResult> _messages;
    auto future =  QtConcurrent::run([this, time, &_messages](){

        auto allMessage = ChatMsgManager::getUserImageHistoryMessage(time, _uid);

        for(const auto& message : allMessage)
        {
            if(message.Type == INT_MIN)
                continue;

            QStringList imageLinks;
            QTalk::analysisLocalImage(message.Content.data(), imageLinks);

            int index = 0;
            for(const auto& link : imageLinks)
            {
                StImageResult info;
                info.msg_id = message.MsgId.data();
                info.time = message.LastUpdateTime;
                info.from = message.From.data();
                info.imageLink = link;
                info.content = message.Content.data();
                info.index = index++;
                _messages.push_back(info);
            }
        }
    });

    // wait
//    future.waitForFinished();
    while (!future.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);
    if(_messages.empty())
    {
        _hasBefore[EM_IMG] = false;
    }
    else
    {
        // set new data
        int col = 0, row = 0;
        qint64 tmp_time = 0;
        auto it = _messages.begin();
        while (it != _messages.end())
        {
            QList<QStandardItem*> items;
            while (it != _messages.end())
            {
                if( it->time - tmp_time > 24 * 60 * 60 * 1000)
                {
                    auto dateTime = QDateTime::fromMSecsSinceEpoch(it->time);
                    dateTime = QDateTime(dateTime.date());
                    tmp_time = dateTime.toMSecsSinceEpoch();
                    auto* item = new QStandardItem();
                    item->setData(it->time, SEARCH_USER_MSG_TIME);
                    item->setData(1, SEARCH_USER_ITEM_TYPE);
                    item->setText(dateTime.toString("yyyy-MM-dd"));
                    if(!items.empty())
                    {
                        col = 0;
                        _pImageModel->insertRow(row++, items);
                        items.clear();
                    }
                    _pImageView->setRowHeight(row, 50);
                    _pImageModel->insertRow(row++, QList<QStandardItem*>() << new QStandardItem << item);
                }

                auto* item = new QStandardItem();
                item->setData(it->time, SEARCH_USER_MSG_TIME);
                item->setData(2, SEARCH_USER_ITEM_TYPE);
                item->setData(QVariant::fromValue(*it), SEARCH_USER_INFO);
                item->setToolTip(QDateTime::fromMSecsSinceEpoch(it->time).toString("yyyy-MM-dd hh:mm"));
                items.append(item);
                it++;
                if(++col == 3)
                {
                    col = 0;
                    break;
                }
            }
            _pImageView->setRowHeight(row, 105);
            _pImageModel->insertRow(row++, items);
        }
    }
    //
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->stop();
        _pStackedWidget->setCurrentWidget(_pImageView);
    }
    //
    QApplication::processEvents(QEventLoop::AllEvents, 300);
    if(time == 0)
        _pImageView->scrollToBottom();
}

//
void LocalSearchMainWgt::onGetLinkMessage(qint64 time) {
    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->start();
        _pStackedWidget->setCurrentWidget(_pLoadingContent);
    }
    auto func = std::bind(&ChatMsgManager::getUserLinkHistoryMessage, time, _uid);
    std::vector<StNetMessageResult> _messages;
    auto future =  QtConcurrent::run([ func, &_messages](){
        auto allMessage = func();
        for(const auto& message : allMessage)
        {
            if(message.Type == INT_MIN)
                continue;

            QStringList links;
            QTalk::analysisLocalLink(message.Content.data(), links);

            for(const auto& link : links)
            {
                StNetMessageResult info;
                info.msg_id = message.MsgId.data();
                info.msg_type = QTalk::Entity::MessageTypeText;
                info.type = message.ChatType ;
                info.time = message.LastUpdateTime;
                info.from = message.From.data();
                info.body = link;
                info.xmpp_id = message.XmppId.data();
                {
                    StTextMessage linkMsg(StTextMessage::EM_LINK);
                    linkMsg.content = link;
                    info.text_messages.push_back(linkMsg);
                }
                _messages.push_back(info);
            }
        }
    });

//    future.waitForFinished();
    while (!future.isFinished())
        QApplication::processEvents(QEventLoop::AllEvents, 100);
    // set new data
    for(const auto& info : _messages)
    {
        auto* item = new QStandardItem();
        item->setData(info.type, SEARCH_USER_TYPE);
        item->setData(info.msg_type, SEARCH_USER_MSG_TYPE);
        item->setData(info.time, SEARCH_USER_MSG_TIME);
        item->setData(QVariant::fromValue(info), SEARCH_USER_INFO);
        _pLinkModel->appendRow(item);
    }
    _pLinkSortModel->sort(0);

    if(_messages.empty())
        _hasBefore[EM_LINK] = false;

    if(_pLoadingContent)
    {
        _pLoadingContent->movie()->stop();
        _pStackedWidget->setCurrentWidget(_pLinkListView);
    }

    //
    QApplication::processEvents(QEventLoop::AllEvents, 300);
    if(time == 0)
        _pLinkListView->scrollToBottom();
}

void LocalSearchMainWgt::closeEvent(QCloseEvent *e) {
    _pAllLstModel->clear();
    _pSearchModel->clear();
    _pImageModel->clear();
    _pFileModel->clear();
    UShadowDialog::closeEvent(e);
}

void LocalSearchMainWgt::hideEvent(QHideEvent* e)
{
    _pAllLstModel->clear();
    _pSearchModel->clear();
    _pImageModel->clear();
    _pFileModel->clear();
    UShadowDialog::hideEvent(e);
}