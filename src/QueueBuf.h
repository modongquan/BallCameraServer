#ifndef QUEUE_BUF_H
#define QUEUE_BUF_H

#include <stdint.h>

int32_t QueueBufInit(uint32_t buf_num);
int32_t QueueBufRegister(uint8_t *buf, uint32_t size, uint32_t data_seg_num, uint32_t sub_seg_num);
void QueueBufFree();

int32_t CheckWrDataSeg(uint32_t idx, uint32_t wr_size);
int32_t CheckWrSubDataSeg(uint32_t idx, uint32_t wr_size);
void InsertData(uint32_t idx, uint8_t *pData, uint32_t size);
void SetNextWrBuf(uint32_t idx);

uint32_t GetRdDataSize(uint32_t idx);
int32_t ReadData(uint32_t idx, uint8_t *pOut);
#endif
