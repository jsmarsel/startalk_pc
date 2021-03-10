#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
#ifndef SEARCHITEMDELEGATE_H
#define SEARCHITEMDELEGATE_H

#include <QStyledItemDelegate>
#include "../include/CommonStrcut.h"

enum SearchItemRoles
{
    EM_TYPE_TYPE        = Qt::UserRole + 1, //item类型 对应存 SearchItemType
    EM_ITEM_ROLE_ITEM_TYPE, // QTalk::Search::Action
    EM_ITEM_ROLE_ICON,
    EM_ITEM_ROLE_NAME,
    EM_ITEM_ROLE_SUB_MESSAGE,
    EM_ITEM_ROLE_XMPPID,

    EM_TITLE_ROLE_TYPE,
    EM_TITLE_ROLE_NAME,
    EM_TITLE_ROLE_HAS_MORE,
    EM_TITLE_ROLE_REQ_TYPE,
    EM_TITLE_ROLE_HOVER,

    EM_ITEM_ROLE_KEY,
};

enum SearchItemType
{
    EM_ITEM_TYPE_TITLE,
    EM_ITEM_TYPE_ITEM,
    EM_ITEM_TYPE_SHOW_MORE
};

enum {
    REQ_TYPE_ALL,
    REQ_TYPE_USER,
    REQ_TYPE_GROUP,
    REQ_TYPE_HISTORY,
    REQ_TYPE_FILE,
};

class SearchItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
	using QStyledItemDelegate::QStyledItemDelegate;

public:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

Q_SIGNALS:
    void sgOpenNewSession(int, const QString&, const QString&, const QString&);
    void sgGetMore(int);
    void sgSwitchFun(int);
    void sgShowMessageRecordWnd(const QString&, const QString&);
    void sgShowFileRecordWnd(const QString&);

private:
    int hover_index_row = 0;
    bool press = false;
};

#endif // SEARCHITEMDELEGATE_H
