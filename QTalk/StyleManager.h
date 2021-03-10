#ifndef STYLESHEETMANAGER_H
#define STYLESHEETMANAGER_H

#include <QObject>

class StyleManager : public QObject
{
    Q_OBJECT
public:
    void setStyleSheets(int theme, const std::string& font);
    void setStylesForApp(int theme, const std::string& font);
    void setStyleSheetForPlugin(const QString& plgName, int theme);
};

#endif // STYLESHEETMANAGER_H
