#if _MSC_VER >= 1600
    #pragma execution_character_set("utf-8")
#endif
#ifndef SEARCHPANEL_H
#define SEARCHPANEL_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include "../CustomUi/UCButton.h"
#include "../CustomUi/UShadowWnd.h"
#include "MessageManager.h"
#include "../UICom/UIEntity.h"
#include "SearchView.h"

class SearchThread;

class SearchResultPanel : public UShadowDialog
{
    Q_OBJECT
public:
    explicit SearchResultPanel(QWidget *parent);
    ~SearchResultPanel() override;

public:
    void initPanel();
    void closeSearch();

public slots:
    void onFunSelect(const int &index);
    void onSearchStart(const QString &keywords);
    void onSearchAll();
    void onSearchContact();
    void onSearchGroup();
    void onSearchHistory();
    void onSearchFile();
    void addSearchReq(const QString &req);
    void onGotSearchResult(const QString &key, const SearchResult &ret);
    void onGetMore(int req);

Q_SIGNALS:
    void sgOpenNewSession(const StSessionInfo &into);
    void sgOpenSearch(bool);
    void sgSetEditFocus();
    void sgSelectUp();
    void sgSelectDown();
    void sgSelectItem();
    void sgGotSearchResult(const QString &key, const SearchResult &ret);
    void sgShowMessageRecordWnd(const QString &, const QString &);
    void sgShowFileRecordWnd(const QString &);

private:
    SearchView *_pSearchView {};
    UCButton  *_allBtn {};        // 全部
    UCButton  *_contactBtn {};    // 联系人
    UCButton  *_groupChatBtn {};  // 群聊
    UCButton  *_chatRecordBtn {}; // 聊天记录
    UCButton  *_fileBtn {}; // 文件
    UCButtonGroup *_tabGroup {};
    QLabel       *_pWnLabel {};

    QString _keywords; // 搜索关键字
    int _reqType;
    int _searchStart;
    int _searchLength;
    int _action;
    bool isGetMore;

private:
    SearchThread *_searchThread{};

};

#endif // SEARCHPANEL_H
