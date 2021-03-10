//
// Created by cc on 2020/12/22.
//

#include "GroupWnd.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <QApplication>
#include <QDesktopServices>
#include "../include/Line.h"
#include "../CustomUi/LinkButton.h"
#include "../CustomUi/QtMessageBox.h"
#include "../Platform/Platform.h"
#include "AddressBookPanel.h"
#include "MessageManager.h"
#include "../Platform/NavigationManager.h"
#include "../QtUtil/Entity/JID.h"

GroupWnd::GroupWnd(AddressBookPanel *parent)
    : QFrame(parent), _mainPanel(parent)
{
    ;
    // title
    auto *titleLayout = new QHBoxLayout;
    //
    _pNameLabel = new QLabel(this);
    _pHeadLabel = new HeadPhotoLab;
    _pSendMailBtn = new QPushButton(this);
    _pExitGroupBtn = new QPushButton(this);
    _pDestroyGroupBtn = new QPushButton(this);
    //
    _pSendMailBtn->setToolTip(tr("发邮件"));
    _pExitGroupBtn->setToolTip(tr("退出群组"));
    _pDestroyGroupBtn->setToolTip(tr("销毁群"));
    _pSendMailBtn->setFocusPolicy(Qt::NoFocus);
    _pExitGroupBtn->setFocusPolicy(Qt::NoFocus);
    _pDestroyGroupBtn->setFocusPolicy(Qt::NoFocus);
    _pDestroyGroupBtn->setObjectName("btn_destroy_group");
    //
    auto *btnLay = new QHBoxLayout;
    btnLay->addWidget(_pSendMailBtn);
    btnLay->addWidget(_pExitGroupBtn);
    btnLay->addWidget(_pDestroyGroupBtn);
    btnLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    //
    _pDestroyGroupBtn->setVisible(false);
    //
    auto *topMainLay = new QGridLayout;
    topMainLay->setContentsMargins(20, 5, 20, 20);
    topMainLay->setHorizontalSpacing(20);
    topMainLay->setVerticalSpacing(20);
    topMainLay->addWidget(_pNameLabel, 0, 0, 1, 1);
    topMainLay->addWidget(_pHeadLabel, 0, 1, 2, 1);
    topMainLay->addLayout(btnLay, 1, 0);
    // topMainLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding), 0, 2, 2, 1);
    //
    QFrame *topFrame = new QFrame(this);
    topFrame->setFixedHeight(160);
    topFrame->setObjectName("TopFrame");
    auto *topLayout = new QVBoxLayout(topFrame);
    topLayout->setContentsMargins(0, 0, 0, 20);
    topLayout->addLayout(titleLayout);
    topLayout->addLayout(topMainLay);
    topLayout->setStretch(1, 1);
    //
    QFrame *bodyFrm = new QFrame(this);
    bodyFrm->setContentsMargins(6, 25, 6, 0);
    bodyFrm->setObjectName("BottomFrame");
    auto *bodyLay = new QGridLayout;
    bodyLay->setHorizontalSpacing(15);
    bodyLay->setVerticalSpacing(15);
    //row 0
    _pGroupNameEdit = new QLineEdit(this);
    _pGroupNameEdit->setReadOnly(true);
    _pGroupNameEdit->setPlaceholderText(tr("修改群名称"));
    _pGroupNameEdit->setObjectName("GroupNameEdit");
    //
    bodyLay->addWidget(new QLabel(tr("群名称"), this), 0, 0);
    bodyLay->addWidget(_pGroupNameEdit, 0, 1, 1, 3);
    // bodyLay->setAlignment(_pGroupNameEdit, Qt::AlignLeft);
    // row 1
    _pGroupIdEdit = new QTextEdit(this);
    _pGroupIdEdit->setAcceptRichText(false);
    bodyLay->addWidget(new QLabel(tr("群ID"), this), 1, 0, Qt::AlignTop);
    bodyLay->addWidget(_pGroupIdEdit, 1, 1, 1, 3);
    // line
    bodyLay->addWidget(new Line(this), 2, 0, 1, 4);
    // row 3
    bodyLay->addWidget(new QLabel(tr("群公告"), this), 3, 0);
    // row 4
    _pGroupTopicEdit = new QTextEdit(this);
    _pGroupTopicEdit->setPlaceholderText(tr("暂无公告"));
    _pGroupTopicEdit->setAcceptRichText(false);
    _pGroupTopicEdit->setObjectName("GroupTopicEdit");
    bodyLay->addWidget(_pGroupTopicEdit, 4, 0, 1, 4);
    // row 5
    bodyLay->addWidget(new Line(this), 5, 0, 1, 4);
    // row 6
    QLabel *memberLabel = new QLabel(tr("群成员"), this);
    _pGroupMemberLabel = new QLabel(this);
    LinkButton *showMemberBtn = new LinkButton(tr("查看"));
    bodyLay->addWidget(memberLabel, 6, 0);
    bodyLay->addWidget(_pGroupMemberLabel, 6, 1);
    bodyLay->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding), 6, 2);
    bodyLay->addWidget(showMemberBtn, 6, 3);
    // row 7
    bodyLay->addWidget(new Line(this), 7, 0, 1, 4);
    // row 8
    bodyLay->addWidget(new QLabel(tr("群简介")), 8, 0);
    // row 9
    _pGroupIntroduce = new QTextEdit(this);
    _pGroupIntroduce->setPlaceholderText(tr("暂无简介"));
    _pGroupIntroduce->setAcceptRichText(false);
    _pGroupIntroduce->setObjectName("GroupIntroduceEdit");
    bodyLay->addWidget(_pGroupIntroduce, 9, 0, 1, 4);
    //
    _pSendMsgBtn = new QPushButton(tr("发消息"));
    _pSendMsgBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _pSendMsgBtn->setObjectName("SendMessage");
    _pSendMsgBtn->setFixedHeight(32);
    //

    auto *bLayout = new QVBoxLayout(bodyFrm);
    bLayout->addLayout(bodyLay);
    bLayout->setSpacing(36);
    bLayout->addWidget(_pSendMsgBtn);
    auto *mainFrm = new QFrame(this);
    mainFrm->setFixedWidth(360);
    mainFrm->setObjectName("GroupWnd");
    //
    auto *mainLay = new QVBoxLayout(mainFrm);
    mainLay->setMargin(0);
    mainLay->setSpacing(0);
    mainLay->addWidget(topFrame);
    mainLay->addWidget(new Line(this));
    mainLay->addWidget(bodyFrm);
    //
    auto *lay = new QVBoxLayout(this);
    lay->setAlignment(Qt::AlignHCenter);
    lay->setContentsMargins(0, 80, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(mainFrm);
    lay->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
    //
    _pSendMailBtn->setFixedSize(30, 30);
    _pExitGroupBtn->setFixedSize(30, 30);
    _pDestroyGroupBtn->setFixedSize(30, 30);
    _pHeadLabel->setFixedSize(60, 60);
    _pSendMailBtn->setObjectName("SendMail");
    _pExitGroupBtn->setObjectName("QuitGroup");
    _pNameLabel->setObjectName("NameLabel");
    //
    _pGroupIdEdit->verticalScrollBar()->setVisible(false);
    _pGroupTopicEdit->verticalScrollBar()->setVisible(false);
    _pGroupIntroduce->verticalScrollBar()->setVisible(false);
    //
    _pGroupIdEdit->setReadOnly(true);
    _pGroupTopicEdit->setReadOnly(true);
    _pGroupIntroduce->setReadOnly(true);
    _pGroupIdEdit->setFrameShape(QFrame::NoFrame);
    _pGroupTopicEdit->setFrameShape(QFrame::NoFrame);
    _pGroupIntroduce->setFrameShape(QFrame::NoFrame);
    _pGroupIdEdit->setFixedHeight(55);
    _pGroupTopicEdit->setFixedHeight(80);
    _pGroupIntroduce->setFixedHeight(30);
    //
    _pGroupMemberPopWnd = new GroupMemberPopWnd(this);
    //
    connect(showMemberBtn, &LinkButton::clicked, [this]() {
        _pGroupMemberPopWnd->showCenter(false, nullptr);
        _pGroupMemberPopWnd->raise();
    });

    connect(_pExitGroupBtn, &QPushButton::clicked, [this]() {
        int ret = QtMessageBox::warning(this, tr("警告"), tr("即将退出本群, 是否继续?"),
                                        QtMessageBox::EM_BUTTON_YES | QtMessageBox::EM_BUTTON_NO);
        if (ret == QtMessageBox::EM_BUTTON_YES)
        {
            AddressBookMsgManager::quitGroup(_strGroupId.toStdString());
            _mainPanel->showEmptyLabel();
        }
    });

    connect(_pSendMsgBtn, &QPushButton::clicked, this, &GroupWnd::sendMessageSlot);
    connect(_pDestroyGroupBtn, &QPushButton::clicked, this, &GroupWnd::onDestroyGroupGroupCard);
    connect(_pSendMailBtn, &QPushButton::clicked, this, &GroupWnd::onSendMail);
    connect(this, &GroupWnd::sgUpdateGroupMember, this, &GroupWnd::showGroupMember);
}

void GroupWnd::setData(std::shared_ptr<QTalk::Entity::ImGroupInfo> &data)
{
    QString localHead = QString::fromStdString(QTalk::GetHeadPathByUrl(data->HeaderSrc));
    if (QFile::exists(localHead))
        _pHeadLabel->setHead(localHead, 30, false, true);

    QString name = QString::fromStdString(data->Name);
    QFontMetricsF f(_pNameLabel->font());
    name = f.elidedText(name, Qt::ElideRight, 200);

    _pNameLabel->setText(name);
    _pGroupNameEdit->setText(QString::fromStdString(data->Name));
    _pGroupIdEdit->setText(QString::fromStdString(data->GroupId));
    _pGroupTopicEdit->setText(QString::fromStdString(data->Topic));
    _pGroupIntroduce->setText(QString::fromStdString(data->Introduce));
    //
    _strGroupId = QString::fromStdString(data->GroupId);
    _srtHead = QString::fromStdString(data->HeaderSrc);
    _groupName = QString::fromStdString(data->Name);
}

void GroupWnd::sendMessageSlot()
{
    StSessionInfo stSession(QTalk::Enum::GroupChat, _strGroupId, _groupName);
    stSession.headPhoto = _srtHead;

    emit _mainPanel->sgSwitchCurFun(0);
    emit _mainPanel->sgOpenNewSession(stSession);

    if (_pGroupMemberPopWnd->isActiveWindow())
    {
        _pGroupMemberPopWnd->close();
    }
}

void GroupWnd::showGroupMember(const std::map<std::string, QTalk::StUserCard> &userCards, const std::map<std::string, QUInt8> &userRole)
{
    _pSendMailBtn->setVisible(true);
    _pDestroyGroupBtn->setVisible(false);
    int onlineNum = 0;
    //
    _pGroupMemberPopWnd->reset();
    auto it = userCards.begin();
    for (; it != userCards.end(); it++)
    {
        //
        groupMembers.push_back(it->first);
        //
        std::string name = it->second.nickName;
        if (name.empty())
            name = QTalk::getUserNameNoMask(it->first);

        QString xmppId = QString::fromStdString(it->first);
        QString userName = QString::fromStdString(name);
        QString headSrc = QString::fromStdString(QTalk::GetHeadPathByUrl(it->second.headerSrc));
        QString searchKey = QString::fromStdString(it->second.searchKey);

        QUInt8 role = 3;
        if (userRole.find(it->first) != userRole.end())
            role = userRole.at(it->first);
        //
        bool isOnline = PLAT.isOnline(it->first);

        if (role == 1)
        {
            std::string userId = it->first;
            std::string selfId = PLAT.getSelfXmppId();
            _pDestroyGroupBtn->setVisible(userId == selfId);
        }

        _pGroupMemberPopWnd->addItem(xmppId, userName, headSrc, role, isOnline, searchKey);

        if (isOnline)
            onlineNum++;
    }
    _pGroupMemberPopWnd->addEnd();
    _pGroupMemberLabel->setText(QString("( %1/ %2)").arg(onlineNum).arg(userCards.size()));
}

void GroupWnd::onSendMail()
{
    if (!groupMembers.empty())
    {
        int btn = QtMessageBox::warning(this, tr("%1 即将打开您的默认邮箱客户端 \n").arg(QApplication::applicationName()),
                                        tr("此操作可能导致邮箱客户端崩溃( 取决于邮件客户端和群发人数 )"),
                                        QtMessageBox::EM_BUTTON_YES | QtMessageBox::EM_BUTTON_NO);

        if (btn == QtMessageBox::EM_BUTTON_NO)
            return;

        QString mailUrl = QString("mailto:");
        auto selfDomain = PLAT.getSelfDomain();
        for (const auto &u : groupMembers)
        {
            QTalk::Entity::JID jid(u);
            if (selfDomain != jid.domainname())
                continue;

            QString mail = QString("%1@%2; ")
                               .arg(jid.username().data())
                               .arg(NavigationManager::instance().getMailSuffix().data());
            mailUrl.append(mail);
        }
        QDesktopServices::openUrl(QUrl(mailUrl));
    }
    this->close();
}

void GroupWnd::onDestroyGroupGroupCard()
{
    int ret = QtMessageBox::warning(this, tr("警告"), tr("群即将被销毁, 是否继续?"), QtMessageBox::EM_BUTTON_YES | QtMessageBox::EM_BUTTON_NO);
    if (ret == QtMessageBox::EM_BUTTON_YES)
    {
        AddressBookMsgManager::destroyGroup(_strGroupId.toStdString());
        _mainPanel->showEmptyLabel();
    }
}