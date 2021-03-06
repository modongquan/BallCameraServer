#include "qtplayer.h"
#include <QPainter>
#include <QDebug>
#include <QString>

QtPlayer::QtPlayer():PlayerTimer(this)
{
    width = 640;
    height = 480;
    QRect rect = geometry();
    rect.setWidth(width);
    rect.setHeight(height);
    setGeometry(rect);
    rect = geometry();

    connect(&PlayerTimer, SIGNAL(timeout()), this, SLOT(PlayerProc()));
    PlayerTimer.start(40);
}

void QtPlayer::paintEvent(QPaintEvent *e)
{
    if(GetRgb == nullptr) return;

    QImage DispImg(width, height, QImage::Format_RGB32);

    int32_t rd_size = GetRgb(DispImg.bits(), DispImg.sizeInBytes());
    if(rd_size <= 0)
    {
        return;
    }

    QPainter painter;
    painter.begin(this);
    painter.drawImage(QPoint(0,0), DispImg);

    QPen draw_pen;
    draw_pen.setWidth(3);
    for(uint32_t i = 0;i < coordinates.size();i ++)
    {
        int32_t x = coordinates[i].left_top_x;
        int32_t y = coordinates[i].left_top_y;
        int32_t w = abs(x - coordinates[i].right_bottom_x);
        int32_t h = abs(y - coordinates[i].right_bottom_y);
        x = x * width / 1920;
        y = y * height / 1080;
        w = w * width / 1920;
        h = h * height / 1080;
        if(!coordinates[i].type)
        {
            draw_pen.setColor(QColor(Qt::green));
        }
        else
        {
            draw_pen.setColor(QColor(Qt::red));
        }
        painter.setPen(draw_pen);
        painter.drawRect(x, y, w, h);
    }
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
