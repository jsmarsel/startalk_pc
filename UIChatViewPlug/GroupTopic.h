#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

#ifndef _GROUPTOPIC_H_
#define _GROUPTOPIC_H_

#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QStackedLayout>

class GroupTopic : public QFrame {
Q_OBJECT
public:
    explicit GroupTopic(QString &groupId,
                        QString &groupName,
                        QString &groupTopic,
                        QWidget *parent = nullptr);

    ~GroupTopic() override;

public:
    void setTopic(const QString &topic);

protected:
    void initUi();
    bool eventFilter(QObject *, QEvent *) override;

private:
    void onEditBtnClicked();

private:
    QTextEdit *_pTopicEdit{};
    QLabel *_pEmptyLabel{};
    QStackedLayout *_pStackLay{};
    QPushButton *_pBtnShowMore{};

private:
    QString &_groupId;
    QString &_groupName;
    QString &_groupTopic;
};

#endif//_GROUPTOPIC_H_