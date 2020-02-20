#include "QueueBuf.h"
#include <malloc.h>
#include <string.h>

typedef struct
{
    uint8_t *pDataSeg;
    uint32_t size;
}StruDefDataSeg;

typedef struct
{
    uint8_t *start;
    uint8_t *end;
    uint32_t size;

    StruDefDataSeg *pQueueDataSeg;
    uint32_t QueueDataSegNum;
    volatile uint32_t SegWrIdx;
    volatile uint32_t SegRdIdx;

    StruDefDataSeg *pSubQueue;
    uint32_t SubQueueSegNum;
    volatile uint32_t SubSegWrIdx;
}StruDefQueueBuf;

static StruDefQueueBuf *pQueueBuf;
static uint32_t QueueBufIdx = 0;
static uint32_t QueueBufNum = 0;

int32_t QueueBufInit(uint32_t buf_num)
{
    pQueueBuf = (StruDefQueueBuf *)malloc(buf_num * sizeof(StruDefQueueBuf));
    if(pQueueBuf == nullptr) return -1;

    QueueBufNum = buf_num;

    return 0;
}

int32_t QueueBufRegister(uint8_t *buf, uint32_t size, uint32_t data_seg_num, uint32_t sub_seg_num)
{
    if(buf == nullptr || size == 0 || (QueueBufIdx >= QueueBufNum)) return -1;

    uint32_t idx = QueueBufIdx;

    pQueueBuf[idx].pQueueDataSeg = (StruDefDataSeg *)malloc(data_seg_num * sizeof(StruDefDataSeg));
    if(pQueueBuf[idx].pQueueDataSeg == nullptr) return -1;
    pQueueBuf[idx].QueueDataSegNum = data_seg_num;
    pQueueBuf[idx].SegWrIdx = 0;
    pQueueBuf[idx].SegRdIdx = 0;

    pQueueBuf[idx].pSubQueue = (StruDefDataSeg *)malloc(sub_seg_num * sizeof(StruDefDataSeg));
    if(pQueueBuf[idx].pSubQueue == nullptr) return -1;
    pQueueBuf[idx].SubQueueSegNum = sub_seg_num;
    pQueueBuf[idx].SubSegWrIdx = 0;

    pQueueBuf[idx].start = buf;
    pQueueBuf[idx].end = buf + size;
    pQueueBuf[idx].size = size;
    QueueBufIdx = idx + 1;

    memset((void *)(pQueueBuf[idx].pQueueDataSeg), 0, data_seg_num * sizeof(StruDefDataSeg));
    pQueueBuf[idx].pQueueDataSeg[0].pDataSeg = pQueueBuf[idx].start;

    memset((void *)(pQueueBuf[idx].pSubQueue), 0, sub_seg_num * sizeof(StruDefDataSeg));
    pQueueBuf[idx].pSubQueue[0].pDataSeg = pQueueBuf[idx].start;

    printf("queue buf %d(%p - %p) \n", idx, pQueueBuf[idx].start, pQueueBuf[idx].end);
    fflush(stdout);

    return idx;
}

void QueueBufFree()
{
    if(pQueueBuf)
    {
        for(uint32_t i = 0;i < QueueBufIdx;i++)
        {
            if(pQueueBuf[i].pQueueDataSeg) free(pQueueBuf[i].pQueueDataSeg);
            if(pQueueBuf[i].pSubQueue) free(pQueueBuf[i].pSubQueue);
        }
        free(pQueueBuf);
    }
}

int32_t CheckWrDataSeg(uint32_t idx, uint32_t wr_size)
{
    uint32_t wr_idx = pQueueBuf[idx].SegWrIdx;
    bool is_skip_wr = false;

    do{
        uint32_t rd_idx = pQueueBuf[idx].SegRdIdx;
        uint8_t *pWr = pQueueBuf[idx].pQueueDataSeg[wr_idx].pDataSeg;
        uint8_t *pRd = pQueueBuf[idx].pQueueDataSeg[rd_idx].pDataSeg;

        uint32_t diff = static_cast<uint32_t>((pWr >= pRd)?(pWr - pRd):(pWr + pQueueBuf[idx].size - pRd));
        if(diff + wr_size > pQueueBuf[idx].size)
        {
            printf("queue buf %d : write the current frame to preview data segment \n", idx);
            fflush(stdout);
            is_skip_wr = true;

            wr_idx = (wr_idx > 0)?(wr_idx - 1):(pQueueBuf[idx].QueueDataSegNum - 1);
            if(wr_idx == pQueueBuf[idx].SegRdIdx)
            {
                printf("skip the data to avoid data buffer overwrite \n");
                fflush(stdout);
                return false;
            }
            continue;
        }
        break;
    }while(1);

    if(is_skip_wr)
    {
        pQueueBuf[idx].pQueueDataSeg[wr_idx].size = 0;
        pQueueBuf[idx].SegWrIdx = wr_idx;

        memset((void *)(pQueueBuf[idx].pSubQueue), 0, pQueueBuf[idx].SubQueueSegNum * sizeof(StruDefDataSeg));
        pQueueBuf[idx].SubSegWrIdx = 0;
        pQueueBuf[idx].pSubQueue[0].pDataSeg = pQueueBuf[idx].pQueueDataSeg[wr_idx].pDataSeg;
    }

    return 0;
}

int32_t CheckWrSubDataSeg(uint32_t idx, uint32_t wr_size)
{
    uint32_t rd_idx = pQueueBuf[idx].SegRdIdx;
    uint32_t sub_wr_idx = pQueueBuf[idx].SubSegWrIdx;
    uint8_t *pWr = pQueueBuf[idx].pSubQueue[sub_wr_idx].pDataSeg;
    uint8_t *pRd = pQueueBuf[idx].pQueueDataSeg[rd_idx].pDataSeg;
    bool is_skip_wr = false;

    do{
        uint32_t diff = static_cast<uint32_t>((pWr >= pRd)?(pWr - pRd):(pWr + pQueueBuf[idx].size - pRd));
        if(diff + wr_size > pQueueBuf[idx].size)
        {
            printf("queue buf %d : write the current frame to preview sub data segment \n", idx);
            fflush(stdout);
            is_skip_wr = true;

            if(sub_wr_idx > 1)
            {
                sub_wr_idx--;
            }
            else
            {
                printf("skip the data to avoid data buffer overwrite \n");
                fflush(stdout);
                return -1;
            }
            pWr = pQueueBuf[idx].pSubQueue[sub_wr_idx].pDataSeg;
            continue;
        }
        break;
    }while(1);

    if(is_skip_wr)
    {
        pQueueBuf[idx].SubSegWrIdx = sub_wr_idx;
        pQueueBuf[idx].pSubQueue[sub_wr_idx].size = 0;
    }

    return 0;
}

void InsertData(uint32_t idx, uint8_t *pData, uint32_t size)
{
    uint32_t sub_wr_idx = pQueueBuf[idx].SubSegWrIdx;
    uint8_t *pWr = pQueueBuf[idx].pSubQueue[sub_wr_idx].pDataSeg;
    uint8_t *pStart = pWr;

    if(pWr + size < pQueueBuf[idx].end)
    {
        memcpy(pWr, pData, size);
        pWr += size;
    }
    else
    {
        uint32_t cpy_size = static_cast<uint32_t>(pQueueBuf[idx].end - pWr);
        memcpy(pWr, pData, cpy_size);
        memcpy(pQueueBuf[idx].start, pData + cpy_size, size - cpy_size);
        pWr = pWr + size - pQueueBuf[idx].size;
    }

    pQueueBuf[idx].pSubQueue[sub_wr_idx].size = size;
    if(++sub_wr_idx >= pQueueBuf[idx].SubQueueSegNum) sub_wr_idx = 0;
    pQueueBuf[idx].pSubQueue[sub_wr_idx].pDataSeg = pWr;
    pQueueBuf[idx].SubSegWrIdx = sub_wr_idx;
//    printf("insert data : (%p - %p) \n", pStart, pWr);
//    fflush(stdout);
}

void SetNextWrBuf(uint32_t idx)
{
    uint32_t wr_idx = pQueueBuf[idx].SegWrIdx;
    uint32_t sub_wr_idx = pQueueBuf[idx].SubSegWrIdx;
    uint8_t *pWrStart = pQueueBuf[idx].pQueueDataSeg[wr_idx].pDataSeg;
    uint8_t *pWrEnd = pQueueBuf[idx].pSubQueue[sub_wr_idx].pDataSeg;
    uint32_t wr_size;

    if(pWrEnd >= pWrStart)
    {
        wr_size = static_cast<uint32_t>(pWrEnd - pWrStart);
    }
    else
    {
        wr_size = static_cast<uint32_t>(pQueueBuf[idx].end - pWrStart + pWrEnd - pQueueBuf[idx].start);
    }
    pQueueBuf[idx].pQueueDataSeg[wr_idx].size = wr_size;

    uint32_t wr_next_idx = (wr_idx + 1 < pQueueBuf[idx].QueueDataSegNum)?(wr_idx + 1):0;

    pQueueBuf[idx].pQueueDataSeg[wr_next_idx].pDataSeg = pWrEnd;
    pQueueBuf[idx].pQueueDataSeg[wr_next_idx].size = 0;
    pQueueBuf[idx].SegWrIdx = wr_next_idx;

    memset((void *)(pQueueBuf[idx].pSubQueue), 0, pQueueBuf[idx].SubQueueSegNum * sizeof(StruDefDataSeg));
    pQueueBuf[idx].SubSegWrIdx = 0;
    pQueueBuf[idx].pSubQueue[0].pDataSeg = pWrEnd;

//    printf("wr data buf = (%p - %p) \n", pWrStart, pWrEnd);
//    fflush(stdout);
}

uint32_t GetRdDataSize(uint32_t idx)
{
    uint32_t rd_idx =  pQueueBuf[idx].SegRdIdx;
    uint32_t rd_size = pQueueBuf[idx].pQueueDataSeg[rd_idx].size;

    return rd_size;
}

int32_t ReadData(uint32_t idx, uint8_t *pOut)
{
    if(!pOut) return -1;


    uint32_t rd_idx = pQueueBuf[idx].SegRdIdx;
    uint32_t wr_idx = pQueueBuf[idx].SegWrIdx;

    if(rd_idx == pQueueBuf[idx].SegWrIdx)
    {
        return -1;
    }

//    uint32_t diff = (wr_idx >= rd_idx)?(wr_idx - rd_idx):(wr_idx + pQueueBuf[idx].QueueDataSegNum - rd_idx);
//    if(diff >= 2)
//    {

//    }

    if(pQueueBuf[idx].pQueueDataSeg[rd_idx].size == 0)
    {
        return -1;
    }

    uint8_t *pData = pQueueBuf[idx].pQueueDataSeg[rd_idx].pDataSeg;
    uint32_t rd_size = pQueueBuf[idx].pQueueDataSeg[rd_idx].size;
    if(pData + rd_size <= pQueueBuf[idx].end)
    {
        memcpy(pOut, pData, rd_size);
    }
    else
    {
        size_t cpy_size = static_cast<size_t>(pQueueBuf[idx].end - pData);
        memcpy(pOut, pData, cpy_size);
        memcpy(pOut + cpy_size, pQueueBuf[idx].start, rd_size - cpy_size);
    }

//    printf("rd data buf = (%p - %p) \n",static_cast<void *>(pData), pQueueBuf[idx].pQueueDataSeg[rd_idx + 1].pDataSeg);
//    fflush(stdout);

    pQueueBuf[idx].pQueueDataSeg[rd_idx].pDataSeg = nullptr;
    pQueueBuf[idx].pQueueDataSeg[rd_idx].size = 0;

    if(++rd_idx >= pQueueBuf[idx].QueueDataSegNum)
    {
        rd_idx = 0;
    }
    pQueueBuf[idx].SegRdIdx = rd_idx;

    return 0;
}
