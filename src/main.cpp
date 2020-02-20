#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <vector>
#include <math.h>

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

#ifdef __cplusplus
}
#endif

using namespace  SDS_Device::sdk;
using namespace sds::error;
using namespace SDS_Device::enums;

#define EVENT_TYPE_NUM  (NETEVENT_DOWNLOADING_FILE + 1)
#define DEVICE_ID_SIZE  32

#define H264_DATA_BUF_SIZE  (1024*1024)
#define DISP_DATA_BUF_SIZE  (50*1024*1024)
#define RGB_DATA_QUEUE_NUM  256
#define COORDINATES_MAX_NUM 64

typedef void (*EventHandleDef)(const char*);
typedef struct
{
    uint8_t *pRgb;
    uint32_t size;
}StruDefRgbData;

EventHandleDef EventHdl[EVENT_TYPE_NUM];
char DeviceId[DEVICE_ID_SIZE];
bool is_dev_ready = false;
_handle StreamHdl;

uint8_t H264DataBuf[H264_DATA_BUF_SIZE];
int32_t H264DataIdx = 0;
bool is_first_iframe_recv = false;
bool is_skip_frame = false;

QtPlayer *pVideoPlayer;
uint32_t video_w = 0;
uint32_t video_h = 0;

uint8_t DispDataBuf[DISP_DATA_BUF_SIZE];
StruDefRgbData RgbDataQueue[RGB_DATA_QUEUE_NUM];
volatile uint32_t QueueRdIdx = 0;
volatile uint32_t QueueWrIdx = 0;

char test[32] = {"test_"};
char jpg[32] = {".jpg"};
uint32_t counter = 0;

struct timespec start;
struct timespec end;

std::vector<StruDefCoordinate> Coordinates;

bool CheckRgbWrBuf(uint32_t rgb_size)
{
    uint32_t wr_idx = QueueWrIdx;

    do{
        uint32_t rd_idx = QueueRdIdx;
        uint8_t *pWr = RgbDataQueue[QueueWrIdx].pRgb;
        uint8_t *pRd = RgbDataQueue[rd_idx].pRgb;

        uint32_t diff = static_cast<uint32_t>((pWr >= pRd)?(pWr - pRd):(pWr + DISP_DATA_BUF_SIZE - pRd));
        if(diff + rgb_size > DISP_DATA_BUF_SIZE)
        {
            printf("write the current frame to preview display buf \n");
            fflush(stdout);

            wr_idx = (wr_idx > 0)?(wr_idx - 1):(RGB_DATA_QUEUE_NUM - 1);
            if(wr_idx == rd_idx)
            {
                return false;
            }
            continue;
        }
        break;
    }while(1);

    QueueWrIdx = wr_idx;
    return true;
}

uint8_t *GetRgbWrBuf(uint32_t rgb_size)
{
    uint32_t wr_idx = QueueWrIdx;
    uint32_t rd_idx = QueueRdIdx;
    uint8_t *pWr = RgbDataQueue[QueueWrIdx].pRgb;
    uint8_t *pRd = RgbDataQueue[rd_idx].pRgb;

    do{
        uint32_t diff = static_cast<uint32_t>((pWr >= pRd)?(pWr - pRd):(pWr + DISP_DATA_BUF_SIZE - pRd));
        if(diff + rgb_size > DISP_DATA_BUF_SIZE)
        {
            printf("write the current frame to preview display buf \n");
            fflush(stdout);

            wr_idx = (wr_idx > 0)?(wr_idx - 1):0;
            if(wr_idx == rd_idx)
            {
                return nullptr;
            }
            continue;
        }
        break;
    }while(1);

    QueueWrIdx = wr_idx;
    return RgbDataQueue[QueueWrIdx].pRgb;
}

void PushRgb(uint8_t rgb, uint32_t idx)
{
    uint8_t *pWr = RgbDataQueue[QueueWrIdx].pRgb;
    if(pWr + idx >= DispDataBuf + DISP_DATA_BUF_SIZE)
    {
        pWr = pWr + idx - DISP_DATA_BUF_SIZE;
    }
    else
    {
        pWr += idx;
    }
    *pWr = rgb;
}

void InsertRgb(uint8_t *pRgb, uint32_t size, uint32_t idx)
{
    for(uint32_t i = 0;i < size;i ++)
    {
        PushRgb(pRgb[i], idx + i);
    }
}

void SetNextRgbWrBuf(uint32_t rgb_size)
{
    uint8_t *pWr = RgbDataQueue[QueueWrIdx].pRgb + rgb_size;
    uint8_t *pOut = RgbDataQueue[QueueWrIdx].pRgb;

    RgbDataQueue[QueueWrIdx].size = rgb_size;
    QueueWrIdx = (QueueWrIdx + 1 < RGB_DATA_QUEUE_NUM)?(QueueWrIdx + 1):0;
    if(pWr < DispDataBuf + DISP_DATA_BUF_SIZE)
    {
        RgbDataQueue[QueueWrIdx].pRgb = pWr;
    }
    else
    {
        RgbDataQueue[QueueWrIdx].pRgb = pWr - DISP_DATA_BUF_SIZE;
    }

    //    printf("wr rgb buf = (%p - %p) \n", pOut, pOut + rgb_size);
    //    fflush(stdout);
}

void SaveImageDataoFile(AVFrame *pFrame, const char *pFileOut)
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

void YuvToRgb(AVCodecContext *pCodecCtx, AVFrame *pFrame)
{
#if 1
    int image_size = av_image_get_buffer_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height, 4);
    if(image_size <= 0) return;

    uint32_t rgb_size = static_cast<uint32_t>(image_size);
    if(!CheckRgbWrBuf(rgb_size)) return;

    struct SwsContext *pSwsCtx = nullptr;
    AVFrame *pRgbFrame = nullptr;
    uint8_t *pRgbBuf = nullptr;

    do
    {
        pSwsCtx = sws_getCachedContext(nullptr, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                       pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB32, SWS_BICUBIC,
                                       nullptr, nullptr, nullptr);
        if(pSwsCtx == nullptr)
        {
            fprintf(stderr, "sws_getCachedContext error\n");
            fflush(stderr);
            return;
        }

        pRgbFrame = av_frame_alloc();
        if(pRgbFrame == nullptr) break;
        pRgbBuf = (uint8_t *)av_malloc(rgb_size);
        if(pRgbBuf == nullptr) break;

        av_image_fill_arrays(pRgbFrame->data, pRgbFrame->linesize, pRgbBuf, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height, 4);
        pRgbFrame->format = AV_PIX_FMT_RGB32;
        sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pRgbFrame->data, pRgbFrame->linesize);

        InsertRgb(pRgbBuf, rgb_size, 0);

        SetNextRgbWrBuf(rgb_size);
        video_w = pFrame->width;
        video_h = pFrame->height;
    }while(0);

    //    char file_name[128];
    //    snprintf(file_name, 128, "%s%d%s", test, counter++, ".bmp");
    //    CreateBmp(file_name, pRgbFrame->data[0], pFrame->width, pFrame->height, 32);

    if(pRgbBuf) av_free(pRgbBuf);
    if(pRgbFrame) av_frame_free(&pRgbFrame);
    if(pSwsCtx) sws_freeContext(pSwsCtx);
#else
    /*R = Y + 1.402 * (V - 128)
      G = Y - 0.34414 * (U - 128) - 0.71414 * (V - 128)
      B = Y + 1.772 * (U - 128)*/

    int frame_size = pFrame->width * pFrame->height * 4;
    if(frame_size <= 0) return;

    if(!CheckRgbWrBuf(static_cast<uint32_t>(frame_size))) return;

    uint8_t *pYData = pFrame->data[0];
    uint8_t *pUData = pFrame->data[1];
    uint8_t *pVData = pFrame->data[2];
    int yIdx, vIdx, uIdx, rgbIdx;
    int rgb[4];

    for (int i = 0;i < pCodecCtx->height;i++)
    {
        for (int j = 0;j < pCodecCtx->width;j++)
        {
            yIdx = i * pCodecCtx->width + j;
            vIdx = (i/2) * (pCodecCtx->width/2) + (j/2);
            uIdx = vIdx;

            rgb[0] = 128;
            rgb[1] = static_cast<int>(pYData[yIdx] + 1.370705 * (pVData[uIdx] - 128));
            rgb[2] = static_cast<int>(pYData[yIdx] - 0.698001 * (pUData[uIdx] - 128) - 0.703125 * (pVData[vIdx] - 128));
            rgb[3] = static_cast<int>(pYData[yIdx] + 1.732446 * (pUData[vIdx] - 128));

            for (int k = 0;k < 4;k++)
            {
                rgbIdx = (i * pCodecCtx->width + j) * 4 + k;
                if(rgb[k] >= 0 && rgb[k] <= 255)
                    PushRgb(static_cast<uint8_t>(rgb[k]), static_cast<uint32_t>(rgbIdx));
                else
                    PushRgb((rgb[k] < 0)?0:255, static_cast<uint32_t>(rgbIdx));
            }
        }
    }
    SetNextRgbWrBuf(frame_size);
    video_w = pFrame->width;
    video_h = pFrame->height;

#endif
}

int FiltersDraw(AVCodecContext *dec_ctx, AVFrame *frame, AVFrame *filt_frame, int32_t x, int32_t y, int32_t w, int32_t h)
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

    sprintf(draw_desc, "drawbox=x=%d:y=%d:w=%d:h=%d:color=yellow@1",
            x, y, w, h);

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

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);

    return ret;
}

void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVFrame *filt_frame, AVPacket *pkt)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return;
    }

    while(ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
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
        printf("frame pts = 0x%lx \n", frame->best_effort_timestamp);
        fflush(stdout);

        if(Coordinates.size() > 0)
        {
            int32_t x = Coordinates[0].left_top_x;
            int32_t y = Coordinates[0].left_top_y;
            int32_t w = abs(x - Coordinates[0].right_bottom_x);
            int32_t h = abs(y - Coordinates[0].right_bottom_y);
            FiltersDraw(dec_ctx, frame, filt_frame, x, y, w, h);
            Coordinates.erase(Coordinates.begin());
            //            SaveImageDataoFile(filt_frame, "./test.jpg");
            YuvToRgb(dec_ctx, filt_frame);
        }
        else
        {
            YuvToRgb(dec_ctx, frame);
        }

    }
}

int FrameDecode(void)
{
    const AVCodec *codec = nullptr;
    AVCodecParserContext *parser = nullptr;
    AVCodecContext *c= nullptr;
    AVFrame *frame = nullptr;
    AVFrame *filt_frame = nullptr;
    int ret = -1;
    AVPacket *pkt = nullptr;
    uint8_t *pDecBuf = nullptr;
    uint32_t decode_size;

    do{
        decode_size = GetRdDataSize(H264DataIdx);
        if(decode_size == 0)
        {
            fprintf(stderr, "%s : get decode data size is zero \n", __FUNCTION__);
            fflush(stderr);
            break;
        }

        pDecBuf = (uint8_t *)av_malloc(decode_size);
        if(pDecBuf == nullptr)
        {
            fprintf(stderr, "%s : molloc fail for decode data \n", __FUNCTION__);
            fflush(stderr);
            break;
        }

        if(ReadData(H264DataIdx, pDecBuf) < 0)
        {
            fprintf(stderr, "%s : read data fail to decode \n", __FUNCTION__);
            fflush(stderr);
            break;
        }

        pkt = av_packet_alloc();
        if (!pkt)
            break;

        /* find the MPEG-1 video decoder */
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            fprintf(stderr, "Codec not found\n");
            break;
        }

        parser = av_parser_init(codec->id);
        if (!parser) {
            fprintf(stderr, "parser not found\n");
            break;
        }

        c = avcodec_alloc_context3(codec);
        if (!c) {
            fprintf(stderr, "Could not allocate video codec context\n");
            break;
        }

        /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

        /* open it */
        if (avcodec_open2(c, codec, nullptr) < 0) {
            fprintf(stderr, "Could not open codec\n");
            break;
        }

        frame = av_frame_alloc();
        filt_frame = av_frame_alloc();
        if (!frame || !filt_frame) {
            fprintf(stderr, "Could not allocate video frame or filt frame\n");
            break;
        }

        /* use the parser to split the data into frames */
        uint8_t *pData = pDecBuf;
        while(decode_size > 0)
        {
            ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
                                   pData, decode_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                fprintf(stderr, "Error while parsing\n");
                break;
            }

            /* flush the decoder */
            if(pkt->size)
                decode(c, frame, filt_frame, pkt);

            pData += ret;
            decode_size -= ret;
        }
        decode(c, frame, filt_frame, nullptr);

        if(decode_size == 0)
        {
            ret = 0;
        }
    }while(0);

    if(pDecBuf) av_free(pDecBuf);
    if(parser) av_parser_close(parser);
    if(c) avcodec_free_context(&c);
    if(frame) av_frame_free(&frame);
    if(filt_frame) av_frame_free(&filt_frame);
    if(pkt) av_packet_free(&pkt);

    return ret;
}

bool ReadRgb(uint8_t *pOut, uint32_t *pW, uint32_t *pH)
{
    if(pOut)
    {
        uint32_t rd_idx = QueueRdIdx;
        uint32_t wr_idx = QueueWrIdx;

        if(rd_idx == QueueWrIdx)
        {
            return false;
        }

        uint32_t diff = (wr_idx >= rd_idx)?(wr_idx - rd_idx):(wr_idx + RGB_DATA_QUEUE_NUM - rd_idx);
        if(diff >= 5)
        {
            printf("skip forward 5 frame \n");
            fflush(stdout);
            rd_idx = (rd_idx + 5 < RGB_DATA_QUEUE_NUM)?(rd_idx + 5):(rd_idx + 5 - RGB_DATA_QUEUE_NUM);
        }

        if(RgbDataQueue[rd_idx].size == 0)
        {
            return false;
        }

        uint8_t *pRgb = RgbDataQueue[rd_idx].pRgb;
        uint32_t rgb_size = RgbDataQueue[rd_idx].size;
        if(pRgb + rgb_size <= DispDataBuf + DISP_DATA_BUF_SIZE)
        {
            memcpy(pOut, pRgb, rgb_size);
        }
        else
        {
            size_t cpy_size = static_cast<size_t>(DispDataBuf + DISP_DATA_BUF_SIZE - pRgb);
            memcpy(pOut, pRgb, cpy_size);
            memcpy(pOut + cpy_size, DispDataBuf, rgb_size - cpy_size);
        }

        //        printf("rd rgb buf = (%p - %p) \n",static_cast<void *>(pRgb), static_cast<void *>(pRgb + rgb_size));
        //        fflush(stdout);

        RgbDataQueue[rd_idx].pRgb = nullptr;
        RgbDataQueue[rd_idx].size = 0;

        if(++rd_idx >= RGB_DATA_QUEUE_NUM)
        {
            rd_idx = 0;
        }
        QueueRdIdx = rd_idx;

        return true;
    }
    else if(pW && pH)
    {
        *pW = video_w;
        *pH = video_h;

        return true;
    }
    else
    {
        return false;
    }
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
    //    printf("type = 0x%x, lpBuf = %p, size = %u, llStamp = %lld, stream hdl = %p\n", type, static_cast<void *>(lpBuf), size, llStamp, stream);
    //    fflush(stdout);
#if 1
    switch (type)
    {
    case em_FrameType_Header_Frame:
        break;
    case em_FrameType_Video_IFrame:
        if(is_first_iframe_recv)
        {
            SetNextWrBuf(H264DataIdx);
        }
        else
        {
            is_first_iframe_recv = true;
        }

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
            if(CheckWrSubDataSeg(H264DataIdx, size) < 0) break;
            InsertData(H264DataIdx, lpBuf, static_cast<uint32_t>(size));
        }
        break;
    }
    default:
        break;
    }
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
    for(uint32_t i = 0;i < coordinates_nums;i ++)
    {
        Coordinates.push_back(coordinates[i]);
        if(Coordinates.size() > COORDINATES_MAX_NUM) Coordinates.erase(Coordinates.begin());
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

void *main_proc(void *arg)
{
    while(true)
    {
        if(is_dev_ready)
        {
            int Ret = SDS_NetDevice_OpenStream("30013", 1, eMainStream, StreamHdl);
            if(Ret != ERR_SUCCESS)
            {
                printf("SDS_NetDevice_OpenStream errcode = 0x%x \n", Ret);
                return nullptr;
            }

            Ret = SDS_NetDevice_GetAlarm("30013", true);
            if(Ret != ERR_SUCCESS)
            {
                printf("SDS_NetDevice_GetAlarm errcode = 0x%x \n", Ret);
                return nullptr;
            }
            is_dev_ready = false;
        }

        if(GetRdDataSize(H264DataIdx) > 0)
        {
            FrameDecode();
        }

        usleep(30000);
    }
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

    pVideoPlayer = new QtPlayer;
    pVideoPlayer->GetRgb = ReadRgb;

    memset((void *)RgbDataQueue, 0, sizeof(StruDefRgbData) * RGB_DATA_QUEUE_NUM);
    RgbDataQueue[0].pRgb = DispDataBuf;
    //    printf("DispDataBuf = (0x%x - 0x%x) \n", DispDataBuf, DispDataBuf + DISP_DATA_BUF_SIZE);
    //    fflush(stdout);

    if(QueueBufInit(256) < 0)
    {
        fprintf(stderr, "QueueBufInit err \n");
        fflush(stderr);
        return -1;
    }
    H264DataIdx = QueueBufRegister(H264DataBuf, H264_DATA_BUF_SIZE, 256, 256);
    if(H264DataIdx < 0)
    {
        fprintf(stderr, "QueueBufRegister err \n");
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

    pthread_t main_pth;
    pthread_create(&main_pth, nullptr, main_proc, nullptr);

    return a.exec();
}
