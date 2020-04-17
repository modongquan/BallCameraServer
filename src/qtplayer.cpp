#include "qtplayer.h"
#include <QPainter>
#include <QDebug>
#include <QString>

QtPlayer::QtPlayer():PlayerTimer(this), PlayCounter(0)
{
    QRect rect = geometry();
    rect.setWidth(640);
    rect.setHeight(480);
    setGeometry(rect);
    rect = geometry();

    connect(&PlayerTimer, SIGNAL(timeout()), this, SLOT(PlayerProc()));
    PlayerTimer.start(10);
}

void QtPlayer::paintEvent(QPaintEvent *e)
{
    if(GetRgb == nullptr) return;

    QImage DispImg(640, 480, QImage::Format_RGB32);

    int32_t rd_size = GetRgb(DispImg.bits(), DispImg.sizeInBytes());
    if(rd_size <= 0)
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
