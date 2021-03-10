#include "PlugManager.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QApplication>
#include <iostream>
#include "../QtUtil/Utils/Log.h"

/**
  * @函数名
  * @功能描述 加载所有qt插件
  * @参数
  * @date 2018.9.11
  */
void PlugManager::LoadPluginAllQt() {
    //
    for (const QString& plug : _plugs) {
#ifdef _MACOS
#ifdef QT_NO_DEBUG
        QString plugin = _pluginPath + "lib" + plug + ".dylib";
#else
        QString plugin = _pluginPath + "lib" + plug + "d.dylib";
#endif
#else
#ifdef _WINDOWS
#ifdef QT_NO_DEBUG
        QString plugin = _pluginPath + plug + ".dll";
#else
        QString plugin = _pluginPath + plug + "d.dll";
#endif
#else
#ifdef QT_NO_DEBUG
        QString plugin = _pluginPath + "lib" + plug + ".so";
#else
        QString plugin = _pluginPath + "lib" + plug + "d.so";
#endif
#endif
#endif

        if (QFile::exists(plugin)) {
            qInfo() << "plugin has been found:  " << plugin;
            auto *loader = new QPluginLoader(plugin);
            _pluginRegisterQt.insert(plug, loader);
        } else {
            qWarning() << "plugin file is not exists... path: " << plugin;
        }
    }
}

/**
  * @函数名
  * @功能描述 根据关键字卸载插件
  * @参数
  * @date 2018.9.11
  */
bool PlugManager::UnloadPluginQt(const QString &key) {
    QPluginLoader *loader = _pluginRegisterQt.value(key);
    if (nullptr != loader) {
        bool result = loader->unload();
        _pluginRegisterQt.remove(key);
        delete loader;
        return result;
    }
    return false;
}

/**
  * @函数名
  * @功能描述 获取插件实例
  * @参数
  * @date 2018.9.11
  */
QObject *PlugManager::GetPluginInstanceQt(const QString &key) {

    if (_pluginRegisterQt.value(key)) {
        QObject *plugin = _pluginRegisterQt.value(key)->instance();
        if (plugin) {
            return plugin;
        } else {
            auto strErr = _pluginRegisterQt.value(key)->errorString();
            qWarning() << "error load:  " << strErr;
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.9.27
  */
std::shared_ptr<QMap<QString, QObject *> > PlugManager::GetAllPluginInstanceQt() const {
    std::shared_ptr<QMap<QString, QObject *> > plugins(new QMap<QString, QObject *>);
        for (const QString& pluginName : _pluginRegisterQt.keys()) {
            QPluginLoader *pluginloader = _pluginRegisterQt.value(pluginName);
            if (pluginloader->instance()) {
                plugins->insert(pluginName, pluginloader->instance());
            }
        }
    return plugins;
}

/**
  * @函数名
  * @功能描述 设置插件相对应用程序路径
  * @参数
  * @date 2018.9.27
  */
void PlugManager::setPluginPath(const QString &path) {
    _pluginPath = path;
}

//
void PlugManager::setPlugNames(const QVector<QString> &plugs) {
    _plugs = plugs;
}
