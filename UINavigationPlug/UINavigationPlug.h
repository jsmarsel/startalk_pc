﻿#ifndef UINAVIGATIONPLUG_H
#define UINAVIGATIONPLUG_H

#include "uinavigationplug_global.h"
#include "../interface/view/IUINavigationPlug.h"
#include "SessionFrm.h"

class UINAVIGATIONPLUGSHARED_EXPORT UINavigationPlug : public QObject, public IUINavigationPlug
{
    Q_OBJECT
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "com.plugin.navigation.www/1.0" FILE "navigation.json")
#endif
    Q_INTERFACES(IUINavigationPlug)
public:
    UINavigationPlug();

    // UIPluginInterface interface
public:
    QWidget *widget() override;

    // IUINavigationPlug interface
private:
    void init() override;
    void updateReadCount() override;

private:
    SessionFrm *_mainPanel;
};

#endif // UINAVIGATIONPLUG_H
