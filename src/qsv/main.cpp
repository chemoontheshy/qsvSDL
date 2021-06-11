

#include <stdio.h>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>

}
using namespace std;

//
typedef struct DecodeContext {
    AVBufferRef* hw_device_ref;
}DecodeContext;




int main(int argc, char* argv[])
{
    //总上下文
    AVFormatContext* input_ctx = nullptr;

    //解码结构体
    AVCodecContext* decoder_ctx = nullptr;
    //解码器
    AVCodec* decoder = nullptr;
    //硬解码缓存区
    DecodeContext decode = {nullptr};

    AVBufferRef* te = nullptr;

    AVPacket* packet;

    int ret, i;
    //视频流
    int video_stream_index = -1;
    //获取输入的文件名
    const char* input_name = "../../3rd/video/test.mp4";
    //打开输入文件
    ret = avformat_open_input(&input_ctx, input_name, nullptr, nullptr);
    if (ret < 0) {
        cout << "not open the input file :" << input_name << endl;
        return 0;
    }

    //获取流信息
    ret = avformat_find_stream_info(input_ctx, nullptr);
    if (ret < 0) {
        cout << "find stream info error." << endl;
        return 0;
    }

    //视频流开始
    video_stream_index = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder,0);
    if (decoder == nullptr) {
        cout << "find video stream index error." << endl;
        return 0;
    }
   
    //
    ////打开硬件设备
    //ret = av_hwdevice_ctx_create(&decode.hw_device_ref, AV_HWDEVICE_TYPE_QSV,nullptr, nullptr, 0);
    //if (ret < 0) {
    //    cout << "cannot open the hardware dvice" << endl;
    //    return 0;
    //}

    //初始化解码器
    decoder = avcodec_find_decoder_by_name("h264_qsv");
    if (!decoder) {
        cout << "The QSV decoder is not present in libavcodec" << endl;
        return 0;
    }

    decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx) {
        cout << AVERROR(ENOMEM) << endl;
    }
    decoder_ctx->codec_id = AV_CODEC_ID_H264;

    decoder_ctx->opaque=&decode;
    //decoder_ctx->get_format = AV_HWDEVICE_TYPE_QSV;

    //
    ret = avcodec_open2(decoder_ctx, nullptr, nullptr);
    if (ret < 0) {
        cout << "Error open the decoder"<<endl;
    }

    AVFrame* frame = av_frame_alloc();
    AVFrame* sw_frame = av_frame_alloc();
    packet = av_packet_alloc();
    //开始解码
    int frameFinish = 0;
    int num = 0;
    while (av_read_frame(input_ctx, packet) >= 0) {
        int index = packet->stream_index;
        if (index == video_stream_index) {
            frameFinish = avcodec_send_packet(decoder_ctx, packet);
            if (frameFinish < 0) {
                continue;
            }
            frameFinish = avcodec_receive_frame(decoder_ctx, frame);
            if (frameFinish < 0) {
                continue;
            }
            if (frameFinish >= 0) {
                num++;
                cout << "iFrame->format" << frame->format << endl;
                printf("finish decode %d frame\n", num);
                if (frame->format == AV_HWDEVICE_TYPE_QSV) {
                    cout << "test" << endl;
                }
            }
        }
        av_packet_unref(packet);
        av_freep(packet);
    }

    /* flush the decoder */

    //lu
    avformat_close_input(&input_ctx);

    av_frame_free(&frame);
    av_frame_free(&sw_frame);

    avcodec_free_context(&decoder_ctx);

    av_buffer_unref(&decode.hw_device_ref);

    cout << "end" << endl;
    //
    return 0;
}