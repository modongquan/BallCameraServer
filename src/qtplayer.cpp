#include "qtplayer.h"
#include <QPainter>
#include <QDebug>
#include <QString>

#pragma pack(1)
typedef struct tagBITMAPFILEHEADER
{
    uint16_t bfType; //2 位图文件的类型，必须为“BM”
    uint32_t bfSize; //4 位图文件的大小，以字节为单位
    uint16_t bfReserved1; //2 位图文件保留字，必须为0
    uint16_t bfReserved2; //2 位图文件保留字，必须为0
    uint32_t bfOffBits; //4 位图数据的起始位置，以相对于位图文件头的偏移量表示，以字节为单位
} BITMAPFILEHEADER;//该结构占据14个字节。
// printf("%d\n",sizeof(BITMAPFILEHEADER));

typedef struct tagBITMAPINFOHEADER{
    uint32_t biSize; //4 本结构所占用字节数
    int32_t biWidth; //4 位图的宽度，以像素为单位
    int32_t biHeight; //4 位图的高度，以像素为单位
    uint16_t biPlanes; //2 目标设备的平面数不清，必须为1
    uint16_t biBitCount;//2 每个像素所需的位数，必须是1(双色), 4(16色)，8(256色)或24(真彩色)之一
    uint32_t biCompression; //4 位图压缩类型，必须是 0(不压缩),1(BI_RLE8压缩类型)或2(BI_RLE4压缩类型)之一
    uint32_t biSizeImage; //4 位图的大小，以字节为单位
    int32_t biXPelsPerMeter; //4 位图水平分辨率，每米像素数
    int32_t biYPelsPerMeter; //4 位图垂直分辨率，每米像素数
    uint32_t biClrUsed;//4 位图实际使用的颜色表中的颜色数
    uint32_t biClrImportant;//4 位图显示过程中重要的颜色数
} BITMAPINFOHEADER;//该结构占据40个字节。
#pragma end

uint32_t QtPlayer::FillBmpHead(uint8_t *pHeadBuf, int32_t width, int32_t height, uint16_t bpp)
{
    BITMAPFILEHEADER bmpheader;
    BITMAPINFOHEADER bmpinfo;

    bmpheader.bfType = 0x4d42;
    bmpheader.bfReserved1 = 0;
    bmpheader.bfReserved2 = 0;
    bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp/8;

    bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
    bmpinfo.biWidth = width;
    bmpinfo.biHeight = height;
    bmpinfo.biPlanes = 1;
    bmpinfo.biBitCount = bpp;
    bmpinfo.biCompression = 0;
    uint32_t line_size = width * bpp / 8;
    if(line_size / 4 * 4 != line_size)
    {
        line_size = (line_size / 4 + 1) * 4;
    }
    bmpinfo.biSizeImage = line_size*height;
    bmpinfo.biXPelsPerMeter = 100;
    bmpinfo.biYPelsPerMeter = 100;
    bmpinfo.biClrUsed = 0;
    bmpinfo.biClrImportant = 0;

    memcpy(pHeadBuf, &bmpheader, sizeof(bmpheader));
    memcpy(pHeadBuf + sizeof(bmpheader), &bmpinfo, sizeof(bmpinfo));

    return bmpheader.bfOffBits;
}

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

#if 1
    QImage DispImg(w, h, QImage::Format_RGB32);
    if(!GetRgb(DispImg.bits(), nullptr, nullptr))
    {
        return;
    }
#else
    uint32_t offset = FillBmpHead(RgbData, w, h, 32);
    if(!GetRgb(static_cast<uint8_t *>(RgbData + offset), nullptr, nullptr))
    {
        return;
    }

    QImage DispImg;
    DispImg.loadFromData(RgbData, w * h * 4 + offset);
#endif

#if 0
    QImage DispImg2;
    QString FileName = QString("test_") + QString::number(PlayCounter++) + QString(".bmp");
    DispImg2.load(FileName);
    qDebug() << DispImg2.format() << endl;
#endif

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
