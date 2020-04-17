#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QOpenGLWidget>
#include <QTimer>

typedef int32_t (*FuncCB)(uint8_t *, uint32_t);

class QtPlayer : public QOpenGLWidget
{
    Q_OBJECT
public:
    QtPlayer();

    FuncCB GetRgb;

private slots:
    void PlayerProc();

private:
    void paintEvent(QPaintEvent *e) override;
//    void paintGL() override;

    QTimer PlayerTimer;
    uint32_t PlayCounter;
    uint8_t RgbData[1920*1080*4 + 54];
};

#endif // QTPLAYER_H
