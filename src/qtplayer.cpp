#include "qtplayer.h"
#include <QPainter>
#include <QDebug>
#include <QString>

QtPlayer::QtPlayer():PlayerTimer(this), PlayCounter(0)
{
    QRect rect = geometry();
    rect.setWidth(1920);
    rect.setHeight(1080);
    setGeometry(rect);
    rect = geometry();

    connect(&PlayerTimer, SIGNAL(timeout()), this, SLOT(PlayerProc()));
    PlayerTimer.start(30);
}

void QtPlayer::paintEvent(QPaintEvent *e)
{
    if(GetRgb == nullptr) return;

    uint32_t w = 0, h = 0;

    if(!GetRgb(nullptr, &w, &h))
    {
        return;
    }
    if(w == 0 || h == 0) return;

    QImage DispImg(w, h, QImage::Format_RGB32);
    if(!GetRgb(DispImg.bits(), nullptr, nullptr))
    {
        return;
    }

    QPainter painter;
    painter.begin(this);
    painter.drawImage(QPoint(0,0), DispImg);
    painter.end();
}

//void QtPlayer::paintGL()
//{

//}

void QtPlayer::PlayerProc()
{
    if(isVisible())
    {
        update();
    }
    else
    {
        show();
    }
}
