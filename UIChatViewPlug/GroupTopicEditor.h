//
// Created by cc on 2020/12/9.
//

#ifndef QTALK_V2_GROUPTOPICEDITOR_H
#define QTALK_V2_GROUPTOPICEDITOR_H

#include "../CustomUi/UShadowWnd.h"
#include <QTextEdit>
#include <QToolButton>

class GroupTopicEditor : public UShadowDialog{
    Q_OBJECT
public:
    explicit GroupTopicEditor(const QString& groupId,
                              const QString& groupName,
                              const QString& groupTopic = "",
                              QWidget* parent = nullptr);

private:
    void onEditBtnClicked();
    void onClearBtnClicked();
    void enableUI(bool editing);
    void onCancelBtnClicked();
    void onOkBtnClicked();

private:
    QTextEdit *_pEdit{};

    QToolButton *_pEditBtn{};
    QToolButton *_pClearBtn{};
    QToolButton *_pOkBtn{};
    QToolButton *_pCancelBtn{};

private:
    QString _groupId;
    QString _groupTopic;
};



#endif //QTALK_V2_GROUPTOPICEDITOR_H
