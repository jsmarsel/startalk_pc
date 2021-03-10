//
// Created by cc on 2019-07-10.
//

#ifndef QTALK_V2_UCBUTTON_H
#define QTALK_V2_UCBUTTON_H

#include <QFrame>
#include "customui_global.h"

/**
* @description: UCButton
* @author: cc
* @create: 2019-07-10 14:22
**/
class CUSTOMUISHARED_EXPORT UCButton : public QFrame
{
    Q_OBJECT
public:
    explicit UCButton(QString  text, QWidget *parent = nullptr);
    ~UCButton() override = default;

public:
    void setCheckState(bool check);

Q_SIGNALS:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) override ;
    bool event(QEvent *e) override ;

private:
    QString _content;
    bool    _isCheck;
    QFont   _font;
};

class CUSTOMUISHARED_EXPORT UCButtonGroup : public QObject
{
    Q_OBJECT
public:
    explicit UCButtonGroup(QObject *parent = nullptr);
    ~UCButtonGroup() override = default;

Q_SIGNALS:
    void clicked(int);

public:
    void addButton(UCButton *, int);
    UCButton *button(int index);
    void setCheck(int);

private:
    void onButtonClicked();

private:
    std::map<UCButton *, int> _mapBtns;
};
#endif //QTALK_V2_UCBUTTON_H
