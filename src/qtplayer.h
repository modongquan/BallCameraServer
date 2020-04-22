#ifndef QTPLAYER_H
#define QTPLAYER_H

#include <QOpenGLWidget>
#include <QTimer>
#include <vector>

#include "EventParser.h"

typedef int32_t (*FuncCB)(uint8_t *, uint32_t);

class QtPlayer : public QOpenGLWidget
{
    Q_OBJECT
public:
    QtPlayer();

    FuncCB GetRgb;
     std::vector<StruDefCoordinate> coordinates;

private slots:
    void PlayerProc();

private:
    void paintEvent(QPaintEvent *e) override;
//    void paintGL() override;

    QTimer PlayerTimer;
};

#endif // QTPLAYER_H
