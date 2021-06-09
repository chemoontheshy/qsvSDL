//当前C++兼容C语言
extern "C"
{
	//avcodec:编解码(最重要的库)
#include <libavcodec/avcodec.h>
//avformat:封装格式处理
#include <libavformat/avformat.h>
//swscale:视频像素数据格式转换
#include <libswscale/swscale.h>
//avdevice:各种设备的输入输出
#include <libavdevice/avdevice.h>
//avutil:工具库（大部分库都需要这个库的支持）
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
//SDL2
#include <SDL.h>
}

#include<iostream>
#include<mutex>
#include<time.h>

using namespace std;

//硬解码相关
static AVBufferRef* hw_device_ctx = nullptr;
static enum AVPixelFormat hw_pix_fmt;

void initlib()
{
	mutex m_mutex;
	lock_guard<mutex> lock(m_mutex);
	static bool isInit = false;
	avformat_network_init();
	auto version = avcodec_version();
	cout << "init ffmpeg lib ok." << "version:" << version<<endl;
}


int main(int argc, char** argv)
{
	clock_t start, finish;
	start = clock();
	//SDL
	SDL_Rect rect;
	Uint32 pixformat;
	SDL_Window *win = nullptr;
	SDL_Renderer *renderer = nullptr;
	SDL_Texture* texture = nullptr;

	// 默认窗体大小
	int w_width = 640;
	int w_height = 480;

	//SDL初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
		return 0;
	}
	//网络初始化
	initlib();

	//一帧完成
	int frameFinish;
	// 视频宽度
	int videoWidght;
	// 视频高度
	int videoHeight;
	// 上一次视频宽度
	int oldWidght;
	// 上一次视频高度
	int oldHeight;
	// 视频流索引
	int videoStreamIndex;
	//存储解码后图片buffer
	uint8_t *out_bufffer = nullptr;
	//包对象
	AVPacket *packet = nullptr;
	//输入帧对象
	AVFrame *pFrame = nullptr;
	//输出帧对象
	AVFrame *pFrameRPG = nullptr;
	//格式对象
	AVFormatContext *pFormatCtx = nullptr;
	//视频解码器
	AVCodecContext *videoCodec = nullptr;
	//处理图片数据对象
	SwsContext *img_convert_ctx = nullptr;

	//参数对象
	AVDictionary *avdic = nullptr;
	//视频解码
	AVCodec *videoDecoder = nullptr;

	

	//打开码流前指定各种参数比如：探测时间/超时时间/最大延时等
	//设置缓存大小，1080p可以将值调大
	 //设置缓存大小,1080p可将值调大
	av_dict_set(&avdic, "buffer_size", "8192000", 0);
	//以tcp方式打开,如果以udp方式打开将tcp替换为udp
	av_dict_set(&avdic, "rtsp_transport", "tcp", 0);
	//设置超时断开连接时间,单位微秒,3000000表示3秒
	av_dict_set(&avdic, "stimeout", "3000000", 0);
	//设置最大时延,单位微秒,1000000表示1秒
	av_dict_set(&avdic, "max_delay", "1000000", 0);
	//自动开启线程数
	av_dict_set(&avdic, "threads", "auto", 0);
	
	//获取输入的文件名
	const char* input = "../../3rd/video/test.mp4";

	//封装格式上下文
	pFormatCtx = avformat_alloc_context();

    //打开视频流
	int ret = avformat_open_input(&pFormatCtx, input, nullptr, nullptr);
	if (ret < 0) {
		cout << "open input error." << endl;
		return 0;
	}
	//cout << "open input successful." << endl;

	//释放设置参数
	if (avdic != nullptr) {
		av_dict_free(&avdic);
	}
	
	//获取流信息
	ret = avformat_find_stream_info(pFormatCtx, nullptr);
	if (ret < 0) {
		cout << "find stream info error." << endl;
		return 0;
	}

	//视频流开始
	videoStreamIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
	if (videoDecoder == nullptr) {
		cout << "find video stream index error." << endl;
		return 0;
	}


	//获取视频流
	AVStream* videoStream = pFormatCtx->streams[videoStreamIndex];

	//获取视频流解码器或者指定解码器
	videoCodec = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(videoCodec, videoStream->codecpar);

	videoDecoder = avcodec_find_decoder(videoCodec->codec_id);
	//videoDecoder = avcodec_find_decoder_by_name("h264_qsv");
	if (videoDecoder == nullptr) {
		cout << "video decoder not fond." << endl;
		return 0;
	}
	

	//设置加速解码
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;

	//videoCodec->get_format = videoDecoder; b

	//打开解码器
	ret = avcodec_open2(videoCodec, videoDecoder, nullptr);
	if (ret < 0) {
		cout << "open video codec error" << endl;
		return 0;
	}

	//获取分辨率大小
	videoWidght = videoCodec->width;
	videoHeight = videoCodec->height;

	//如果没有获取到宽高则返回
	if (videoHeight == 0 || videoWidght == 0) {
		cout << "find width height error" << endl;
		return 0;
	}
	//
	//输出视频信息
	printf("video stream information->index: %d ,decodec: %s ,format: %s time: %lld s resolution:%d*%d",
		videoStreamIndex,videoDecoder->name,pFormatCtx->iformat->name,(pFormatCtx->duration)/1000000,videoWidght,videoHeight);
	
	//预分配好内存
	packet = av_packet_alloc();
	pFrame = av_frame_alloc();
	pFrameRPG = av_frame_alloc();

	//SDL
	w_width = videoWidght;
	w_height = videoHeight;

	//创建窗口
	win = SDL_CreateWindow("Player",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		w_width,
		w_height,
		SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

	if (!win) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
		return 0;
	}

	//创建渲染器
	renderer = SDL_CreateRenderer(win, -1, 0);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
		return 0;
	}

	//SDL格式yuv
	pixformat = SDL_PIXELFORMAT_IYUV;
	//创建纹理
	texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		w_width, w_height);

	//计数
	int num = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		int index = packet->stream_index;
		if (index == videoStreamIndex) {
			frameFinish = avcodec_send_packet(videoCodec, packet);
			if (frameFinish < 0) {
				continue;
			}
			frameFinish = avcodec_receive_frame(videoCodec, pFrame);
			if (frameFinish < 0) {
				continue;
			}
			if (frameFinish >= 0) {
				num++;
				printf("finish decode %d frame\n", num);
				SDL_UpdateYUVTexture(texture, nullptr,
					pFrame->data[0],pFrame->linesize[0],
					pFrame->data[1],pFrame->linesize[1],
					pFrame->data[2],pFrame->linesize[2]);
				//Set size of window
				rect.x = 0;
				rect.y = 0;
				rect.w = videoCodec->width;
				rect.h = videoCodec->height;
				//展示
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, &rect);
				SDL_RenderPresent(renderer);
				SDL_Delay(40);
			}
		}
		av_packet_unref(packet);
		av_freep(packet);

		
	}
	if (img_convert_ctx != nullptr) {
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = nullptr;
	}

	if (packet != nullptr) {
		av_packet_unref(packet);
		packet = nullptr;
	}

	if (pFrame != nullptr) {
		av_frame_free(&pFrame);
		pFrame = nullptr;
	}

	if (pFrame != nullptr) {
		av_frame_free(&pFrame);
		pFrame = nullptr;
	}
	if (videoCodec != nullptr) {
		avcodec_close(videoCodec);
		videoCodec = nullptr;
	}

	if (pFormatCtx != nullptr) {
		avformat_close_input(&pFormatCtx);
		pFormatCtx = nullptr;
	}

	//SDL free

	if(win) {
		SDL_DestroyWindow(win);
	}

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (texture) {
		SDL_DestroyTexture(texture);
	}
	finish = clock();
	//clock()函数返回此时CPU时钟计时单元数
	cout << endl << "the time cost is:" << double(finish - start) / CLOCKS_PER_SEC << endl;
	return 0;
}