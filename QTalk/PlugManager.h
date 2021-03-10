#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QMap>
#include <QPluginLoader>
#include <QLibrary>
#include <QVector>
#include <memory>

class PlugManager
{
public:
    void LoadPluginAllQt();
    bool UnloadPluginQt(const QString &key);
    QObject *GetPluginInstanceQt(const QString &key);
    std::shared_ptr<QMap<QString, QObject *>> GetAllPluginInstanceQt() const;
    void setPluginPath(const QString &path);

public:
    void setPlugNames(const QVector<QString>& plugs);

private:
    QMap<QString, QPluginLoader*> _pluginRegisterQt;
    QString _pluginPath;

    QVector<QString> _plugs;
};

#endif // PLUGINMANAGER_H
