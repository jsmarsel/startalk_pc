#include "SearchItemDelegate.h"
#include "../UICom/qimage/qimage.h"
#include "../QtUtil/Utils/Log.h"
#include "../include/im_enum.h"
#include "../UICom/StyleDefine.h"
#include "../Platform/Platform.h"

#include <QListView>
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include <QMouseEvent>
#include <QEvent>
#include <QPainterPath>

#define HEAD_WIDTH 40

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.07
  */
QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    SearchItemType itemType = (SearchItemType)index.data(EM_TYPE_TYPE).toInt();

    const QSize& size = QStyledItemDelegate::sizeHint(option, index);

    switch (itemType)
    {
        case EM_ITEM_TYPE_TITLE:
        case EM_ITEM_TYPE_SHOW_MORE:
            return {size.width(), 42};
        default:
        case EM_ITEM_TYPE_ITEM:
            return {size.width(), 56};
    }
}

/**
  * @函数名
  * @功能描述
  * @参数
  * @date 2018.11.07
  */
void SearchItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);

    SearchItemType itemType = (SearchItemType)index.data(EM_TYPE_TYPE).toInt();

    QRect rect = option.rect;
    if (option.state & QStyle::State_Selected)
        painter->fillRect(rect, QTalk::StyleDefine::instance().getSearchSelectColor());
    else
        painter->fillRect(rect, QTalk::StyleDefine::instance().getSearchNormalColor());

    switch (itemType)
    {
        case EM_ITEM_TYPE_TITLE:
        {
            QString name = index.data(EM_TITLE_ROLE_NAME).toString();
            bool hasMore = index.data(EM_TITLE_ROLE_HAS_MORE).toBool();
            int req = index.data(EM_TITLE_ROLE_REQ_TYPE).toInt();
            bool hover = index.data(EM_TITLE_ROLE_HOVER).toBool();
            //
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 14);
            QFontMetricsF titleNameF(painter->font());
            painter->setPen(QTalk::StyleDefine::instance().getNavNameFontColor());
            QRect nameRect(rect.x() + 14, (int)(rect.height() - titleNameF.height()) / 2 + rect.y(),
                           rect.width() - 15, (int)titleNameF.height() + 5);
            painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, name);
            //
            QString btnName;
            if(REQ_TYPE_ALL == req /**|| REQ_TYPE_HISTORY == req || REQ_TYPE_FILE == req**/)
            {
                if(hasMore)
                    btnName = tr("展开更多");
                else
                    break;
            }
            else
                btnName = tr("收起");

            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 12);
            QFontMetricsF titleBtnF(painter->font());
            if (hover)
                painter->setPen(QColor(0, 202, 190));
            else
                painter->setPen(QTalk::StyleDefine::instance().getNavContentFontColor());

            int w = (int)titleBtnF.width(btnName) + 5;
            QRect btnNameRect(rect.right() - w - 14, (int)(rect.height() - titleBtnF.height()) / 2 + rect.y(),
                              w, (int)titleBtnF.height() + 5);
            painter->drawText(btnNameRect, Qt::AlignVCenter, btnName);

            break;
        }
        case EM_ITEM_TYPE_ITEM:
        {
            QString name = index.data(EM_ITEM_ROLE_NAME).toString();
            QString subMessage = index.data(EM_ITEM_ROLE_SUB_MESSAGE).toString();
            int retType = index.data(EM_ITEM_ROLE_ITEM_TYPE).toInt();
            bool showCenter = subMessage.isEmpty();
            // icon
            painter->setRenderHints(QPainter::Antialiasing, true);
            painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
            painter->setPen(Qt::NoPen);
            QRect headRect(rect.x() + 16, (rect.height() - HEAD_WIDTH) / 2 + rect.y(),
                           HEAD_WIDTH, HEAD_WIDTH);
            int dpi = QTalk::qimage::dpi();
            if(QTalk::Search::EM_ACTION_HS_FILE == retType)
            {
                QFileInfo iconInfo(name);
                QString iconPath;
                QString suffix = iconInfo.suffix().toLower();
                if(suffix == "mp3")
                    iconPath = ":/QTalk/image1/file_type/audio.png";
                else if(suffix == "mp4")
                    iconPath = ":/QTalk/image1/file_type/video.png";
                else if(suffix == "txt" || suffix == "json")
                    iconPath = ":/QTalk/image1/file_type/text.png";
                else if(suffix == "pdf")
                    iconPath = ":/QTalk/image1/file_type/pdf.png";
                else if(suffix == "ppt" || suffix == "pptx")
                    iconPath = ":/QTalk/image1/file_type/ppt.png";
                else if(suffix == "doc" || suffix == "docx")
                    iconPath = ":/QTalk/image1/file_type/word.png";
                else if(suffix == "xls" || suffix == "xlsx")
                    iconPath = ":/QTalk/image1/file_type/excel.png";
                else if(suffix == "rar" || suffix == "zip" || suffix == "7z")
                    iconPath = ":/QTalk/image1/file_type/zip.png";
                else
                    iconPath = ":/QTalk/image1/file_type/unknown.png";

                QPixmap pixmap = QTalk::qimage::loadImage(iconPath, true, true, HEAD_WIDTH * dpi);
                int w = pixmap.width() / dpi;
                int h = pixmap.height() / dpi;
                painter->drawPixmap((HEAD_WIDTH - w) / 2 + headRect.x(),
                                    (HEAD_WIDTH - h) / 2 + headRect.y(), w, h, pixmap);
            }
            else
            {
                QString icon = index.data(EM_ITEM_ROLE_ICON).toString();
                QFileInfo iconInfo(icon);
                if(!iconInfo.exists() || iconInfo.isDir() )
                {
#ifdef _STARTALK
                    icon =  ":/QTalk/image1/StarTalk_defaultHead.png";
#else
                    icon = ":/QTalk/image1/headPortrait.png";
#endif
                }
                QPixmap pixmap = QTalk::qimage::loadImage(icon, true, true, HEAD_WIDTH * dpi);
                QPainterPath path;
                painter->setBrush(QColor(240, 240, 240, 200));
                painter->drawEllipse(headRect);
                path.addEllipse(headRect);
                painter->setClipPath(path);
                int w = pixmap.width() / dpi;
                int h = pixmap.height() / dpi;
                painter->drawPixmap((HEAD_WIDTH - w) / 2 + headRect.x(),
                                    (HEAD_WIDTH - h) / 2 + headRect.y(), w, h, pixmap);
            }
            //
            painter->restore();
            painter->save();
            // name
            painter->setPen(QTalk::StyleDefine::instance().getNavNameFontColor());
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 14);
            QFontMetricsF itemNameF(painter->font());
            QRect nameRect;
            int x = rect.x() + 16 + HEAD_WIDTH + 8;
            int h = rect.height() / 2;
            int maxW = rect.width() - x - 16;
            if(showCenter)
            {
                nameRect = QRect(x, rect.y(), maxW, rect.height());
                painter->drawText(nameRect, Qt::AlignVCenter, name);
            }
            else
            {
                nameRect = QRect(x, 0 + rect.y(), maxW, h);
                painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignBottom, name);
            }
            // sub message
            if(!showCenter)
            {
                painter->setPen(QTalk::StyleDefine::instance().getNavContentFontColor());
                QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 12);
                QFontMetricsF itemSubF(painter->font());
                QRect subRect = QRect(x, h + rect.y(), maxW, h);
                painter->drawText(subRect, Qt::AlignVCenter | Qt::AlignTop,
                        itemSubF.elidedText(subMessage, Qt::ElideRight, maxW));
            }
            break;
        }
        case EM_ITEM_TYPE_SHOW_MORE:
        {
            QTalk::setPainterFont(painter, AppSetting::instance().getFontLevel(), 14);
            QFontMetricsF moreF(painter->font());
            painter->setPen(QTalk::StyleDefine::instance().getNavContentFontColor());
            painter->drawText(rect, Qt::AlignCenter, tr("查看更多"));
            break;
        }
        default:
            break;
    }

    painter->restore();
}

bool SearchItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                            const QStyleOptionViewItem &option, const QModelIndex &index)
{
    SearchItemType itemType = (SearchItemType)index.data(EM_TYPE_TYPE).toInt();

    if(event->type() == QEvent::MouseButtonPress)
    {
        press = true;
        hover_index_row = index.row();

        switch (itemType)
        {
            case EM_ITEM_TYPE_TITLE:
            {
                const auto &rect = option.rect;
                QRect btnRect(rect.right() - 14 - 50 + rect.x(), (rect.height() - 14) / 2 + rect.y(), 50, 16);
                auto* evt = (QMouseEvent*)event;
                if(btnRect.contains(evt->pos()))
                {
                    model->setData(index, true, EM_TITLE_ROLE_HOVER);
                    //
                    int retType = index.data(EM_TITLE_ROLE_TYPE).toInt();
                    int req = index.data(EM_TITLE_ROLE_REQ_TYPE).toInt();
                    if(REQ_TYPE_USER == req ||
                        REQ_TYPE_GROUP == req ||
                        REQ_TYPE_HISTORY == req ||
                        REQ_TYPE_FILE == req)
                    {
                        emit sgSwitchFun(REQ_TYPE_ALL);
                    }
                    else if(REQ_TYPE_ALL == req)
                    {
                        switch (retType)
                        {
                            case QTalk::Search::EM_ACTION_USER:
                                emit sgSwitchFun(REQ_TYPE_USER);
                                break;
                            case QTalk::Search::EM_ACTION_MUC:
                            case QTalk::Search::EM_ACTION_COMMON_MUC:
                            case (QTalk::Search::EM_ACTION_COMMON_MUC | QTalk::Search::EM_ACTION_MUC):
                                emit sgSwitchFun(REQ_TYPE_GROUP);
                                break;
                            case QTalk::Search::EM_ACTION_HS_SINGLE:
                            case QTalk::Search::EM_ACTION_HS_MUC:
                            case QTalk::Search::EM_ACTION_HS_SINGLE | QTalk::Search::EM_ACTION_HS_MUC:
                            {
                                emit sgSwitchFun(REQ_TYPE_HISTORY);
                                break;
                            }
                            case QTalk::Search::EM_ACTION_HS_FILE:
                            {
                                emit sgSwitchFun(REQ_TYPE_FILE);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
                break;
            }
            case EM_ITEM_TYPE_ITEM:
            {
                //
                int type = model->data(index, EM_ITEM_ROLE_ITEM_TYPE).toInt();

                if(QTalk::Search::EM_ACTION_USER == type)
                {
                    int chatType = QTalk::Enum::TwoPersonChat;
                    QString xmppId = index.data(EM_ITEM_ROLE_XMPPID).toString();
                    QString icon = index.data(EM_ITEM_ROLE_ICON).toString();
                    QString name = index.data(EM_ITEM_ROLE_NAME).toString();
                    emit sgOpenNewSession(chatType, xmppId, name, icon);
                }
                else if(QTalk::Search::EM_ACTION_MUC == type ||
                        QTalk::Search::EM_ACTION_COMMON_MUC == type ||
                        (QTalk::Search::EM_ACTION_MUC | QTalk::Search::EM_ACTION_COMMON_MUC) == type)
                {
                    int chatType = QTalk::Enum::GroupChat;
                    QString xmppId = index.data(EM_ITEM_ROLE_XMPPID).toString();
                    QString icon = index.data(EM_ITEM_ROLE_ICON).toString();
                    QString name = index.data(EM_ITEM_ROLE_NAME).toString();
                    emit sgOpenNewSession(chatType, xmppId, name, icon);
                }
                else if(QTalk::Search::EM_ACTION_HS_SINGLE == type ||
                        QTalk::Search::EM_ACTION_HS_MUC == type ||
                        (QTalk::Search::EM_ACTION_HS_SINGLE | QTalk::Search::EM_ACTION_HS_MUC) == type)
                {
                    QString key = index.data(EM_ITEM_ROLE_KEY).toString();
                    QString xmppId = index.data(EM_ITEM_ROLE_XMPPID).toString();
                    emit sgShowMessageRecordWnd(key, xmppId);
                }
                else if(QTalk::Search::EM_ACTION_HS_FILE == type)
                {
                    QString key = index.data(EM_ITEM_ROLE_KEY).toString();
                    emit sgShowFileRecordWnd(key);
                }
                break;
            }
            case EM_ITEM_TYPE_SHOW_MORE:
            {
                int req = index.data(EM_TITLE_ROLE_REQ_TYPE).toInt();
                emit sgGetMore(req);
                break;
            }
            default:
                break;
        }

    }
    else if(event->type() == QEvent::MouseButtonRelease)
    {
        if(press || hover_index_row == index.row())
        {
            press = false;
            model->setData(index, false, EM_TITLE_ROLE_HOVER);
        }
    }
    else if(event->type() == QEvent::MouseMove)
    {
        if(press && hover_index_row != index.row())
        {
            if(EM_ITEM_TYPE_TITLE == itemType)
            {
                model->setData(model->index(hover_index_row, 0), false, EM_TITLE_ROLE_HOVER);
            }

            press = false;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
