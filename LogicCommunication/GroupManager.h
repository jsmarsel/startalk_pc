
#ifndef _GROUPMANAGER_H_
#define _GROUPMANAGER_H_

#include <map>
#include <vector>
#include "../entity/im_group.h"
#include "../include/CommonStrcut.h"

typedef std::map<std::string, std::vector<QTalk::Entity::ImGroupInfo> > MapGroupCard;

class Communication;

class GroupManager {
public:
    explicit GroupManager(Communication *pComm);

public:
    //
    bool getUserGroupInfo(MapGroupCard &groups);
    //
    bool getGroupCard(const MapGroupCard &groups);
    //
    bool upateGroupInfo(const QTalk::StGroupInfo& groupInfo);
    bool updateTopic(const std::string& groupId, const std::string& topic);
    //
    void getUserIncrementMucVcard();

protected:
    Communication *_pComm;
};

#endif//_GROUPMANAGER_H_