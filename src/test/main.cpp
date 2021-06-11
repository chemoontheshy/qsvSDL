/**
 * @file
 * HW-Accelerated decoding example.
 *
 * @example hw_decode.c
 * This example shows how to do HW-accelerated decoding with output
 * frames from the HW video surfaces.
 */

 //��ǰC++����C����
extern "C"
{
    //avcodec:�����(����Ҫ�Ŀ�)
#include <libavcodec/avcodec.h>
//avformat:��װ��ʽ����
#include <libavformat/avformat.h>
//swscale:��Ƶ�������ݸ�ʽת��
#include <libswscale/swscale.h>
//avdevice:�����豸���������
#include <libavdevice/avdevice.h>
//avutil:���߿⣨�󲿷ֿⶼ��Ҫ������֧�֣�
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <SDL.h>
}
#include <stdio.h>
#include <iostream>
#include <time.h>
using namespace std;

//Ӳ����
static AVBufferRef* hw_device_ctx = nullptr;
static enum AVPixelFormat hw_pix_fmt;
static FILE* output_file = nullptr;
int a = 0;


//SDL
SDL_Rect rect;
Uint32 pixformat;
SDL_Window* win = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

// Ĭ�ϴ����С
int w_width = 640;
int w_height = 480;

static int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
        nullptr, nullptr, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext* ctx,
    const enum AVPixelFormat* pix_fmts)
{
    const enum AVPixelFormat* p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}


int main(int argc, char* argv[])
{
    clock_t start, finish;
    start = clock();
    //FFmpeg����ͨ��
    AVFormatContext *input_ctx = nullptr;
    int video_stream, ret;
    AVStream *video = nullptr;
    AVCodecContext *decoder_ctx = nullptr;
    AVCodec *decoder = nullptr;
   
    enum AVHWDeviceType type;
    int i;

    // SDL
    //SDL��ʼ��
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
        return 0;
    }

    //Ӳ����
    //Available device types: cuda dxva2 qsv d3d11va opencl vulkan
    const char* m_type = "qsv";
    type = av_hwdevice_find_type_by_name(m_type);
    //֧�ֵĸ�ʽ
    if (type == AV_HWDEVICE_TYPE_NONE) {
        fprintf(stderr, "Device type %s is not supported.\n", m_type);
        fprintf(stderr, "Available device types:");
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
        fprintf(stderr, "\n");
        return -1;
    }
    //��ȡ������ļ���
    const char* input = "../../3rd/video/test.mp4";

    //��װ��ʽ������
    input_ctx = avformat_alloc_context();

    //����Ƶ��
    ret = avformat_open_input(&input_ctx, input, nullptr, nullptr);
    if (ret < 0) {
        cout << "open input error." << endl;
        return 0;
    }

    //�ҳ���Ƶ��Ϣ
    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }
    cout << "test" << endl;
    // ѭ������Ƶ��
    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    video_stream = ret;

    for (i = 0;; i++) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                decoder->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
    //�򿪽�����
    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    //��������ȡָ����ʽ
    decoder_ctx->get_format = get_hw_format;

    if (hw_decoder_init(decoder_ctx, type) < 0)
        return -1;

    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", video_stream);
        return -1;
    }
    w_width = decoder_ctx->width;
    w_height = decoder_ctx->height;

    //��������
    win = SDL_CreateWindow("Player",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w_width,
        w_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!win) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
        return 0;
    }

    //������Ⱦ��
    renderer = SDL_CreateRenderer(win, -1, 0);
    if (!renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
        return 0;
    }

    //SDL��ʽyuv
    pixformat = SDL_PIXELFORMAT_IYUV;
    //��������
    texture = SDL_CreateTexture(renderer,
        pixformat,
        SDL_TEXTUREACCESS_STREAMING,
        w_width, w_height);

    //���ٻ���洢һ֡����
    //�������ַ���������,avpicture_fill�Ѿ��𽥱�����
    //avpicture_fill((AVPicture *)avFrame3, buffer, dstFormat, videoWidth, videoHeight);
    int frameFinish;
    AVPacket* packet = av_packet_alloc();
    AVFrame *iFrame = av_frame_alloc();
    AVFrame *oFrame = av_frame_alloc();
    uint8_t *out_bufffer = nullptr;
    int byte = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, w_width, w_height, 1);
    out_bufffer = (uint8_t*)av_malloc(byte * sizeof(uint8_t));
    cout <<"decoder_ctx->pix_fmt"<< decoder_ctx->pix_fmt << endl;
    //���ٻ���洢һ֡
    av_image_fill_arrays(oFrame->data, oFrame->linesize, out_bufffer, AV_PIX_FMT_YUV420P, w_width, w_height, 1);
    //ͼ��ת��
    //����ͼƬ���ݶ���
    //SwsContext *img_convert_ctx = nullptr;
    //img_convert_ctx = sws_getContext(w_width, w_height, AV_PIX_FMT_D3D11VA_VLD, w_width, w_height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    int num = 0;
    while (av_read_frame(input_ctx, packet) >= 0) {
        int index = packet->stream_index;
        if (index == video_stream) {
            frameFinish = avcodec_send_packet(decoder_ctx, packet);
            if (frameFinish < 0) {
                continue;
            }
            frameFinish = avcodec_receive_frame(decoder_ctx, iFrame);
            if (frameFinish < 0) {
                continue;
            }
            if (frameFinish >= 0) {
                num++;
                cout << "iFrame->format" << iFrame->format << endl;
                printf("finish decode %d frame\n", num);
                if (iFrame->format == hw_pix_fmt) {
                    /*data from GPU*/
                    //sws_scale(img_convert_ctx, (const uint8_t *const *)iFrame->data, iFrame->linesize, 0, w_height, oFrame->data, oFrame->linesize);
                    //SDL_UpdateYUVTexture(texture, nullptr,
                    //    oFrame->data[0], oFrame->linesize[0],
                    //    oFrame->data[1], oFrame->linesize[1],
                    //    oFrame->data[2], oFrame->linesize[2]);
                    ////Set size of window
                    //rect.x = 0;
                    //rect.y = 0;
                    //rect.w = w_width;
                    //rect.h = w_height;
                    ////չʾ
                    //SDL_RenderClear(renderer);
                    //SDL_RenderCopy(renderer, texture, nullptr, &rect);
                    //SDL_RenderPresent(renderer);
                    //SDL_Delay(40);
                }
            }
        }
        av_packet_unref(packet);
        av_freep(packet);
    }

    //SDL free

    if (win) {
        SDL_DestroyWindow(win);
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }

    if (texture) {
        SDL_DestroyTexture(texture);
    }
    finish = clock();
    //clock()�������ش�ʱCPUʱ�Ӽ�ʱ��Ԫ��
    cout << endl << "the time cost is:" << float(finish - start) / CLOCKS_PER_SEC << endl;
    return 0;
}