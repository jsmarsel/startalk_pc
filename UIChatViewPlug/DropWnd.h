//
// Created by cc on 2020/9/10.
//

#ifndef QTALK_V2_DROPWND_H
#define QTALK_V2_DROPWND_H

#include <QFrame>

class DropWnd : public  QFrame{
    Q_OBJECT

    using  QFrame::QFrame;

public:
    void setDrop(const QPixmap& mask, const QString& name);

protected:
    void paintEvent(QPaintEvent* e) override ;

private:
    QString _name;
    QPixmap _mask;
};


#endif //QTALK_V2_DROPWND_H
