//
// Created by cc on 2018/11/16.
//

#include "TitleFrm.h"
#include "PictureBrowser.h"
#include "../include/Line.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QSplitter>
#include <QMouseEvent>
#include "../UICom/uicom.h"

#define BTN_SIZE 30

TitleFrm::TitleFrm(PictureBrowser* pPicBrowser)
    :QFrame(), _pPicBrowser(pPicBrowser)
{
    initUi();
}


void TitleFrm::initUi()
{
    this->setObjectName("TitleFrame");
    this->setFixedHeight(50);

    _pTurnBeforeBtn = new QToolButton(this);
    _pTurnNextBtn = new QToolButton(this);
    _pEnlargeBtn = new QToolButton(this);
    _pNarrowBtn = new QToolButton(this);
    _pOne2OneBtn = new QToolButton(this);
    _pRotateBtn = new QToolButton(this);
    _pSaveAsBtn = new QToolButton(this);
    _pMinBtn = new QToolButton(this);
    _pMaxBtn = new QToolButton(this);
    _pRestBtn = new QToolButton(this);
    _pCloseBtn = new QToolButton(this);

    _pTurnBeforeBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pTurnNextBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pEnlargeBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pNarrowBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pOne2OneBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pRotateBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pSaveAsBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pMinBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pMaxBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pRestBtn->setFixedSize(BTN_SIZE, BTN_SIZE);
    _pCloseBtn->setFixedSize(BTN_SIZE, BTN_SIZE);

    _pTurnBeforeBtn->setObjectName("TurnLeftBtn");
    _pTurnNextBtn->setObjectName("TurnRightBtn");
    _pEnlargeBtn->setObjectName("EnlargeBtn");
    _pNarrowBtn->setObjectName("NarrowBtn");
    _pOne2OneBtn->setObjectName("One2OneBtn");
    _pRotateBtn->setObjectName("RotateBtn");
    _pSaveAsBtn->setObjectName("SaveAsBtn");
    _pMinBtn->setObjectName("MinBtn");
    _pMaxBtn->setObjectName("MaxBtn");
    _pRestBtn->setObjectName("RestBtn");

    dingBtn = new QToolButton(this);
    dingBtn->setFixedSize(BTN_SIZE - 5, BTN_SIZE - 5);
    dingBtn->setObjectName("DingBtn");
    dingBtn->setToolTip(tr("钉"));
    dingBtn->setCheckable(true);

    _pTurnBeforeBtn->setToolTip(tr("上一张"));
    _pTurnNextBtn->setToolTip(tr("下一张"));
    _pEnlargeBtn->setToolTip(tr("放大"));
    _pNarrowBtn->setToolTip(tr("缩小"));
    _pOne2OneBtn->setToolTip(tr("1:1显示"));
    _pRotateBtn->setToolTip(tr("旋转"));
    _pSaveAsBtn->setToolTip(tr("另存为"));
    _pMinBtn->setToolTip(tr("最小化"));
    _pMaxBtn->setToolTip(tr("最大化"));
    _pRestBtn->setToolTip(tr("恢复窗口"));
    _pCloseBtn->setToolTip(tr("关闭"));

	_pMinBtn->setVisible(false);
	_pMaxBtn->setVisible(false);
	_pRestBtn->setVisible(false);
	_pRestBtn->setVisible(false);

    auto *topLayout = new QHBoxLayout();
#ifdef _MACOS
    _pCloseBtn->setFixedSize(12, 12);
    _pCloseBtn->setObjectName("gmCloseBtn");
    topLayout->addItem(new QSpacerItem(8, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));
    topLayout->addWidget(_pCloseBtn);
    topLayout->addItem(new QSpacerItem(8, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));
    topLayout->addWidget(new Line(Qt::Vertical, this));
#endif

    topLayout->addWidget(dingBtn);
    topLayout->addWidget(new Line(Qt::Vertical, this));

    topLayout->addWidget(_pTurnBeforeBtn);
    topLayout->addWidget(_pTurnNextBtn);
    //topLayout->addWidget(new Line(this));
    topLayout->addWidget(_pEnlargeBtn);
    topLayout->addWidget(_pNarrowBtn);
    topLayout->addWidget(_pOne2OneBtn);
    topLayout->addWidget(new Line(Qt::Vertical, this));
    topLayout->addWidget(_pRotateBtn);
    topLayout->addWidget(_pSaveAsBtn);
    topLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    topLayout->addWidget(_pMinBtn);
    topLayout->addWidget(_pMaxBtn);
    topLayout->addWidget(_pRestBtn);
#ifndef _MACOS
    _pCloseBtn->setObjectName("gwCloseBtn");
    topLayout->addWidget(_pCloseBtn);
#endif
    setLayout(topLayout);
    // todo
    _pMinBtn->setVisible(false);
    _pMaxBtn->setVisible(false);
    _pRestBtn->setVisible(false);

    connect(_pTurnBeforeBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::turnBefore);
    connect(_pTurnNextBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::turnNext);

    connect(_pCloseBtn, &QToolButton::clicked, _pPicBrowser, [this](){
        _pPicBrowser->setVisible(false);
        _pPicBrowser->onCloseWnd();
    });
    connect(_pMinBtn, &QToolButton::clicked, [this]()
    {
        if(nullptr != _pPicBrowser)
        {
            _pPicBrowser->showMinimized();
        }
    });
    connect(_pMaxBtn, &QToolButton::clicked, [this]()
    {
        if(nullptr != _pPicBrowser)
        {
            _pMaxBtn->setVisible(false);
            _pRestBtn->setVisible(true);
            _pPicBrowser->showMaximized();
        }
    });
    connect(_pRestBtn, &QToolButton::clicked, [this]()
    {
        if(nullptr != _pPicBrowser)
        {
            _pMaxBtn->setVisible(true);
            _pRestBtn->setVisible(false);
            _pPicBrowser->showNormal();
        }
    });

    connect(_pEnlargeBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::enlargeSignal);
    connect(_pNarrowBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::narrowSignal);
    connect(_pOne2OneBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::one2oneSiganl);
    connect(_pRotateBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::rotateSiganl);
    connect(_pSaveAsBtn, &QToolButton::clicked, _pPicBrowser, &PictureBrowser::saveAsSignal);

    connect(dingBtn, &QToolButton::clicked, [this](bool checked){
        _pPicBrowser->setWindowFlag(Qt::X11BypassWindowManagerHint, checked);
        _pPicBrowser->setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        _pPicBrowser->setVisible(true);
    });
}

void TitleFrm::setBeforeBtnEnable(bool enable) {
    _pTurnBeforeBtn->setEnabled(enable);
}

void TitleFrm::setNextBtnEnable(bool enable) {

    _pTurnNextBtn->setEnabled(enable);
}
