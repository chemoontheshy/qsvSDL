

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
   
    
    ret = av_hwdevice_ctx_create(&decode.hw_device_ref, AV_HWDEVICE_TYPE_QSV, "auto", nullptr, 0);
    if (ret < 0) {
        cout << "cannot open the hardware dvice" << endl;
        return 0;
    }
    cout << "end" << endl;
    //
    return 0;
}