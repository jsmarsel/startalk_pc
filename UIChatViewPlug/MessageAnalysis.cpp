//
// Created by cc on 2019/11/07.
//

#include "MessageAnalysis.h"
#include "../Emoticon/EmoticonMainWgt.h"
#include "../Platform/Platform.h"
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <ChatUtil.h>

namespace QTalk {

    void analysisMessage(StNetMessageResult &info)
    {
        switch (info.msg_type)
        {
            case QTalk::Entity::MessageTypeGroupNotify:

            case QTalk::Entity::MessageTypeRobotTurnToUser:
            case QTalk::Entity::MessageTypeRobotAnswerList:
            {
                break;
            }
            case QTalk::Entity::MessageTypeRevoke:
            {

                QString content = QObject::tr("%1撤回了一条消息").arg(info.from == PLAT.getSelfXmppId().data() ?
                                                               QObject::tr("你") : info.user_name);
                info.body = content;
                break;
            }
            case QTalk::Entity::MessageTypeFile:
            {
                QTalk::analysisFileMessage(info.body, info.extend_info, info);
                break;
            }
            case QTalk::Entity::MessageTypeCommonTrdInfo:
            case QTalk::Entity::MessageTypeCommonTrdInfoV2:
            {
                QTalk::analysisCommonTrdMessage(info.body, info.extend_info, info);
                break;
            }
            case QTalk::Entity::MessageTypeSourceCode:
            {
                QTalk::analysisCodeMessage(info.body, info.extend_info, info);
                break;
            }
            case QTalk::Entity::MessageTypeSmallVideo:
            {
                QTalk::analysisVideoMessage(info.body, info.extend_info, info);
                break;
            }
            case QTalk::Entity::WebRTC_MsgType_VideoCall:
            case QTalk::Entity::WebRTC_MsgType_AudioCall:
            case QTalk::Entity::WebRTC_MsgType_Video:
            case QTalk::Entity::WebRTC_MsgType_Video_Group:
                break;
            case QTalk::Entity::MessageTypeShock:
            {
                QString msgFrom = info.from;
                info.body = QObject::tr("你");
                if (PLAT.getSelfXmppId() == msgFrom.toStdString())
                    info.body.append(QObject::tr("抖了他一下"));
                else
                    info.body.append(QObject::tr("收到一个窗口抖动"));
                break;
            }
            default:
            {
                qWarning() << "------ default message type " << info.msg_type;
            }
            case QTalk::Entity::MessageTypeGroupAt:
            {
                auto doc = QJsonDocument::fromJson(info.backup_info.toUtf8());
                if(doc.isNull())
                {

                }
                else
                {
                    auto ary = doc.array();
                    for(const auto& item : ary)
                    {
                        auto obj = item.toObject();
                        int objType = obj.value("type").toInt();
                        if(objType == 10001 )
                        {
                            auto data = obj.value("data").toArray();
                            for(const auto& at : data)
                            {
                                auto atObj = at.toObject();
                                QString id = atObj.value("jid").toString();
                                QString text = atObj.value("text").toString();
                                info.at_users[id] = text;
                            }
                        }
                    }
                }
            }
            case QTalk::Entity::MessageTypeText:
            case QTalk::Entity::MessageTypePhoto:
            case QTalk::Entity::MessageTypeImageNew:
            case QTalk::Entity::MessageTypeRobotAnswer:
            {
                bool isSendMessage = info.direction == QTalk::Entity::MessageDirectionSent && info.state == 0;
                if(isSendMessage && info.body.isEmpty())
                {
                    analysisSendTextMessage(info.extend_info, info.text_messages);
                }
                else
                {
                    if(info.body.isEmpty() && !info.extend_info.isEmpty())
                        analysisSendTextMessage(info.extend_info, info.text_messages);
                    else
                        analysisTextMessage(info.body,info.text_messages, info.at_users);
                }
                break;
            }
        }
    }

    void analysisTextMessage(const QString& content, std::vector<StTextMessage>& messages, const std::map<QString, QString>& atUsers)
    {
        QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");
        regExp.setMinimal(true);

        int pos = 0;
        int prePos = 0;
        while ((pos = regExp.indexIn(content, pos)) != -1) {
            // 前面的文本
            QString text = content.mid(prePos, pos - prePos); // 取匹配开始位置到匹配到的位置文本
            if (!text.isEmpty())
                analysisLinkMessage(text, messages, atUsers);

            QString item = regExp.cap(0); // 符合条件的整个字符串
            QString type = regExp.cap(1); // 多媒体类型
            QString val = regExp.cap(2); // 路径

            if ("url" == type) {
                StTextMessage linkMsg(StTextMessage::EM_LINK);
                linkMsg.content = val;
                messages.push_back(linkMsg);
            } else if ("emoticon" == type) {

                StTextMessage emoMsg(StTextMessage::EM_EMOTICON);
                QString pkgid, shortCut;
                analysisEmoticonMessage(item, pkgid, shortCut);

                if(pkgid.isEmpty() || shortCut.isEmpty())
                {
                    emoMsg.imageWidth = emoMsg.imageHeight = 50;
                    emoMsg.content = ":/chatview/image1/default.png";
                }
                else
                {
                    QString localEmo = EmoticonMainWgt::instance()->getEmoticonLocalFilePath(pkgid, shortCut);
                    QFileInfo info(localEmo);
                    if(localEmo.isEmpty() || !info.exists() || info.isDir())
                    {
                        emoMsg.imageWidth = emoMsg.imageHeight = 50;
                        emoMsg.imageLink = QString("%1_%2").arg(pkgid, shortCut.replace("/", ""));
                    }
                    else
                    {
                        emoMsg.content = localEmo;
                        QPixmap emo(localEmo);
                        emoMsg.imageWidth = emo.width();
                        emoMsg.imageHeight = emo.height();
                        QTalk::Image::scaImageSize(emoMsg.imageWidth, emoMsg.imageHeight);
                    }
                    shortCut.replace("/", "");
                    emoMsg.pkgid = pkgid;
                    emoMsg.shortCut = shortCut;
                }

                messages.push_back(emoMsg);
            }
            else if("image" == type)
            {
                StTextMessage imageMsg(StTextMessage::EM_IMAGE);
                analysisImageMessage(item, imageMsg.imageLink, imageMsg.imageWidth, imageMsg.imageHeight);

                QString localImg = QTalk::GetImagePathByUrl(imageMsg.imageLink.toStdString()).data();
                QFileInfo info(localImg);
                if(!info.exists() || info.isDir())
                {
//                    imageMsg.content = ":/chatview/image1/default.png";
//                    downloadImage(msgId.data(), imageMsg.imageLink, imageMsg.imageWidth, imageMsg.imageHeight);
                }
                else
                    imageMsg.content = localImg;

                messages.push_back(imageMsg);
            } else {
	            StTextMessage textMsg(StTextMessage::EM_TEXT);
	            textMsg.content = item;
	            messages.push_back(textMsg);
            }
            //
            pos += regExp.matchedLength();
            prePos = pos;
        }

        QString lastStr = content.mid(prePos);
        if (!lastStr.isEmpty())
            analysisLinkMessage(lastStr, messages, atUsers);
    }

    void  analysisSendTextMessage(const QString& extendInfo, std::vector<StTextMessage>& messages)
    {
        auto document = QJsonDocument::fromJson(extendInfo.toUtf8());
        if(document.isNull()){

        }
        else
        {
            QJsonArray array = document.array();
            for(auto && i : array)
            {
                QJsonObject obj = i.toObject();
                int key = obj.value("key").toInt();
                QString value = obj.value("value").toString();
                switch (key)
                {
                    case Type_Text:
                    {
//                        analysisLinkMessage(value, messages);
                        analysisTextMessage(value, messages);
                        break;
                    }
                    case Type_Image:
                    {
                        StTextMessage imageMsg(StTextMessage::EM_IMAGE);
                        QFileInfo info(value);
                        if(!info.exists() || info.isDir())
                        {
                            imageMsg.content = ":/chatview/image1/default.png";
                            imageMsg.imageWidth = imageMsg.imageHeight = 200;
                        }
                        else
                        {
                            imageMsg.content = value;
                            QPixmap pixmap(value);
                            imageMsg.imageWidth = pixmap.width();
                            imageMsg.imageHeight = pixmap.height();
                            QTalk::Image::scaImageSize(imageMsg.imageWidth, imageMsg.imageHeight);
                        }

                        messages.push_back(imageMsg);
                        break;
                    }
                    case Type_At:
                    {
                        if ( PLAT.getSelfName().data() == value) {
                            StTextMessage atMsg(StTextMessage::EM_ATMSG);
                            atMsg.content = QString("@%1 ").arg(value);
                            messages.push_back(atMsg);
                        } else {
                            StTextMessage textMsg(StTextMessage::EM_TEXT);
                            textMsg.content = QString("@%1 ").arg(value);
                            messages.push_back(textMsg);
                        }
                        break;
                    }
                    case Type_Url:
                    {
                        StTextMessage linkMsg(StTextMessage::EM_LINK);
                        linkMsg.content = value;
                        messages.push_back(linkMsg);
                        break;
                    }
                    case Type_Invalid:
                    default:
                        break;
                }
            }
        }
    }

    void analysisLinkMessage(const QString &text, std::vector<StTextMessage>& messages, const std::map<QString, QString>& atUsers)
    {
        if (!text.isEmpty()) {
            // 对于没有安装[obj]格式发送的链接 进行转换
            QRegExp regExp("(((ftp|https?)://|www.)[a-zA-Z0-9\\.\\-]+\\.([a-zA-Z0-9]{1,4})([^ ^\"^\\[^\\]^\\r^\\n]*))");

            auto pos = 0;
            auto prePos = 0;
            while ((pos = regExp.indexIn(text, pos)) != -1) {
                QString url = regExp.cap(0);

                if (pos - prePos > 0) {
                    QString tmpText = text.mid(prePos, pos - prePos); // 取匹配开始位置到匹配到的位置文本
                    analysisAtMessage(tmpText, messages, atUsers);
                }

                StTextMessage linkMsg(StTextMessage::EM_LINK);
                linkMsg.content = url;
                messages.push_back(linkMsg);

                pos += regExp.matchedLength();
                prePos = pos;
            }


            if (prePos <= text.size() - 1) {
                QString lastStr = text.mid(prePos);
                analysisAtMessage(lastStr, messages, atUsers);
            }
        }
    }

    //
    void analysisAtMessage(const QString& content, std::vector<StTextMessage>& messages, const std::map<QString, QString>& atUsers)
    {
        auto tmpStr = QString("@%1 ").arg(PLAT.getSelfName().data());
        static QString allStr = "@all";
        // type groupAt
        if((!atUsers.empty() &&
           atUsers.find(PLAT.getSelfXmppId().data()) != atUsers.end()) || content.contains(tmpStr))
        {
            auto blocks = content.split(tmpStr);
            for(auto it = blocks.begin(); it != blocks.end();)
            {
                StTextMessage textMsg(StTextMessage::EM_TEXT);
                textMsg.content = *it;
                messages.push_back(textMsg);
                it++;

                if(it != blocks.end())
                {
                    StTextMessage atMsg(StTextMessage::EM_ATMSG);
                    atMsg.content = tmpStr;
                    messages.push_back(atMsg);
                }
            }
        }
        else if(content.contains(allStr))
        {
            auto blocks = content.split(allStr);
            for(auto it = blocks.begin(); it != blocks.end();)
            {
                StTextMessage textMsg(StTextMessage::EM_TEXT);
                textMsg.content = *it;
                messages.push_back(textMsg);
                it++;

                if(it != blocks.end())
                {
                    StTextMessage atMsg(StTextMessage::EM_ATMSG);
                    atMsg.content = allStr;
                    messages.push_back(atMsg);
                }
            }
        }
        else
        {
            StTextMessage textMsg(StTextMessage::EM_TEXT);
            textMsg.content = content;
            messages.push_back(textMsg);
        }
    }

    void analysisImageMessage(const QString &content, QString &imageLink, qreal &width, qreal &height) {

        QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");

        if (regExp.indexIn(content) != -1) {

            QString item = regExp.cap(0); // 符合条件的整个字符串
            QString type = regExp.cap(1); // 多媒体类型
            QString val = regExp.cap(2); // 路径
            QString strWidth = regExp.cap(3); // 宽高

            std::string localFilePath;

            if ("image" == type) {

                QString rawUrl = QUrl(val).toEncoded();
                imageLink = rawUrl;

                if (!strWidth.isEmpty()) {
                    QRegExp exp("width=([0-9]*\\.?[0-9]+) height=([0-9]*\\.?[0-9]+)");
                    if (exp.indexIn(strWidth) >= 0) {
                        width = exp.cap(1).toDouble();
                        height = exp.cap(2).toDouble();
                    }
                }
                //
                if(width == 0 && height == 0)
                {
                    QString localImg = QTalk::GetImagePathByUrl(imageLink.toStdString()).data();
                    QFileInfo info(localImg);
                    if(info.exists() && !info.isDir())
                    {
                        QPixmap pixmap(localImg);
                        width = pixmap.width();
                        height = pixmap.height();
                    }
                    else
                    {
                        width = height = 200;
                        return;
                    }
                }

                QTalk::Image::scaImageSize(width, height);

            } else if ("emoticon" == type) {
//                error_log("analysis error type content:{0}", content.toStdString());
            }
        }
    }

/**
 *
 * @param content
 * @param imageLink
 * @param width
 * @param height
 */
    void analysisEmoticonMessage(const QString& content, QString& pkgid, QString& shortCut)
    {
        QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");

        if (regExp.indexIn(content) != -1) {

            QString item = regExp.cap(0); // 符合条件的整个字符串
            QString type = regExp.cap(1); // 多媒体类型
            QString val = regExp.cap(2); // 路径
            QString strWidth = regExp.cap(3); // 宽高

            if ("image" == type) {
//                error_log("analysis error type content:{0}", content.toStdString());
            } else if ("emoticon" == type) {
                shortCut = val;
                shortCut.remove("[").remove("]");
                if (!strWidth.isEmpty()) {
                    QRegExp exp("width=(.+) height=(.+)");
                    if (exp.indexIn(strWidth) >= 0) {
                        pkgid = exp.cap(1);
                    }
                }
            }
        }
    }

    void analysisFileMessage(const QString& content, const QString& extendInfo, StNetMessageResult& ret)
    {
        QString data = extendInfo;
        if(extendInfo.isEmpty())
            data = content;

        if(data.isEmpty())
            return;

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if(doc.isEmpty())
        {

        }
        else
        {
            QJsonObject object = doc.object();
            ret.file_info.fileName = object.value("FileName").toString();
            ret.file_info.fileSize = object.value("FileSize").toString();
            ret.file_info.fileLink = object.value("HttpUrl").toString();
            ret.file_info.fileMd5 = object.value("FILEMD5").toString();
            ret.file_info.fileIcon = QTalk::getIconByFileName(ret.file_info.fileName);
        }
    }

    //
    void analysisCommonTrdMessage(const QString& content, const QString& extendInfo, StNetMessageResult& ret)
    {
        QString data = extendInfo;
        if(extendInfo.isEmpty())
            data = content;

        if(data.isEmpty())
            return;

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if(doc.isEmpty())
        {
            ret.common_trd.desc = data;
        }
        else
        {
            QJsonObject object = doc.object();
            ret.common_trd.title = object.value("title").toString();
            ret.common_trd.desc = object.value("desc").toString();
            ret.common_trd.img = object.value("img").toString();
            ret.common_trd.link = object.value("linkurl").toString();
            ret.common_trd.showAll = object.value("showas667").toBool(false);
        }
    }

    // code
    void analysisCodeMessage(const QString& content, const QString& extendInfo, StNetMessageResult& ret)
    {
        QString data = extendInfo;
        if(extendInfo.isEmpty())
            data = content;

        if(data.isEmpty())
            return;

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if(doc.isEmpty())
        {
            ret.code.code = data;
        }
        else
        {
            QJsonObject object = doc.object();
            ret.code.code = object.value("Code").toString();
            ret.code.codeStyle = object.value("CodeStyle").toString();
            ret.code.language = object.value("CodeType").toString();
        }
    }

    //
    void analysisVideoMessage(const QString& content, const QString& extendInfo, StNetMessageResult& ret)
    {
        QString data = extendInfo;
        if(extendInfo.isEmpty())
            data = content;

        if(data.isEmpty())
            return;

        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if(doc.isEmpty())
        {
            ret.code.code = data;
        }
        else
        {
            QJsonObject object = doc.object();
            ret.video.videoUrl = object.value("FileUrl").toString();
            ret.video.thumbUrl = object.value("ThumbUrl").toString();
            ret.video.width = object.value("Width").toInt(0);
            ret.video.height = object.value("Height").toInt(0);
            if (ret.video.width == 0 || ret.video.height == 0) {
                ret.video.width = object.value("Width").toString().toInt();
                ret.video.height = object.value("Height").toString().toInt();
            }
            if(object.contains("newVideo"))
                ret.video.newVideo = object.value("newVideo").toBool();
            //
            QTalk::Image::scaImageSize(ret.video.width, ret.video.height);
        }
    }

    void analysisLocalImage(const QString& content, QStringList& links)
    {
        QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");
        regExp.setMinimal(true);

        int pos = 0;
        while ((pos = regExp.indexIn(content, pos)) != -1) {
            QString item = regExp.cap(0); // 符合条件的整个字符串
            QString type = regExp.cap(1); // 多媒体类型
            QString val = regExp.cap(2); // 路径

            if("image" == type)
            {
                QString link;
                qreal w = 0, h = 0;
                analysisImageMessage(item, link, w, h);
                if(!link.isEmpty())
                    links.push_back(link);
            }
            //
            pos += regExp.matchedLength();
        }
    }

    void analysisLocalLink(const QString& content, QStringList& links)
    {
        QRegExp regExp("\\[obj type=[\\\\]?\"([^\"]*)[\\\\]?\" value=[\\\\]?\"([^\"]*)[\\\\]?\"(.*)\\]");
        regExp.setMinimal(true);

        int pos = 0;
        while ((pos = regExp.indexIn(content, pos)) != -1) {
            QString item = regExp.cap(0); // 符合条件的整个字符串
            QString type = regExp.cap(1); // 多媒体类型
            QString val = regExp.cap(2); // 路径

            if("url" == type)
                links.push_back(val);

            pos += regExp.matchedLength();
        }
    }
}