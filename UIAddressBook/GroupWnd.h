//
// Created by cc on 2020/12/22.
//

#ifndef QTALK_V2_GROUPWND_H
#define QTALK_V2_GROUPWND_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include "../entity/im_group.h"
#include "../include/CommonStrcut.h"
#include "../CustomUi/HeadPhotoLab.h"
#include "../include/ModButton.h"
#include "../CustomUi/GroupMemberPopWnd.hpp"

class AddressBookPanel;
class GroupWnd : public QFrame
{
    Q_OBJECT

public:
    explicit GroupWnd(AddressBookPanel *parent = nullptr);

Q_SIGNALS:
    void sgUpdateGroupMember(std::map<std::string, QTalk::StUserCard>, std::map<std::string, QUInt8> userRole);

public:
    void setData(std::shared_ptr<QTalk::Entity::ImGroupInfo> &data);

private:
    void sendMessageSlot();
    void showGroupMember(const std::map<std::string, QTalk::StUserCard> &, const std::map<std::string, QUInt8> &userRole);
    void onSendMail();
    void onDestroyGroupGroupCard();

private:
    AddressBookPanel *_mainPanel{};

    QLabel *_pNameLabel{};
    HeadPhotoLab *_pHeadLabel{};
    QPushButton *_pSendMailBtn{};
    QPushButton *_pExitGroupBtn{};
    QPushButton *_pDestroyGroupBtn{};
    QLineEdit *_pGroupNameEdit{};
    QTextEdit *_pGroupIdEdit{};
    QTextEdit *_pGroupTopicEdit{};
    QLabel *_pGroupMemberLabel{};
    QTextEdit *_pGroupIntroduce{};
    QPushButton *_pSendMsgBtn{};

    GroupMemberPopWnd *_pGroupMemberPopWnd{};
    QString _strGroupId;
    QString _groupName;
    QString _srtHead;

    std::vector<std::string> groupMembers;
};

#endif //QTALK_V2_GROUPWND_H
