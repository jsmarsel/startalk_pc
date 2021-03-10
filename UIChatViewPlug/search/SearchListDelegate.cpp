//
// Created by cc on 2019/11/05.
//

#include "SearchListDelegate.h"
#include "../../UICom/StyleDefine.h"
#include "../../UICom/qimage/qimage.h"
#include "../../Platform/AppSetting.h"
#include <QPainter>
#include <QFileInfo>
#include <QEvent>
#include <QDebug>
#include <QPainterPath>

#define HEAD_WIDTH 40

SearchListDelegate::SearchListDelegate(QWidget * parent)
    :QStyledItemDelegate(parent)
{
}

QSize SearchListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    const QSize& size = QStyledItemDelegate::sizeHint(option, index);
    return {size.width(), 48};
}

void SearchListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
//    QStyledItemDelegate::paint(painter, option, index);
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);

    QRect rect = option.rect;
    if (option.state & QStyle::State_Selected)
    {
        auto color = QTalk::StyleDefine::instance().getSearchSelectColor();
        painter->fillRect(rect, color);
    }
    else
        painter->fillRect(rect, QTalk::StyleDefine::instance().getSearchNormalColor());

    auto itemType = index.data(EM_SEARCH_LIST_DATA_ROLE).toInt();
    switch (itemType)
    {
        case EM_SEARCH_LIST_ROLE_ITEM:
        {
            QString name = index.data(EM_SEARCH_LIST_DATA_NAME).toString();
            QString subMessage = index.data(EM_SEARCH_LIST_DATA_SUB_MESSAGE).toString();
            // icon
            painter->setRenderHints(QPainter::Antialiasing, true);
            painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
            painter->setPen(Qt::NoPen);
            QRect headRect(rect.x() + 16, (rect.height() - HEAD_WIDTH) / 2 + rect.y(),
                           HEAD_WIDTH, HEAD_WIDTH);
            int dpi = QTalk::qimage::dpi();

            QString icon = index.data(EM_SEARCH_LIST_DATA_HEAD).toString();
            QFileInfo iconInfo(icon);
            if(!iconInfo.exists() || iconInfo.isDir() )
            {
#ifdef _STARTALK
                icon =  ":/QTalk/image1/StarTalk_defaultHead.png";
#else
                icon = ":/QTalk/image1/headPortrait.png";
#endif
            }
            QPixmap pixmap = QTalk::qimage::loadImage(icon, false, true, HEAD_WIDTH * dpi);
            QPainterPath path;
            painter->setBrush(QColor(240, 240, 240, 200));
            painter->drawEllipse(headRect);
            path.addEllipse(headRect);
            painter->setClipPath(path);
            int w = pixmap.width() / dpi;
            int h = pixmap.height() / dpi;
            painter->drawPixmap((HEAD_WIDTH - w) / 2 + headRect.x(),
                                (HEAD_WIDTH - h) / 2 + headRect.y(), w, h, pixmap);
            //
            painter->restore();
            painter->save();
            // name
            painter->setPen(QTalk::StyleDefine::instance().getNavNameFontColor());
            QRect nameRect;
            int x = rect.x() + 16 + HEAD_WIDTH + 8;
            h = rect.height() / 2;
            int maxW = rect.width() - x - 16;

            nameRect = QRect(x, 0 + rect.y(), maxW, h);
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 14);
            painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignBottom, name);
            // sub message
            painter->setPen(QTalk::StyleDefine::instance().getNavContentFontColor());
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 12);
            QFontMetricsF itemSubF(painter->font());
            QRect subRect = QRect(x, h + rect.y(), maxW, h);
            QString text = itemSubF.elidedText(subMessage, Qt::ElideRight, maxW);
            text.replace("\n", "");
            painter->drawText(subRect, Qt::AlignVCenter | Qt::AlignTop,
                              text);
            break;
        }
        case EM_SEARCH_LIST_ROLE_GET_MORE:
        {
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 14);
            painter->setPen(QTalk::StyleDefine::instance().getNavContentFontColor());
            painter->drawText(rect, Qt::AlignCenter, tr("查看更多"));
            break;
        }
        default:
            break;
    }

    painter->restore();
}

bool SearchListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) {

    if(event->type() == QEvent::MouseButtonPress)
    {
        auto itemType = index.data(EM_SEARCH_LIST_DATA_ROLE).toInt();
        switch (itemType) {
            case EM_SEARCH_LIST_ROLE_ITEM: {
                QString key = index.data(EM_SEARCH_LIST_DATA_KEY).toString();
                QString id = index.data(EM_SEARCH_LIST_DATA_XMPP_ID).toString();
                int type = index.data(EM_SEARCH_LIST_DATA_TO_TYPE).toInt();
                emit sgGetUserMessages(key, id, type);
                break;
            }
            case EM_SEARCH_LIST_ROLE_GET_MORE: {
                emit sgGetMore();
                model->removeRow(index.row());
                break;
            }
            default:
                break;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
