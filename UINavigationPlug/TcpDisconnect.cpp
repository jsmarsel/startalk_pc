#include "TcpDisconnect.h"
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>

TcpDisconnect::TcpDisconnect(SessionFrm *pMainPanel, QWidget *parent)
    : QFrame(parent), _pMainPanel(pMainPanel)
{
    setFrameShape(QFrame::NoFrame);
    setObjectName("TcpDisconnectWgt");

    QPixmap pic(":/UINavigationPlug/image1/error.png");
    QLabel *picLabel = new QLabel;
    picLabel->setPixmap(pic);
    _pTextLabel = new QLabel(tr("当前网络不可用"));
    auto *layout = new QHBoxLayout(this);
    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    layout->addWidget(picLabel);
    layout->addWidget(_pTextLabel);
    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
    setLayout(layout);

    connect(this, &TcpDisconnect::sgSetText, this, &TcpDisconnect::setText);
}

TcpDisconnect::~TcpDisconnect() = default;

/**
  * @函数名   onRetryConnected
  * @功能描述 
  * @参数
  * @author   cc
  * @date     2018/10/24
  */
void TcpDisconnect::onRetryConnected()
{
    _pTextLabel->setText(tr("当前网络不可用"));
}

void TcpDisconnect::setText(const QString &text)
{
    _pTextLabel->setText(text);
}