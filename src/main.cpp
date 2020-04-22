#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <vector>
#include <math.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <QApplication>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

#include "SDS_Device_DataDefine.h"
#include "SDS_Device_Api.h"
#include "cms_errcode.h"
#include "qtplayer.h"
#include "QueueBuf.h"
#include "EventParser.h"
#include "rtmpsrv.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/mem.h"
#include "libavutil/hwcontext.h"
#include "libavutil/opt.h"
#include "libavformat/avio.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/error.h"

#ifdef __cplusplus
}
#endif

using namespace  SDS_Device::sdk;
using namespace sds::error;
using namespace SDS_Device::enums;

//#define HARDWARE_ENCODE_DEF
#define LOCAL_PLAY_DEF

#define EVENT_TYPE_NUM  (NETEVENT_DOWNLOADING_FILE + 1)
#define DEVICE_ID_SIZE  32

#define DECODE_CACHE_FRAME_NUM  10

#define H264_DATA_BUF_SIZE  (10*1024*1024)
#define YUV_DATA_BUF_SIZE   (300*1024*1024)
#define DISP_DATA_BUF_SIZE  (50*1024*1024)
#define RTMP_DATA_BUF_SIZE  (1024*1024)
#define I_FRAME_CACHE_SIZE  (300*1024)

#define READ_ONE_FRAME_SIZE (10*1024*1024)
#define READ_ONE_GOP_SIZE   (1024*1024)
#define READ_ONE_RTMP_SIZE  (200*1024)

#define SPS_DATA_BUF_SIZE   256
#define PPS_DATA_BUF_SIZE   256

#define RGB_DATA_QUEUE_NUM  256
#define COORDINATES_MAX_NUM 64

typedef void (*EventHandleDef)(const char*);

EventHandleDef EventHdl[EVENT_TYPE_NUM];
char DeviceId[DEVICE_ID_SIZE];
uint32_t DevIdIndex = 0;
bool is_dev_ready = false;
_handle StreamHdl;

uint8_t H264DataBuf[H264_DATA_BUF_SIZE];
int32_t H264DataIdx = 0;
uint8_t H264ReadBuf[READ_ONE_GOP_SIZE];
bool is_first_iframe_recv = false;
bool is_skip_frame = false;

uint8_t YuvDataBuf[YUV_DATA_BUF_SIZE];
int32_t YuvDataIdx = 0;
uint8_t YuvBuf[READ_ONE_FRAME_SIZE];

uint8_t RtmpDataBuf[RTMP_DATA_BUF_SIZE];
int32_t RtmpDataIdx = 0;
uint8_t RtmpReadBuf[READ_ONE_RTMP_SIZE];

QtPlayer *pVideoPlayer;
int64_t encode_idx = 0;

uint8_t DispDataBuf[DISP_DATA_BUF_SIZE];
uint8_t RgbBuf[READ_ONE_FRAME_SIZE];
int32_t DispDataIdx = 0;

uint8_t SpsBuf[SPS_DATA_BUF_SIZE];
volatile uint32_t SpsSize = 0;

uint8_t PpsBuf[PPS_DATA_BUF_SIZE];
volatile uint32_t PpsSize = 0;

uint8_t IFrameCache[I_FRAME_CACHE_SIZE];
uint32_t IFrameSize = 0;

pthread_mutex_t video_param_mtx;
bool is_key_frame = false;

char test[32] = {"test_"};
char jpg[32] = {".jpg"};

uint32_t gop_cnt = 0;
struct timeval start_time, end_time;

AVCodecContext *pDecodecCtx;
const AVCodec *pDecodec;
AVCodecParserContext *pDecParserCtx;
AVFrame *pDecFrame;
AVFrame *pFiltFrame;
AVPacket *pDecPkt;

AVCodecContext *pEncodecCtx;
const AVCodec *pEncodec;
AVBufferRef *pHwDevCtx = nullptr;
AVFrame *pEncFrame;
#ifdef HARDWARE_ENCODE_DEF
AVFrame *pEncHwFrame;
#endif
AVPacket *pEncPkt;


void PlayMp4File(const char *pFile)
{
    if(!pFile) return;

    int32_t ret = 0;
    int32_t video_idx = -1;
    AVFormatContext *pFmtCtx;
    AVCodecParameters *pCodecParam;
    AVPacket av_pkt;

    pFmtCtx = avformat_alloc_context();

    ret = avformat_open_input(&pFmtCtx, pFile, nullptr, nullptr);
    if(ret < 0)
    {
        printf("avformat_open_input %s fail\n", pFile);
        fflush(stdout);
        return;
    }

    if(avformat_find_stream_info(pFmtCtx, NULL) < 0)
    {
        printf("avformat_find_stream_info fail\n", pFile);
        fflush(stdout);
        return;
    }

    for(uint32_t i = 0;i < pFmtCtx->nb_streams;i ++)
    {
        if(pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_idx = i;
        }
    }
    if(video_idx < 0)
    {
        printf("did not find AVMEDIA_TYPE_VIDEO\n");
        fflush(stdout);
        return;
    }

    pCodecParam = pFmtCtx->streams[video_idx]->codecpar;
    SpsSize = (((uint16_t)(pCodecParam->extradata[6]) << 8) | (pCodecParam->extradata[7]));
    memcpy(SpsBuf, &(pCodecParam->extradata[8]), SpsSize);
    uint32_t off = 6 + sizeof(uint16_t) + SpsSize + 1;
    PpsSize = (((uint16_t)(pCodecParam->extradata[off]) << 8) | (pCodecParam->extradata[off + 1]));
    memcpy(PpsBuf, &(pCodecParam->extradata[off + 2]), PpsSize);

    while(av_read_frame(pFmtCtx, &av_pkt) >= 0)
    {
        if(av_pkt.stream_index == video_idx)
        {
            InsertData(RtmpDataIdx, (uint8_t *)&av_pkt.dts, sizeof(av_pkt.dts));
            InsertData(RtmpDataIdx, av_pkt.data, av_pkt.size);
            SetNextWrBuf(RtmpDataIdx);
            av_packet_unref(&av_pkt);

            usleep(40000);
        }
    }
}

void SaveImageDataToFile(AVFrame *pFrame, const char *pFileOut)
{
    // printf("channel id = %d frame id = %d channel name = %s video type = %d image size = %d\n", VideoImage.video_image_info.channel_id,
    //        VideoImage.video_image_info.frame_id, VideoImage.video_image_info.channel_name, VideoImage.video_image_info.video_type,
    //        VideoImage.img.size);

    AVFormatContext *pFormatCtx = nullptr;
    AVStream *pAVStream = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVCodec *pCodec = nullptr;
    AVPacket pkt;

    do
    {
        // 分配AVFormatContext对象
        pFormatCtx = avformat_alloc_context();

        // 设置输出文件格式
        pFormatCtx->oformat = av_guess_format("mjpeg", nullptr, nullptr);
        // 创建并初始化一个和该url相关的AVIOContext
        if (avio_open(&pFormatCtx->pb, pFileOut, AVIO_FLAG_READ_WRITE) < 0)
        {
            printf("Couldn't open output file \n");
            return;
        }

        // 构建一个新stream
        pAVStream = avformat_new_stream(pFormatCtx, 0);
        if (pAVStream == nullptr)
        {
            printf("avformat_new_stream error \n");
            break;
        }

        // 设置该stream的信息
        pCodecCtx = avcodec_alloc_context3(nullptr);

        pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
        pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        pCodecCtx->pix_fmt = (AVPixelFormat)(pFrame->format);
        pCodecCtx->width = pFrame->width;
        pCodecCtx->height = pFrame->height;
        pCodecCtx->time_base.num = 1;
        pCodecCtx->time_base.den = 25;
        if(pCodecCtx->pix_fmt == AV_PIX_FMT_YUV420P)
        {
            pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        }
        // Begin Output some information
        av_dump_format(pFormatCtx, 0, pFileOut, 1);
        // End Output some information

        // 查找解码器
        pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
        if (!pCodec)
        {
            printf("could not find codec \n");
            break;
        }

        // 设置pCodecCtx的解码器为pCodec
        if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0)
        {
            printf("Could not open codec \n");
            break;
        }

        int y_size = pCodecCtx->width * pCodecCtx->height;

        //Write Header
        int ret = avformat_write_header(pFormatCtx, nullptr);
        if (ret < 0)
        {
            printf("Frame2JPG::avformat_write_header \n");
            break;
        }

        //Encode
        // 给AVPacket分配足够大的空间
        av_new_packet(&pkt, y_size * 3);

        ret = avcodec_send_frame(pCodecCtx, pFrame);
        if (ret < 0)
        {
            printf("avcodec_send_frame err \n");
            break;
        }

        ret = avcodec_receive_packet(pCodecCtx, &pkt);
        if (ret < 0)
        {
            printf("avcodec_receive_packet err \n");
            break;
        }

        ret = av_write_frame(pFormatCtx, &pkt);
        if (ret < 0)
        {
            printf("av_write_frame err \n");
        }

        //Write Trailer
        av_write_trailer(pFormatCtx);
        av_packet_unref(&pkt);
    } while (0);

    if(pAVStream)
    {
        if (pCodecCtx)
        {
            avcodec_close(pCodecCtx);
        }
    }

    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}

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
#pragma pack()
#pragma end
bool CreateBmp(const char *filename, uint8_t *pRGBBuffer, int32_t width, int32_t height, uint16_t bpp)
{
    BITMAPFILEHEADER bmpheader;
    BITMAPINFOHEADER bmpinfo;

    FILE *fp = nullptr;

    fp = fopen(filename,"wb");
    if( fp == nullptr )
    {
        return false;
    }

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

    fwrite(&bmpheader,sizeof(BITMAPFILEHEADER),1,fp);
    fwrite(&bmpinfo,sizeof(BITMAPINFOHEADER),1,fp);
    fwrite(pRGBBuffer,width*height*bpp/8,1,fp);
    fclose(fp);
    fp = nullptr;

    return true;
}

void GetVideoParam(uint8_t *pVideoData, uint32_t size)
{
    if(pVideoData == nullptr) return;

    uint32_t idx = 0;
    uint8_t *pSPSData = nullptr;
    uint32_t sps_size = 0;
    uint8_t *pPPSData = nullptr;
    uint32_t pps_size = 0;
    uint8_t divide_sps_status = 0; /* 0:init 1:counting 2:over */
    uint8_t divide_pps_status = 0; /* 0:init 1:counting 2:over */

    while (idx < size)
    {
        uint32_t divide = *(uint32_t *)(pVideoData + idx);
        if(divide != 0x01000000)
        {
            idx++;
            if(divide_sps_status == 1) sps_size++;
            else if(divide_pps_status == 1) pps_size++;
            continue;
        }
        if(divide_sps_status == 1) divide_sps_status = 2;
        else if(divide_pps_status == 1) divide_pps_status = 2;

        idx += sizeof(uint32_t);
        if((pVideoData[idx] & 0x1f) == 0x7)
        {
            pSPSData = (pVideoData + ++idx);
            sps_size = 1;
            divide_sps_status = 1;
        }
        else if((pVideoData[idx] & 0x1f) == 0x8)
        {
            pPPSData = (pVideoData + ++idx);
            pps_size = 1;
            divide_pps_status = 1;
        }
        else if((pVideoData[idx] & 0x1f) == 0x5)
        {
            is_key_frame = true;
            break;
        }
        else if((pVideoData[idx] & 0x1f) == 0x1)
        {
            is_key_frame = false;
            break;
        }
    }

    pthread_mutex_lock(&video_param_mtx);
    if(divide_pps_status == 2)
    {
        memcpy(PpsBuf, pPPSData, pps_size);
        PpsSize = pps_size;

    }
    if(divide_sps_status == 2)
    {
        memcpy(SpsBuf, pSPSData, sps_size);
        SpsSize = sps_size;
    }
    pthread_mutex_unlock(&video_param_mtx);
}

void GetVideoParamCb(uint8_t **pSpsBuf, uint32_t *pSpsSize, uint8_t **pPpsBuf, uint32_t *pPpsSize)
{
    if(!pSpsBuf || !pSpsSize || !pPpsBuf || !pPpsSize) return;

    pthread_mutex_lock(&video_param_mtx);

    *pPpsBuf = PpsBuf;
    *pPpsSize = PpsSize;

    *pSpsBuf = SpsBuf;
    *pSpsSize = SpsSize;

    pthread_mutex_unlock(&video_param_mtx);
}

void GetVideoDataCb(uint8_t **pDataBuf, uint32_t *pSize)
{
    if(!pDataBuf || !pSize) return;

    uint32_t rd_size = GetRdDataSize(RtmpDataIdx);
    if(rd_size > 0)
    {
        if(!ReadData(RtmpDataIdx, RtmpReadBuf))
        {
            *pDataBuf = RtmpReadBuf;
            *pSize = rd_size;
            return;
        }
    }

    *pDataBuf = nullptr;
    *pSize = 0;
}

int32_t EncodecInit(void)
{
#ifndef HARDWARE_ENCODE_DEF
    pEncodec = avcodec_find_encoder(AV_CODEC_ID_H264);
#else
    int err = av_hwdevice_ctx_create(&pHwDevCtx, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to create a VAAPI device, err = %d\n", err);
        fflush(stderr);
        return -1;
    }

    pEncodec = avcodec_find_encoder_by_name("h264_vaapi");
#endif
    if (!pEncodec) {
        fprintf(stderr, "avcodec find encoder err\n");
        fflush(stderr);
        return -1;
    }

    pEncodecCtx = avcodec_alloc_context3(pEncodec);
    if (!pEncodecCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        fflush(stderr);
        return -1;
    }

    pEncFrame = av_frame_alloc();
    if(!pEncFrame)
    {
        fprintf(stderr, "av_frame_alloc err\n");
        fflush(stderr);
        return -1;
    }

    pEncPkt = av_packet_alloc();
    if(!pEncPkt)
    {
        fprintf(stderr, "av_packet_alloc err\n");
        fflush(stderr);
        return -1;
    }

    return 0;
}

void EncodecDeInit(void)
{
    avcodec_free_context(&pEncodecCtx);
    av_frame_free(&pEncFrame);
#ifdef HARDWARE_ENCODE_DEF
    av_buffer_unref(&pHwDevCtx);
#endif
    av_packet_free(&pEncPkt);
}

int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx)
{
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = nullptr;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
        fflush(stderr);
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = ctx->pix_fmt;
    frames_ctx->width     = ctx->width;
    frames_ctx->height    = ctx->height;
    frames_ctx->initial_pool_size = ctx->gop_size;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize VAAPI frame context.\n");
        fflush(stderr);
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

int32_t EncodecStart(void)
{
    int32_t ret;

    pEncodecCtx->codec_id = AV_CODEC_ID_H264;
    pEncodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

    /* put sample parameters */
    pEncodecCtx->bit_rate = 2*1024*1024;
    /* resolution must be a multiple of two */
    pEncodecCtx->width = pDecodecCtx->width;
    pEncodecCtx->height = pDecodecCtx->height;
    /* frames per second */
    pEncodecCtx->time_base = pDecodecCtx->time_base;
    pEncodecCtx->framerate = pDecodecCtx->framerate;

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    pEncodecCtx->gop_size = pDecodecCtx->gop_size;
    pEncodecCtx->max_b_frames = pDecodecCtx->max_b_frames;
    pEncodecCtx->pix_fmt = pDecodecCtx->pix_fmt;

    //    pEncodecCtx->qmax = 51;
    //    pEncodecCtx->qmin = 11;
    //    pEncodecCtx->slices = 4;

#ifndef HARDWARE_ENCODE_DEF
    if (pEncodec->id == AV_CODEC_ID_H264)
    {
        /*preset参数，画质从差到好（编解码工作量从低到高），分别是：ultrafast、superfast、veryfast、faster、fast、
         * medium、slow、slower、veryslow、placebo（有的版本是不能用的，自己查支持哪些参数*/
        av_opt_set(pEncodecCtx->priv_data, "preset", "slow", 0);
    }
#else
    int err;

    /* set hw_frames_ctx for encoder's AVCodecContext */
    if (set_hwframe_ctx(pEncodecCtx, pHwDevCtx) < 0) {
        fprintf(stderr, "Failed to set hwframe context.\n");
        fflush(stderr);
        return -1;
    }

    if (!(pEncHwFrame = av_frame_alloc())) {
        err = AVERROR(ENOMEM);
        fprintf(stderr, "alloc hardware frame err\n");
        fflush(stderr);
        return -1;
    }
    if ((err = av_hwframe_get_buffer(pEncodecCtx->hw_frames_ctx, pEncHwFrame, 0)) < 0) {
        fprintf(stderr, "av_hwframe_get_buffer");
        fflush(stderr);
        return -1;
    }
    if (!pEncHwFrame->hw_frames_ctx) {
        err = AVERROR(ENOMEM);
        fprintf(stderr, "hardware frame\n");
        fflush(stderr);
        return -1;
    }
#endif

    /* open it */
    ret = avcodec_open2(pEncodecCtx, pEncodec, NULL);
    if (ret < 0) {
        fprintf(stderr, "%s : Could not open codec\n", __FUNCTION__);
        fflush(stderr);
        return -1;
    }

    int32_t y_size = pEncodecCtx->width * pEncodecCtx->height;

    av_image_fill_arrays(pEncFrame->data, pEncFrame->linesize, YuvBuf, pEncodecCtx->pix_fmt, pEncodecCtx->width, pEncodecCtx->height, 32);
    pEncFrame->data[0] = YuvBuf;
    pEncFrame->data[1] = YuvBuf + y_size;
    pEncFrame->data[2] = YuvBuf + y_size * 5 / 4;
    pEncFrame->format = pEncodecCtx->pix_fmt;
    pEncFrame->width = pEncodecCtx->width;
    pEncFrame->height = pEncodecCtx->height;

    return 0;
}

void EncodecStop(void)
{
    avcodec_close(pEncodecCtx);
    av_frame_unref(pEncFrame);
#ifdef HARDWARE_ENCODE_DEF
    av_frame_free(&pEncHwFrame);
#endif
}

void Encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    /* send the frame to the encoder */
    //    if (frame)
    //    {
    //        printf("Send frame %3" PRId64"\n", frame->pts);
    //        fflush(stdout);
    //    }

    if(frame) frame->pts = encode_idx++;

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        fflush(stdout);
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            fflush(stdout);
            return;
        }
        InsertData(RtmpDataIdx, (uint8_t *)&pkt->dts, sizeof(pkt->dts));
        InsertData(RtmpDataIdx, pkt->data, pkt->size);
        SetNextWrBuf(RtmpDataIdx);

        av_packet_unref(pkt);
        //                printf("Write packet %3" PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        //                fflush(stdout);
    }
}

void FrameEncode(void)
{
    uint32_t rd_size = GetRdDataSize(YuvDataIdx);
    if(rd_size == 0) return;

    int32_t enc_counter = 0;
    struct timeval start_time, end_time;

    gettimeofday(&start_time, nullptr);
    EncodecStart();
    while(1)
    {
        if(ReadData(YuvDataIdx, YuvBuf))
        {
            usleep(10000);
            continue;
        }
#ifdef HARDWARE_ENCODE_DEF
        int err;

        if ((err = av_hwframe_transfer_data(pEncHwFrame, pEncFrame, 0)) < 0)
        {
            fprintf(stderr, "Error while transferring frame data to surface\n");
            fflush(stderr);
            enc_counter = pDecodecCtx->gop_size;
        }
        Encode(pEncodecCtx, pEncHwFrame, pEncPkt);
#else
        Encode(pEncodecCtx, pEncFrame, pEncPkt);
#endif
        //        SaveImageDataToFile(pEncFrame, "./test2.jpg");

        enc_counter++;
        if(enc_counter >= pDecodecCtx->gop_size)
        {
            /*after encode one gop frame and flush out the cache data, must reopen encoder*/
            Encode(pEncodecCtx, nullptr, pEncPkt);
            break;
        }
    }
    EncodecStop();
    gettimeofday(&end_time, nullptr);
    printf("encode time = %ld\n", end_time.tv_sec*1000+end_time.tv_usec/1000-(start_time.tv_sec*1000+start_time.tv_usec/1000));
}

void YuvToRgb(void)
{
    if(!GetRdDataSize(YuvDataIdx)) return;

    int src_width = 1920;
    int src_height = 1080;
    int dst_width = 640;
    int dst_height = 480;

    enum AVPixelFormat rgb_fmt = AV_PIX_FMT_RGB32;
    enum AVPixelFormat yuv_fmt = AV_PIX_FMT_YUV420P;
    int image_size = av_image_get_buffer_size(rgb_fmt, dst_width, dst_height, 4);
    if(image_size <= 0) return;

    uint32_t rgb_size = static_cast<uint32_t>(image_size);
    if(CheckWrDataSeg(YuvDataIdx, rgb_size) < 0) return;

    struct SwsContext *pSwsCtx = nullptr;
    AVFrame *pRgbFrame = nullptr;
    AVFrame *pYuvFrame = nullptr;

    pRgbFrame = av_frame_alloc();
    if(pRgbFrame == nullptr) return;
    pYuvFrame = av_frame_alloc();
    if(pYuvFrame == nullptr) return;

    if(av_image_alloc(pRgbFrame->data, pRgbFrame->linesize, dst_width, dst_height, rgb_fmt, 32) < 0) return;
    if(av_image_alloc(pYuvFrame->data, pYuvFrame->linesize, src_width,
                   src_height, yuv_fmt, 16) < 0) return;

    pSwsCtx = sws_getCachedContext(nullptr, src_width, src_height, yuv_fmt,
                                   dst_width, dst_height, rgb_fmt, SWS_BILINEAR,
                                   nullptr, nullptr, nullptr);
    if(pSwsCtx == nullptr)
    {
        fprintf(stderr, "sws_getCachedContext error\n");
        fflush(stderr);
        return;
    }
    while(!ReadData(YuvDataIdx, YuvBuf))
    {
//        av_image_fill_arrays(pRgbFrame->data, pRgbFrame->linesize, RgbBuf, rgb_fmt, src_width, src_height, 4);
//        pRgbFrame->data[0] = RgbBuf;
//        pRgbFrame->format = rgb_fmt;
//        pRgbFrame->width = dst_width;
//        pRgbFrame->height = dst_height;

        int32_t y_size = src_width * src_height;
        memcpy(pYuvFrame->data[0], YuvBuf, y_size * 3 / 2);
//        av_image_fill_arrays(pYuvFrame->data, pYuvFrame->linesize, YuvBuf, yuv_fmt, src_width, src_height, 4);
//        pYuvFrame->data[0] = YuvBuf;
//        pYuvFrame->data[1] = YuvBuf + y_size;
//        pYuvFrame->data[2] = YuvBuf + y_size * 5 / 4;
//        pYuvFrame->format = yuv_fmt;
//        pYuvFrame->width = src_width;
//        pYuvFrame->height = src_height;
        //        SaveImageDataToFile(pYuvFrame, "./test2.jpg");

        sws_scale(pSwsCtx, pYuvFrame->data, pYuvFrame->linesize, 0, src_height, pRgbFrame->data, pRgbFrame->linesize);

        InsertData(DispDataIdx, pRgbFrame->data[0], rgb_size);
        SetNextWrBuf(DispDataIdx);
    }

    //    char file_name[128];
    //    snprintf(file_name, 128, "%s%d%s", test, counter++, ".bmp");
    //    CreateBmp(file_name, pRgbFrame->data[0], pFrame->width, pFrame->height, 32);

    av_freep(&pYuvFrame->data[0]);
    av_freep(&pRgbFrame->data[0]);
    av_frame_free(&pYuvFrame);
    av_frame_free(&pRgbFrame);
    if(pSwsCtx) sws_freeContext(pSwsCtx);
}

int FiltersDraw(AVCodecContext *dec_ctx, AVFrame *frame, AVFrame *filt_frame, int32_t x, int32_t y, int32_t w, int32_t h, int32_t type)
{
    char args[512];
    char draw_desc[128];
    int ret = 0;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVRational time_base = dec_ctx->time_base;
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_NONE };
    char color[64];

    if(type)
    {
        sprintf(color, "%s", "red");
    }
    else
    {
        sprintf(color, "%s", "green");
    }
    sprintf(draw_desc, "drawbox=x=%d:y=%d:w=%d:h=%d:color=%s@1",
            x, y, w, h, color);

    do{
        filter_graph = avfilter_graph_alloc();
        if (!outputs || !inputs || !filter_graph) {
            ret = AVERROR(ENOMEM);
            break;
        }

        /* buffer video source: the decoded frames from the decoder will be inserted here. */
        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                 time_base.num, time_base.den,
                 dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                           args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
            break;
        }

        /* buffer video sink: to terminate the filter chain. */
        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                           NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            break;
        }

        ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
                                  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            break;
        }

        /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

        /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
        outputs->name       = av_strdup("in");
        outputs->filter_ctx = buffersrc_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;

        /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
        inputs->name       = av_strdup("out");
        inputs->filter_ctx = buffersink_ctx;
        inputs->pad_idx    = 0;
        inputs->next       = NULL;

        if ((ret = avfilter_graph_parse_ptr(filter_graph, draw_desc,
                                            &inputs, &outputs, NULL)) < 0)
            break;

        if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
            break;

        /* push the decoded frame into the filtergraph */
        if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
            break;
        }

        /* pull filtered frames from the filtergraph */
        while (1) {
            ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
        }
    }while(0);

    avfilter_free(buffersink_ctx);
    avfilter_free(buffersrc_ctx);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);

    return ret;
}

void SendFrameToYuvProc(AVFrame *frame)
{
    enum AVPixelFormat fmt = (enum AVPixelFormat)frame->format;

    int32_t yuv_size = av_image_get_buffer_size(fmt, frame->width, frame->height, 4);
    int32_t y_size = frame->width * frame->height;
    int32_t u_size = y_size / 4;
    int32_t v_size = u_size;
    if(CheckWrDataSeg(YuvDataIdx, yuv_size) < 0) return;

    //            SaveImageDataToFile(frame, "./test1.jpg");
    InsertData(YuvDataIdx, frame->data[0], y_size);
    InsertData(YuvDataIdx, frame->data[1], u_size);
    InsertData(YuvDataIdx, frame->data[2], v_size);

    SetNextWrBuf(YuvDataIdx);
}

void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVFrame *filt_frame, AVPacket *pkt)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        fflush(stderr);
        return;
    }

    while(ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            fflush(stderr);
            avcodec_flush_buffers(pDecodecCtx);
            return;
        }

        //        switch (frame->pict_type) {
        //        case AV_PICTURE_TYPE_I:
        //            printf("i frame \n");
        //            break;
        //        case AV_PICTURE_TYPE_B:
        //            printf("b frame \n");
        //            break;
        //        case AV_PICTURE_TYPE_P:
        //            printf("p frame \n");
        //            break;
        //        default:
        //            printf("other frame \n");
        //            break;
        //        }
#if 0
        AVFrame *pFrameSent;

        if(Coordinates.size() > 0)
        {
            for(int i = 0;i < Coordinates.size();i++)
            {
                int32_t x = Coordinates[i].left_top_x;
                int32_t y = Coordinates[i].left_top_y;
                int32_t w = abs(x - Coordinates[i].right_bottom_x);
                int32_t h = abs(y - Coordinates[i].right_bottom_y);
                FiltersDraw(dec_ctx, frame, filt_frame, x, y, w, h, Coordinates[i].type);
            }
            pFrameSent = filt_frame;
        }
        else
        {
            pFrameSent = frame;
        }
        SendFrameToYuvProc(pFrameSent);
#else
     SendFrameToYuvProc(frame);
#endif
    }
}

int32_t FrameBsf(AVCodecContext *codec, AVPacket *pkt)
{
    AVBSFContext *pBsfCtx;
    const AVBitStreamFilter *pBsf;
    AVCodecParameters *pCodecParam;

    pBsf = av_bsf_get_by_name("h264_mp4toannexb");

    if(av_bsf_alloc(pBsf, &pBsfCtx) < 0)
    {
        printf("av_bsf_alloc err \n");
        fflush(stdout);
        return -1;
    }

    pCodecParam = avcodec_parameters_alloc();
    avcodec_parameters_from_context(pBsfCtx->par_in, codec);

    if(av_bsf_init(pBsfCtx) < 0)
    {
        printf("av_bsf_init err \n");
        fflush(stdout);
        return -1;
    }

    if(av_bsf_send_packet(pBsfCtx, pkt) < 0)
    {
        printf("av_bsf_send_packet err \n");
        fflush(stdout);
    }

    while (av_bsf_receive_packet(pBsfCtx, pkt) == 0) {
        av_packet_unref(pkt);
    }

    avcodec_parameters_free(&pCodecParam);
    av_bsf_free(&pBsfCtx);

    return 0;
}

int32_t FrameDecodeInit(void)
{
    /* find the MPEG-1 video decoder */
    pDecodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pDecodec) {
        fprintf(stderr, "Codec not found\n");
        fflush(stderr);
        return -1;
    }

    pDecParserCtx = av_parser_init(pDecodec->id);
    if (!pDecParserCtx) {
        fprintf(stderr, "parser not found\n");
        fflush(stderr);
        return -1;
    }

    pDecodecCtx = avcodec_alloc_context3(pDecodec);
    if (!pDecodecCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        fflush(stderr);
        return -1;
    }
    pDecodecCtx->err_recognition |= AV_EF_EXPLODE;

    pDecFrame = av_frame_alloc();
    pFiltFrame = av_frame_alloc();
    if (!pDecFrame || !pFiltFrame) {
        fprintf(stderr, "Could not allocate video frame or filt frame\n");
        fflush(stderr);
        return -1;
    }

    pDecPkt = av_packet_alloc();
    if (!pDecPkt){
        fflush(stderr);
        return -1;
    }

    return 0;
}

void FrameDecodeDeInit(void)
{
    av_parser_close(pDecParserCtx);
    avcodec_free_context(&pDecodecCtx);
    av_frame_free(&pDecFrame);
    av_frame_free(&pFiltFrame);
    av_packet_free(&pDecPkt);
}

int32_t FrameDecodeStart(void)
{
    /* For some codecs, such as msmpeg4 and mpeg4, width and height
   MUST be initialized there because this information is not
   available in the bitstream. */

    /* open it */
    if (avcodec_open2(pDecodecCtx, pDecodec, nullptr) < 0) {
        fprintf(stderr, "Could not open codec\n");
        fflush(stderr);
        return -1;
    }

    return 0;
}

void FrameDecodeStop(void)
{
    avcodec_close(pDecodecCtx);
}

int FrameDecode(void)
{
    uint8_t *pDecBuf = H264ReadBuf;
    uint32_t decode_size;
    int ret;

    do{
        decode_size = GetRdDataSize(H264DataIdx);
        if(decode_size == 0)
        {
            fprintf(stderr, "%s : get decode data size is zero \n", __FUNCTION__);
            fflush(stderr);
            break;
        }

        if(ReadData(H264DataIdx, pDecBuf) < 0)
        {
            fprintf(stderr, "%s : read data fail to decode \n", __FUNCTION__);
            fflush(stderr);
            break;
        }

        FrameDecodeStart();

        /* use the parser to split the data into frames */
        uint8_t *pData = pDecBuf;
        while(decode_size > 0)
        {
            ret = av_parser_parse2(pDecParserCtx, pDecodecCtx, &pDecPkt->data, &pDecPkt->size,
                                   pData, decode_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                break;
            }

            /* flush the decoder */
            if(pDecPkt->size)
            {
                //                FrameBsf(pDecodecCtx, pDecPkt);
                decode(pDecodecCtx, pDecFrame, pFiltFrame, pDecPkt);
            }

            pData += ret;
            decode_size -= ret;
        }
        decode(pDecodecCtx, pDecFrame, pFiltFrame, nullptr);

        if(decode_size == 0)
        {
            ret = 0;
        }
    }while(0);

    FrameDecodeStop();
    av_frame_unref(pDecFrame);
    av_packet_unref(pDecPkt);

    return ret;
}

int32_t ReadRgb(uint8_t *pOut, uint32_t size)
{
    if(!pOut) return -1;

    uint32_t rd_size = GetRdDataSize(DispDataIdx);
    if(!rd_size) return 0;
    else if(rd_size > size)
    {
        printf("read rgb buf not enough\n");
        fflush(stdout);
        return -1;
    }

    if(ReadData(DispDataIdx, pOut) < 0)
    {
        printf("read display data fail\n");
        fflush(stdout);
        return -1;
    }

    return rd_size;
}

int CALLBACK EventHandle(long long llUserData, const char* strEvent, int event_type)
{
    printf("event type = %d , event = %s \n", event_type, strEvent);
    fflush(stdout);

    EventHandleDef pEventHdl = EventHdl[event_type];
    if(pEventHdl)
        pEventHdl(strEvent);
    return 0;
}

int CALLBACK VideoFrameHandle(_handle stream, BYTE* lpBuf, int size, long long llStamp, int type, int channel, long long llUserData)
{
    //        printf("type = 0x%x, lpBuf = %p, size = %u, llStamp = %lld, stream hdl = %p\n", type, static_cast<void *>(lpBuf), size, llStamp, stream);
    //        fflush(stdout);
#if 1
    switch (type)
    {
    case em_FrameType_Header_Frame:
        break;
    case em_FrameType_Video_IFrame:
        GetVideoParam(lpBuf, size);

        if(is_first_iframe_recv)
        {
            SetNextWrBuf(H264DataIdx);

//            pDecodecCtx->gop_size = gop_cnt;
            gop_cnt = 0;
        }
        else
        {
            is_first_iframe_recv = true;
            gop_cnt = 0;
        }

        is_skip_frame = false;
        if(CheckWrDataSeg(H264DataIdx, size) < 0)
        {
            is_skip_frame = true;
            break;
        }

        InsertData(H264DataIdx, lpBuf, static_cast<uint32_t>(size));
        break;
    case em_FrameType_Video_VFrame:
    {
        if(is_skip_frame) break;

        if(is_first_iframe_recv)
        {
            gop_cnt++;
            if(CheckWrSubDataSeg(H264DataIdx, size) < 0) break;
            InsertData(H264DataIdx, lpBuf, static_cast<uint32_t>(size));
        }
        break;
    }
    default:
        break;
    }
#elif 1
    static uint32_t frame_cache_cnt = 0;

    switch (type)
    {
    case em_FrameType_Header_Frame:
        break;
    case em_FrameType_Video_IFrame:
        GetVideoParam(lpBuf, size);

        memcpy(IFrameCache, lpBuf, size);
        IFrameSize = size;
        if(is_first_iframe_recv)
        {
            SetNextWrBuf(H264DataIdx);
            if(CheckWrDataSeg(H264DataIdx, IFrameSize) < 0) break;
            InsertData(H264DataIdx, IFrameCache, IFrameSize);
            frame_cache_cnt = 0;
        }
        else
        {
            if(CheckWrDataSeg(H264DataIdx, IFrameSize) < 0) break;
            InsertData(H264DataIdx, IFrameCache, IFrameSize);
            is_first_iframe_recv = true;
        }
        break;
    case em_FrameType_Video_VFrame:
    {
        if(!is_first_iframe_recv) break;

        if(CheckWrSubDataSeg(H264DataIdx, size) < 0) break;
        InsertData(H264DataIdx, lpBuf, static_cast<uint32_t>(size));

        if(++frame_cache_cnt >= DECODE_CACHE_FRAME_NUM)
        {
            SetNextWrBuf(H264DataIdx);
            if(CheckWrDataSeg(H264DataIdx, IFrameSize) < 0) break;
            InsertData(H264DataIdx, IFrameCache, IFrameSize);
            frame_cache_cnt = 0;
        }
        break;
    }
    default:
        break;
    }
#else
    static int64_t dts_cnt = 0;

    InsertData(RtmpDataIdx, (uint8_t *)&dts_cnt, sizeof(dts_cnt));
    InsertData(RtmpDataIdx, lpBuf, size);
    SetNextWrBuf(RtmpDataIdx);
    dts_cnt++;
#endif
    return 0;
}

void EventDevOnlineHdl(const char *strEvent)
{

}

void EventDevRegisterHdl(const char *strEvent)
{
    is_dev_ready = true;
}

void EventAlarmInfoHdl(const char *strEvent)
{
    char device_id[32];
    if(ParseDeviceId(strEvent, device_id) < 0) return;
    printf("device id : %s \n", device_id);

    int32_t event_type;
    if(ParseEventType(strEvent, &event_type) < 0) return;
    if(event_type != NETEVENT_NOTIFY) return;
    printf("event type : %d \n", event_type);

    int32_t alarm_type;
    if(ParseAlarmType(strEvent, &alarm_type) < 0) return;
    if(alarm_type != eAlarmTypeCustom) return;
    printf("alarm type : %d \n", alarm_type);

    int32_t sub_alarm_type;
    if(ParseSubAlarmType(strEvent, &sub_alarm_type) < 0) return;
    if(sub_alarm_type != eAlarmTypeHelmetDanger) return;
    printf("sub alarm type : %d \n", sub_alarm_type);

    StruDefCoordinate coordinates[COORDINATES_MAX_NUM];
    uint32_t coordinates_nums = 0;
    if(ParseCoordinates(strEvent, coordinates, &coordinates_nums) < 0) return;
    if(coordinates_nums > 0)
    {
        pVideoPlayer->coordinates.clear();
    }
    for(uint32_t i = 0;i < coordinates_nums;i ++)
    {
        pVideoPlayer->coordinates.push_back(coordinates[i]);
        if(pVideoPlayer->coordinates.size() > COORDINATES_MAX_NUM) pVideoPlayer->coordinates.erase(pVideoPlayer->coordinates.begin());
        printf("coordinate[%d] : (%d, %d) (%d, %d) \n", i, coordinates[i].left_top_x, coordinates[i].left_top_y,
               coordinates[i].right_bottom_x, coordinates[i].right_bottom_y);
    }
    fflush(stdout);
}

void EventHandleInit(void)
{
    memset(EventHdl, 0 ,sizeof(EventHdl) / sizeof(EventHandleDef));
    EventHdl[NETEVENT_DEVICE_ONLINE] = EventDevOnlineHdl;
    EventHdl[NETEVENT_DEVICE_REGISTER] = EventDevRegisterHdl;
    EventHdl[NETEVENT_NOTIFY] = EventAlarmInfoHdl;
}

void *rtsp_proc(void *arg)
{
    AVFormatContext *pFmtCtx = nullptr;
    AVCodecContext *pCodecCtx = nullptr;
    AVCodec *pCodec;
    AVPacket av_pkt;
    int video_idx = -1;
    unsigned int usleep_time;
    const char rtsp_path[] = {"rtsp://admin:teamway123456@10.0.60.31:554/stream0"};

    do
    {
        pFmtCtx = avformat_alloc_context();
        if(!pFmtCtx)
        {
            printf("avformat_alloc_context fail\n");
            fflush(stdout);
            break;
        }

        if(avformat_open_input(&pFmtCtx, rtsp_path, nullptr, nullptr) < 0)
        {
            printf("avformat_open_input fail\n");
            fflush(stdout);
            break;
        }

        if(avformat_find_stream_info(pFmtCtx, nullptr) < 0)
        {
            printf("avformat_find_stream_info fail\n");
            fflush(stdout);
            break;
        }

        for(unsigned int i = 0;i < pFmtCtx->nb_streams;i++)
        {
            if(pFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                video_idx = i;
            }
        }
        if(video_idx < 0)
        {
            printf("did not find video data\n");
            fflush(stdout);
            break;
        }
        pDecodecCtx->time_base = pFmtCtx->streams[video_idx]->time_base;
        pDecodecCtx->framerate = pFmtCtx->streams[video_idx]->r_frame_rate;

        pCodec = avcodec_find_decoder(pFmtCtx->streams[video_idx]->codecpar->codec_id);
        if(!pCodec)
        {
            printf("did not find codec\n");
            fflush(stdout);
            break;
        }

        pCodecCtx = avcodec_alloc_context3(pCodec);
        if(!pCodecCtx)
        {
            printf("avcodec_alloc_context3 fail\n");
            fflush(stdout);
            break;
        }

        if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
            fprintf(stderr, "Could not open codec\n");
            fflush(stderr);
            break;
        }

        if(pFmtCtx->streams[video_idx]->r_frame_rate.den <= 0 || pFmtCtx->streams[video_idx]->r_frame_rate.num <= 0)
        {
            printf("rtsp frame rate not correct\n");
            fflush(stdout);
            break;
        }
        usleep_time = pFmtCtx->streams[video_idx]->r_frame_rate.den * 1000000 / pFmtCtx->streams[video_idx]->r_frame_rate.num;
        while(!av_read_frame(pFmtCtx, &av_pkt))
        {
            if(av_pkt.stream_index == video_idx)
            {
#if 0
                GetVideoParam(av_pkt.data, av_pkt.size);
                if(is_key_frame)
                {
                    if(is_first_iframe_recv)
                    {
                        SetNextWrBuf(H264DataIdx);
                        pDecodecCtx->gop_size = counter;
                        counter = 1;
                    }
                    else
                    {
                        is_first_iframe_recv = true;
                        counter = 1;
                    }

                    is_skip_frame = false;
                    if(CheckWrDataSeg(H264DataIdx, av_pkt.size) < 0)
                    {
                        is_skip_frame = true;
                        break;
                    }

                    InsertData(H264DataIdx, av_pkt.data, static_cast<uint32_t>(av_pkt.size));
                }
                else
                {
                    counter++;
                    if(is_skip_frame) break;

                    if(is_first_iframe_recv)
                    {
                        if(CheckWrSubDataSeg(H264DataIdx, av_pkt.size) < 0) break;
                        InsertData(H264DataIdx, av_pkt.data, static_cast<uint32_t>(av_pkt.size));
                    }
                }
#else
                InsertData(RtmpDataIdx, (uint8_t *)&av_pkt.dts, sizeof(av_pkt.dts));
                InsertData(RtmpDataIdx, av_pkt.data, av_pkt.size);
                SetNextWrBuf(RtmpDataIdx);
#endif
                //                usleep(usleep_time);
            }
        }
    }while(0);

    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFmtCtx);
    if(pFmtCtx) avformat_free_context(pFmtCtx);

    return nullptr;
}

void *decode_proc(void *arg)
{
    FrameDecodeInit();

    while(true)
    {
        if(is_dev_ready)
        {
            int Ret = SDS_NetDevice_OpenStream("30014", 1, eMainStream, StreamHdl);
            if(Ret != ERR_SUCCESS)
            {
                printf("SDS_NetDevice_OpenStream errcode = 0x%x \n", Ret);
                return nullptr;
            }

            Ret = SDS_NetDevice_GetAlarm("30014", true);
            if(Ret != ERR_SUCCESS)
            {
                printf("SDS_NetDevice_GetAlarm errcode = 0x%x \n", Ret);
                return nullptr;
            }
            is_dev_ready = false;
        }

        uint32_t rd_size = GetRdDataSize(H264DataIdx);
        if(rd_size > 0)
        {
            FrameDecode();
        }
        usleep(30000);
    }


}

void *yuv_proc(void *arg)
{
    EncodecInit();

    while(true)
    {
#ifdef LOCAL_PLAY_DEF
        YuvToRgb();
#else
        FrameEncode();
#endif
        usleep(10000);
    }
}

void *coordinates_proc(void *arg)
{
    int client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_sockfd < 0)
    {
        perror("client socket err");
        return nullptr;
    }

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("10.0.48.66");
    server_addr.sin_port = htons(8091);

//    client_addr.sin_family = AF_INET;
//    client_addr.sin_addr.s_addr = inet_addr("10.0.48.67");
//    client_addr.sin_port = htons(8091);
    socklen_t addr_len = sizeof(client_addr);

    uint8_t recv_buf[512 + 1];

    if(bind(client_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind err");
        fflush(stderr);
        close(client_sockfd);
        return nullptr;
    }

    while(true)
    {
        ssize_t recv_len = recvfrom(client_sockfd, recv_buf, 512, 0, (struct sockaddr *)&client_addr, &addr_len);
        if(recv_len > 0)
        {
            recv_buf[recv_len] = '\0';
            printf("%s\n", recv_buf);
            fflush(stdout);

            StruDefCoordinate coordinates[COORDINATES_MAX_NUM];
            uint32_t nums;

            if(ParseCoordinateFromUdp((const char *)recv_buf, coordinates, &nums) < 0)
            {
                continue;
            }
            pVideoPlayer->coordinates.clear();
            for(uint32_t i = 0;i < nums;i ++)
            {
                pVideoPlayer->coordinates.push_back(coordinates[i]);
            }
        }
        else if(recv_len == 0)
        {
            printf("recvfrom len = 0\n");
            fflush(stdout);
        }
        else
        {
            perror("recvfrom err");
        }
    }
    close(client_sockfd);
    return nullptr;
}

void *rtmp_send_proc(void *arg)
{
    //    RtmpServerInit();
    SetCallBackFunc(video_param, (void *)GetVideoParamCb);
    SetCallBackFunc(video_data, (void *)GetVideoDataCb);
    RtmpServerStart();

    //    while(true)
    //    {
    //        FrameDecode2();
    //        usleep(30000);
    //    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //    enum AVHWDeviceType type;
    //    while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
    //    {
    //        fprintf(stdout, " %s \n", av_hwdevice_get_type_name(type));
    //        fflush(stdout);
    //    }

    EventHandleInit();

#ifdef LOCAL_PLAY_DEF
        pVideoPlayer = new QtPlayer;
        pVideoPlayer->GetRgb = ReadRgb;
#endif

    if(QueueBufInit(256) < 0)
    {
        fprintf(stderr, "QueueBufInit err \n");
        fflush(stderr);
        return -1;
    }
    H264DataIdx = QueueBufRegister(H264DataBuf, H264_DATA_BUF_SIZE, 256, 256);
    if(H264DataIdx < 0)
    {
        fprintf(stderr, "H264DataBuf QueueBufRegister err \n");
        fflush(stderr);
        return -1;
    }
    YuvDataIdx = QueueBufRegister(YuvDataBuf, YUV_DATA_BUF_SIZE, 256, 3);
    if(YuvDataIdx < 0)
    {
        fprintf(stderr, "YuvDataBuf QueueBufRegister err \n");
        fflush(stderr);
        return -1;
    }
    RtmpDataIdx = QueueBufRegister(RtmpDataBuf, RTMP_DATA_BUF_SIZE, 256, 1);
    if(RtmpDataIdx < 0)
    {
        fprintf(stderr, "RtmpDataBuf QueueBufRegister err \n");
        fflush(stderr);
        return -1;
    }
    DispDataIdx = QueueBufRegister(DispDataBuf, DISP_DATA_BUF_SIZE, 256, 3);
    if(DispDataIdx < 0)
    {
        fprintf(stderr, "DispDataBuf QueueBufRegister err \n");
        fflush(stderr);
        return -1;
    }

    st_work_param work_param;
    const char server_ip[] = {"10.0.48.66"};
    strcpy(work_param.server, server_ip);
    work_param.port = 6608;
    work_param.callbackEvent = EventHandle;
    work_param.callbackFrame = VideoFrameHandle;
    work_param.llUserData = 0;
    int Ret = SDS_NetDevice_Init(work_param);
    if(Ret != ERR_SUCCESS)
    {
        printf("SDS_NetDevice_Init errcode = 0x%x \n", Ret);
        return Ret;
    }

    pthread_mutex_init(&video_param_mtx, nullptr);
    //    pthread_t rtsp_pth;
    pthread_t decode_pth;
    pthread_t yuv_pth;
    pthread_t coordinates_pth;

    //    pthread_create(&rtsp_pth, nullptr, rtsp_proc, nullptr);
    pthread_create(&decode_pth, nullptr, decode_proc, nullptr);
    pthread_create(&yuv_pth, nullptr, yuv_proc, nullptr);
    pthread_create(&coordinates_pth, nullptr, coordinates_proc, nullptr);
#ifndef LOCAL_PLAY_DEF
    pthread_t rtmp_send_pth;

    pthread_create(&rtmp_send_pth, nullptr, rtmp_send_proc, nullptr);
    //    PlayMp4File("../rtmp_test.mp4");
#endif

    return a.exec();
}
