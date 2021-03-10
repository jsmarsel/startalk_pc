//
// Created by cc on 2018/11/20.
//

#if _MSC_VER >= 1600
    #pragma execution_character_set("utf-8")
#endif

#ifndef QTALK_V2_SESSIONITEMDELEGATE_H
#define QTALK_V2_SESSIONITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPen>
#include <QSortFilterProxyModel>

enum ItemData
{
    ITEM_DATATYPE_CHATTYPE = Qt::UserRole + 1,
    ITEM_DATATYPE_USERID,
    ITEM_DATATYPE_REALJID,
    ITEM_DATATYPE_USERNAME,
    ITEM_DATATYPE_LASTTIME,
    ITEM_DATATYPE_LASTSTRTIME,
    ITEM_DATATYPE_HEADPATH,
    ITEM_DATATYPE_ISONLINE,
    ITEM_DATATYPE_MESSAGECONTENT,
    ITEM_DATATYPE_UNREADCOUNT,
    ITEM_DATATYPE_ATCOUNT,
    ITEM_DATATYPE_ISTOP,
    ITEM_DATATYPE_UNNOTICE,
    ITEM_DATATYPE_LAST_MESSAGE_ID,
    ITEM_DATATYPE_DRAFT,
    ITEM_DATATYPE_QQQ
};


enum FilterType
{
    EM_SELECT_ALL = 0,
    EM_SELECT_TOP,
    EM_SELECT_UNREAD,
    EM_SELECT_UNNOTICE
};

class SessionSortModel : public QSortFilterProxyModel
{
protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

class SessionitemDelegate : public QStyledItemDelegate
{
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    QWidget *_pParentWgt;
};


#endif //QTALK_V2_SESSIONITEMDELEGATE_H
