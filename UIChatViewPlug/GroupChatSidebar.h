#ifndef _GROUPCHATSIDEBAR_H_
#define _GROUPCHATSIDEBAR_H_

#include <QFrame>
#include <QTextEdit>
#include <QVector>
#include <vector>
#include <QMutexLocker>
#include "../Message/GroupMessage.h"

class GroupMember;
class GroupTopic;
class GroupChatSidebar : public QFrame
{
	Q_OBJECT
public:
    explicit GroupChatSidebar(QWidget* parent = nullptr);
	~GroupChatSidebar() override;

Q_SIGNALS:
	void deleteMember(const std::string& );
	void updateGroupTopic(const QString&);
    void sgUpdateGroupInfo(const QString& groupId, const QString& groupName);

public:
	void updateGroupMember(const GroupMemberMessage& e);
	void updateGroupMember(const std::string& memberJid, const std::string& nick, int affiliation);
	void clearData();
	void onUpdateGroupInfo(const QString& groupId, const QString& groupName);

private:
	void initUi();

public:
	GroupMember* _pGroupMember{};
    GroupTopic*  _pGroupTopic{};

private:
	std::vector<std::string> _arMembers;

private:
	QMutex _mutex;
	QString _groupId;
	QString _groupName;
	QString _groupTopic;
};

#endif//