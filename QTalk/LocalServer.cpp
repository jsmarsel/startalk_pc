//
// Created by cc on 2019/11/02.
//

#include "LocalServer.h"
#include <QLocalSocket>
#include <QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "../Platform/Platform.h"
#include "../Platform/dbPlatForm.h"
#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif // _WINDOWS

LocalServer::LocalServer() {
    _pServer = new QLocalServer(this);
    _pServer->setSocketOptions(QLocalServer::WorldAccessOption);
    connect(_pServer, &QLocalServer::newConnection, this, &LocalServer::onNewClient);
}

LocalServer::~LocalServer() {
    _pServer->close();
}

void LocalServer::runServer(const QString &listenName) {
    QLocalServer::removeServer(listenName);
    bool ok = _pServer->listen(listenName);
    if (!ok)
    {
        // todo
    }
    qInfo() << "LocalServer listen" << listenName << ok;
}

void LocalServer::onNewClient() {
    auto socket = _pServer->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, this, &LocalServer::socketReadyReadHandler);
    connect(socket, &QLocalSocket::disconnected, [this, socket](){
        _datas.erase(socket);
    });
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

// add header
void sendMessage(QLocalSocket* socket, const QByteArray& data)
{
    //
    int len = data.size();
    QByteArray header((char*)&len, sizeof(len));
    //
    socket->write(header);
    socket->write(data);

    socket->flush();
}

void sayHiAsk(QLocalSocket* socket)
{
    QJsonObject obj;
    obj.insert("type", package_say_hi_ack);
    QJsonDocument document;
    document.setObject(obj);
    auto data = document.toJson();
    sendMessage(socket, data);
}

void sendBaseInfo(QLocalSocket* socket)
{
    QJsonObject obj;
    obj.insert("type", package_response);
    obj.insert("response_type", request_base_info);
    QJsonObject detail;
    detail.insert("login_user", PLAT.getSelfXmppId().data());
    detail.insert("login_nav", PLAT.getLoginNav().data());
    detail.insert("ckey", PLAT.getClientAuthKey().data());
    obj.insert("data", detail);
    QJsonDocument document;
    document.setObject(obj);
    auto data = document.toJson();
    sendMessage(socket, data);
}

void sendUserInfo(QLocalSocket* socket, const QJsonArray& users)
{
    if(users.empty() )
        return;

    QJsonObject obj;
    obj.insert("type", package_response);
    obj.insert("response_type", request_user_info);
    QJsonArray userInfos;
    for(const auto& tmp : users)
    {
        QJsonObject userInfo;
        std::string userId = tmp.toString().toStdString();
        auto info = DB_PLAT.getUserInfo(userId);
        userInfo.insert("user_id", tmp);
        if(info)
        {
            userInfo.insert("user_name", info->Name.data());
            userInfo.insert("user_head", info->HeaderSrc.data());
        }
        userInfos.append(userInfo);
    }
    obj.insert("data", userInfos);
    QJsonDocument document;
    document.setObject(obj);
    auto data = document.toJson(QJsonDocument::Compact);
    sendMessage(socket, data);
}

//
void sendWakeupAck(QLocalSocket* socket)
{
    QJsonObject obj;
    obj.insert("type", package_response);
    obj.insert("response_type", request_wakeup_window);
    QJsonDocument document;
    document.setObject(obj);
    auto data = document.toJson(QJsonDocument::Compact);
    sendMessage(socket, data);
}

//
void LocalServer::dealMessage(QLocalSocket* socket, const QByteArray& arrayData)
{
    QJsonParseError error{};
    QJsonDocument document = QJsonDocument::fromJson(arrayData, &error);
    if(document.isEmpty())
    {
        qWarning() << "LocalServer json error" << error.errorString();
    }
    else
    {
        QJsonObject object = document.object();
        auto type = object.value("type").toInt(package_invalid);
        switch (type)
        {
            case package_say_hi:
            {
                sayHiAsk(socket);
                break;
            }
            case package_say_bye:
            {
                socket->disconnectFromServer();
                break;
            }
            case package_request:
            {
                int request = object.value("request_type").toInt(request_invalid);
                switch (request)
                {
                    case request_base_info:
                        sendBaseInfo(socket);
                        break;
                    case request_user_info:
                    {
                        QJsonArray users = object.value("users").toArray();
                        sendUserInfo(socket, users);
                        break;
                    }
                    case request_wakeup_window:
                    {
                        emit sgWakeupWindow();
                        sendWakeupAck(socket);
                        break;
                    }
                    case request_invalid:
                    default:
                        break;
                }
                break;
            }
            case package_invalid:
            default:
                break;
        }
    }
}

void LocalServer::socketReadyReadHandler()
{
    auto* socket = dynamic_cast<QLocalSocket*>(sender());
    if (socket)
    {
        auto& data = _datas[socket];
        data += socket->readAll();
        if (data.size() > sizeof(int))
        {
            //
            int headerlen = sizeof(int);
            auto header = data.left(headerlen);
            int len = *(int*)((void*)header.data());
//            if (data.size() < len + sizeof(int))
//                data += socket->readAll();
            if (data.size() < len + sizeof(int))
                return;
            data = data.mid(headerlen);
            //
            auto body = data.left(len);
            data = data.mid(len);
            dealMessage(socket, body);
        }
    }
}
