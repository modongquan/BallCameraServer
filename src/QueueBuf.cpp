#include "QueueBuf.h"

#define QUEUE_BUF_MAX_NUM   256

typedef struct
{
    uint8_t *buf;
    uint32_t size;
}StruDefQueueBuf;

static StruDefQueueBuf QueueBuf[QUEUE_BUF_MAX_NUM];
static int32_t QueueBufIdx = 0;

int32_t QueueBufRegister(uint8_t *buf, uint32_t size)
{
    if(buf == nullptr || size == 0 || (QueueBufIdx + 1 >= QUEUE_BUF_MAX_NUM)) return -1;

    int32_t idx = QueueBufIdx;
    QueueBuf[idx].buf = buf;
    QueueBuf[idx].size = size;
    QueueBufIdx = idx + 1;

    return idx;
}
