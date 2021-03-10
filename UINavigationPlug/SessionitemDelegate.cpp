//
// Created by cc on 2018/11/20.
//

#include "SessionitemDelegate.h"
#include "../QtUtil/Utils/Log.h"
#include "../CustomUi/HeadPhotoLab.h"
#include <QPainter>
#include <QDateTime>
#include <iostream>
#include <QEvent>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QPen>
#include <QPainterPath>
#include "../Platform/Platform.h"
#include "../UICom/qimage/qimage.h"
#include "../Platform/dbPlatForm.h"
#include "../UICom/StyleDefine.h"

#define HEAD_WIDTH 42
#define ISTOP_TEXT "[置顶] "
#define AT_TEXT_SELF "[有人@我]"
#define AT_TEXT_ALL "[@all]"
#define DRAFT_TIP "[草稿]:"

using namespace QTalk;

/**
  * @函数名
  * @功能描述
  * @参数
  * @author cc
  * @date 2018.9.27
  */
QString GenerateTimeText(const QInt64 &time)
{
    if(0 == time)
        return "";

    QDateTime curTime = QDateTime::currentDateTimeUtc();
    QDateTime msgTime = QDateTime::fromMSecsSinceEpoch(time);
    int curYear = curTime.date().year();
    int msgYear = msgTime.date().year();

    if (curYear > msgYear)
        return msgTime.date().toString("yyyy-MM-dd");

    QInt64 curDays = curTime.date().toJulianDay();
    QInt64 msgDays = msgTime.date().toJulianDay();

    if (curDays - msgDays > 10 * 24)
        return "";

    QString t = curDays > msgDays ? msgTime.date().toString("MM-dd") : msgTime.time().toString("hh:mm");
    std::string tt = t.toStdString();
    return t;
}

bool SessionSortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    if (!source_left.isValid() || !source_right.isValid())
        return false;

    bool leftTop = source_left.data(ITEM_DATATYPE_ISTOP).toBool();
    bool rightTop = source_right.data(ITEM_DATATYPE_ISTOP).toBool();
    bool leftQ = source_left.data(ITEM_DATATYPE_QQQ).toBool();
    bool rightQ = source_right.data(ITEM_DATATYPE_QQQ).toBool();
    bool ret = false;

    if(leftTop != rightTop)
        ret = leftTop > rightTop;
    else if(leftQ != rightQ)
        ret = leftQ > rightQ;
    else
        ret = source_left.data(ITEM_DATATYPE_LASTTIME).toLongLong() > source_right.data(ITEM_DATATYPE_LASTTIME).toLongLong();

    return ret;
}

bool SessionSortModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    int role = filterRole();
    bool res = EM_SELECT_ALL == role;
//    auto userId = index.data(ITEM_DATATYPE_USERID).toString();
//    auto name = index.data(ITEM_DATATYPE_USERNAME).toString();

//    if(!res)
//    {
//        auto userId = index.data(ITEM_DATATYPE_USERID).toString();
//        auto selectId = this->property("CURRENT_ID").toString();
//        res = userId == selectId;
//    }

    if (!res)
    {
        switch (role)
        {
            case EM_SELECT_TOP:
                {
                    res = index.data(ITEM_DATATYPE_ISTOP).toBool();
                    break;
                }

            case EM_SELECT_UNREAD:
            case EM_SELECT_UNNOTICE:
                {
                    int unreadCnt = index.data(ITEM_DATATYPE_UNREADCOUNT).toInt();
                    bool unNotice = index.data(ITEM_DATATYPE_UNNOTICE).toBool();

                    if(role == EM_SELECT_UNREAD)
                        res = !unNotice && unreadCnt > 0;
                    else
                        res = unNotice;

                    break;
                }

            default:
                break;
        }
    }

    return res;
}

//
QSize SessionitemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QSize &size = QStyledItemDelegate::sizeHint(option, index);
    return {size.width(), 66};
}

void SessionitemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(!index.isValid())
        return;

    painter->save();
    bool select = false;
    {
        auto userId = index.data(ITEM_DATATYPE_USERID).toString();
        auto selectId = this->property("CURRENT_ID").toString();
        select = userId == selectId;
    }
    painter->setRenderHint(QPainter::TextAntialiasing);       //
    bool isTop = index.data(ITEM_DATATYPE_ISTOP).toBool();
    bool unNotice = index.data(ITEM_DATATYPE_UNNOTICE).toBool();
    // 背景色
    QRect rect = option.rect;

    if (select)
        painter->fillRect(rect, StyleDefine::instance().getNavSelectColor());
    else
    {
        painter->fillRect(rect, isTop ? StyleDefine::instance().getNavTopColor() :
                          StyleDefine::instance().getNavNormalColor());
    }

    if(isTop)
    {
        QPainterPath path;
        path.moveTo(rect.left(), rect.top());
        path.lineTo(rect.left() + 12, rect.top());
        path.lineTo(rect.left(), rect.top() + 12);
        painter->fillPath(path, QBrush(StyleDefine::instance().getNavTipColor()));
    }

    QString headPath = index.data(ITEM_DATATYPE_HEADPATH).toString();
    QString strName = index.data(ITEM_DATATYPE_USERNAME).toString();
    QString content = index.data(ITEM_DATATYPE_MESSAGECONTENT).toString();
    qint64 time = index.data(ITEM_DATATYPE_LASTTIME).toLongLong();
    QString strTime = GenerateTimeText(time);
    QString draft = index.data(ITEM_DATATYPE_DRAFT).toString();
    bool q = index.data(ITEM_DATATYPE_QQQ).toBool();
    int chattype = index.data(ITEM_DATATYPE_CHATTYPE).toInt();
    //
    strName.replace("\n", " ");
    strName = strName.trimmed().replace(QRegExp("\\s{1,}"), " ");
    bool isOnline = index.data(ITEM_DATATYPE_ISONLINE).toBool();
    int dpi = QTalk::qimage::dpi();
    int unreadCount = index.data(ITEM_DATATYPE_UNREADCOUNT).toInt();;
    QString strUnreadCount = QString::number(unreadCount);

    if(unreadCount > 99) strUnreadCount = "99+";

    //
    int atCount = index.data(ITEM_DATATYPE_ATCOUNT).toInt();
    int fontLevel = AppSetting::instance().getFontLevel();
    //
    QTalk::setPainterFont(painter, fontLevel, 14);
    QFontMetricsF nameF(painter->font());
    double maxNameWidth = rect.width() - 80 - nameF.width(strTime);
    strName = nameF.elidedText(strName, Qt::ElideRight, maxNameWidth);
    painter->restore();
    painter->save();
    // 名称
    painter->setPen(QPen(select ? StyleDefine::instance().getNavNameSelectFontColor() : StyleDefine::instance().getNavNameFontColor()));
    QRectF textRect(rect.x() + 70, rect.y(), (int)maxNameWidth, rect.height() / 2);
    painter->drawText(textRect, Qt::AlignBottom, strName);
    //
    qreal atMsgWidth = 0;

    // 消息免打扰
    if(unNotice)
    {
        painter->setRenderHints(QPainter::Antialiasing, true);
        QPixmap pixmap = QTalk::qimage::loadImage(":/UINavigationPlug/image1/noNotice.png", true, true,
                         HEAD_WIDTH * dpi);
        QRect unNoticeRect(rect.right() - 35, rect.y() + rect.height() / 2 + 6, 16, 16);
        painter->drawPixmap(unNoticeRect, pixmap);

        if(unreadCount > 0)
            content = QString("[%1条] %2").arg(unreadCount).arg(content);
    }

    // 最近消息内容
    QTalk::setPainterFont(painter, fontLevel, 12);
    QFontMetricsF contentF(painter->font());

    if(!draft.isEmpty())
    {
        painter->setPen(QPen(StyleDefine::instance().getNavAtFontColor()));
        qreal draftTipWidth = contentF.width(DRAFT_TIP) + 1;
        QRectF tipRect(rect.x() + 65, rect.y() + rect.height() / 2 + 7, draftTipWidth, contentF.height() + 5);
        painter->drawText(tipRect, Qt::AlignTop, DRAFT_TIP);
        painter->setPen(QPen(StyleDefine::instance().getNavContentFontColor()));
        qreal contentWidth = rect.width() - 80 - draftTipWidth - 20;
        content = contentF.elidedText(content, Qt::ElideRight, contentWidth);
        QRectF contentRect(rect.x() + 65 + draftTipWidth, rect.y() + rect.height() / 2 + 7,
                           contentWidth + 5, contentF.height() + 5);
        painter->drawText(contentRect, Qt::AlignTop, QString("%1").arg(draft));
    }
    else
    {
        if(q)
            painter->fillRect(rect.x(), rect.y(), 6, rect.height(), QColor(255, 0, 0, 150));

        if (unreadCount > 0 && atCount > 0)
        {
            QString strAtCount = (bool) (atCount & 0xF0) ? AT_TEXT_SELF : AT_TEXT_ALL;
            atMsgWidth = contentF.width(strAtCount) + 2;
            painter->setPen(QPen(StyleDefine::instance().getNavAtFontColor()));
            QRectF atRect(rect.x() + 65, rect.y() + rect.height() / 2 + 7, atMsgWidth, contentF.height() + 5);
            painter->drawText(atRect, Qt::AlignTop, strAtCount);
        }

        painter->setPen(QPen(select ? StyleDefine::instance().getNavContentSelectFontColor() : StyleDefine::instance().getNavContentFontColor()));
        qreal contentWidth = rect.width() - 80 - atMsgWidth - 20;
        content = contentF.elidedText(content, Qt::ElideRight, contentWidth);
        QRectF contentRect(rect.x() + 70 + atMsgWidth, rect.y() + rect.height() / 2 + 7,
                           contentWidth + 5, contentF.height() + 5);
        painter->drawText(contentRect, Qt::AlignTop, content);
    }

    // 时间戳
    painter->setPen(QPen(select ? StyleDefine::instance().getNavTimeSelectFontColor() : StyleDefine::instance().getNavTimeFontColor()));
    QTalk::setPainterFont(painter, fontLevel, 11);
    painter->drawText(QRect(rect.x() + 65, rect.y() + 20, rect.width() - 70, rect.height())
                      , Qt::AlignRight, strTime);
    // 头像
    painter->setRenderHints(QPainter::Antialiasing, true);
    QPixmap pixmap;
    QFileInfo headFileInfo(headPath);

    if(headFileInfo.exists() && headFileInfo.suffix().toLower() == "gif")
    {
        headPath = QTalk::qimage::getGifImagePathNoMark(headPath);
        pixmap = QTalk::qimage::loadImage(headPath, true, true, HEAD_WIDTH * dpi);
    }
    else
        pixmap = QTalk::qimage::loadImage(headPath, true, true, HEAD_WIDTH * dpi);

    if(pixmap.isNull())
    {
        if(chattype == QTalk::Enum::GroupChat)
        {
#ifdef _STARTALK
            headPath = ":/QTalk/image1/StarTalk_defaultGroup.png";
#else
            headPath = ":/QTalk/image1/defaultGroupHead.png";
#endif
        }
        else
        {
#ifdef _STARTALK
            headPath = ":/QTalk/image1/StarTalk_defaultHead.png";
#else
            headPath = ":/QTalk/image1/headPortrait.png";
#endif
        }

        pixmap = QTalk::qimage::loadImage(headPath, true, true, HEAD_WIDTH * dpi);
    }

    if(!isOnline)
        pixmap = QTalk::qimage::generateGreyPixmap(pixmap);

    {
        QPainterPath path;
        QRect headRect(rect.x() + 20, rect.y() + 12, HEAD_WIDTH, HEAD_WIDTH);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(240, 240, 240, 200));
        painter->drawEllipse(headRect);
        path.addEllipse(headRect);
        painter->setClipPath(path);
        int w = pixmap.width() / dpi;
        int h = pixmap.height() / dpi;
        painter->drawPixmap((HEAD_WIDTH - w) / 2 + headRect.x(),
                            (HEAD_WIDTH - h) / 2 + headRect.y(), w, h, pixmap);
        //
        painter->fillPath(path, QTalk::StyleDefine::instance().getHeadPhotoMaskColor());
    }
    painter->restore();
    painter->save();

    // 未读消息提示
    if(unreadCount > 0)
    {
        QPen pen(QColor(255, 108, 86));
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(pen);
        painter->setBrush(QBrush(QColor(255, 75, 62)));

        if(unNotice)
        {
            QRect unreadRect(rect.right() - 17, rect.y() + rect.height() / 2 + 10, 8, 8);
            painter->drawRoundRect(unreadRect, 99, 99);
        }
        else
        {
            QRect unreadRect(rect.right() - 32, rect.y() + rect.height() / 2 + 6, 20, 13);
            painter->drawRoundRect(unreadRect, 80, 90);
            painter->setPen(QPen(StyleDefine::instance().getNavUnReadFontColor()));
            QTalk::setPainterFont(painter, fontLevel, 10);
            painter->drawText(unreadRect,  Qt::AlignCenter, strUnreadCount);
        }
    }

    //
    painter->restore();
}

