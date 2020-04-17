#ifndef RTMPSRV_H
#define RTMPSRV_H

#include <stdint.h>
#include <stdlib.h>

typedef enum
{
    video_param = 0,
    video_data,
    invalid
}CallBackType;

typedef void (*GetVideoParamCB)(uint8_t **, uint32_t *, uint8_t **, uint32_t *);
typedef void (*GetVideoDataCB)(uint8_t **, uint32_t *);

int RtmpServerStart(void);
void SetCallBackFunc(CallBackType type, void *cb);

#endif // RTMPSRV_H
