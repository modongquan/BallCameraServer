#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QOpenGLWidget>
#include <QTimer>

typedef bool (*FuncCB)(uint8_t *, uint32_t *, uint32_t *);

class QtPlayer : public QOpenGLWidget
{
    Q_OBJECT
public:
    QtPlayer();

    FuncCB GetRgb;

private slots:
    void PlayerProc();

private:
    uint32_t FillBmpHead(uint8_t *pHeadBuf, int32_t width, int32_t height, uint16_t bpp);

    void paintEvent(QPaintEvent *e) override;
//    void paintGL() override;

    QTimer PlayerTimer;
    uint32_t PlayCounter;
    uint8_t RgbData[1920*1080*4 + 54];
};

#endif // QTPLAYER_H
