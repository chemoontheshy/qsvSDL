#include "QDecode.h"

/// <summary>
/// 构造函数
/// </summary>
QDecode::QDecode()
{
	initFFmepg();
	initSDL();
}

/// <summary>
/// 析构函数
/// </summary>
QDecode::~QDecode()
{
	free();
}

/// <summary>
/// 设置地址
/// </summary>
void QDecode::setUrl(const char* _url)
{
	this->url = _url;
}

/// <summary>
/// 开始播放
/// </summary>
void QDecode::play()
{
	pFormatCtx = avformat_alloc_context();

	//打开视频流
	int ret = avformat_open_input(&pFormatCtx, url, nullptr, nullptr);
	if (ret < 0) {
		cout << "open url error" << endl;
		return;
	}

	//获取流信息
	ret = avformat_find_stream_info(pFormatCtx, nullptr);
	if (ret < 0) {
		cout << "find stream info error." << endl;
		return;
	}

	//视频流索引
	videoStreamIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
	if (videoDecoder == nullptr) {
		cout << "find video stream index error" << endl;
		return;
	}

	// 获取视频流
	AVStream* videoStream = pFormatCtx->streams[videoStreamIndex];

	//获取视频流解码器或者指定解码器
	videoCodec = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(videoCodec, videoStream->codecpar);

	//videoDecoder = avcodec_find_decoder(videoCodec->codec_id);
	videoDecoder = avcodec_find_decoder_by_name("h264_qsv");
	if (videoDecoder == nullptr) {
		cout << "video decoder not foud." << endl;
		return;
	}

	//设置加速解码
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;

	//打开解码器
	ret = avcodec_open2(videoCodec, videoDecoder, nullptr);
	if (ret < 0) {
		cout << "open video codec error" << endl;
		return;
	}

	//获取分辨率大小
	videoWidth = videoCodec->width;
	videoHeight = videoCodec->height;

	if (videoWidth == 0 || videoHeight == 0) {
		cout << "width or height error" << endl;
		return;
	}

	//SDL
	//创建窗口
	win = SDL_CreateWindow("qsvPlayer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		videoWidth,
		videoHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!win) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
		return;
	}

	//创建渲染器
	renderer = SDL_CreateRenderer(win, -1, 0);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
		return;
	}

	//创建纹理
	texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth, videoHeight);
	
	//计数
	int num = 0;

	//分配内存
	packet = av_packet_alloc();
	pFrameNV12 = av_frame_alloc();
	pFrameYUV = av_frame_alloc();

	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (stopped) {
			break;
		}
		if (packet->stream_index == videoStreamIndex) {
			frameFinish = avcodec_send_packet(videoCodec, packet);
			if (frameFinish < 0) {
				continue;
			}
			frameFinish = avcodec_receive_frame(videoCodec, pFrameNV12);
			if (frameFinish < 0) {
				continue;
			}
			if (frameFinish >= 0) {
				num++;
				printf("finish decode %d frame\n", num);
				pFrameYUV = nv12_to_yuv420P(pFrameNV12);
				SDL_UpdateYUVTexture(texture, nullptr,
					pFrameYUV->data[0], pFrameYUV->linesize[0],
					pFrameYUV->data[1], pFrameYUV->linesize[1],
					pFrameYUV->data[2], pFrameYUV->linesize[2]);
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
	free();

}

/// <summary>
/// 中止播放
/// </summary>
void QDecode::stop()
{
	stopped = true;
	free();
}


/// <summary>
/// 初始化FFmpeg相关
/// </summary>
/// <returns>正常返回0，错误返回-1</returns>
int QDecode::initFFmepg()
{
	stopped = false;
	frameFinish = -1;
	videoWidth = 0;
	videoHeight = 0;
	videoStreamIndex = -1;
	url = nullptr;
	packet = nullptr;
	pFrameNV12 = nullptr;
	pFrameYUV = nullptr;
	videoCodec = nullptr;
	avdic = nullptr;
	videoDecoder = nullptr;
	mutex m_mutex;
	lock_guard<mutex> lock(m_mutex);
	avformat_network_init();
	auto version = avcodec_version();
	cout << "init ffmpeg lib ok." << "version:" << version << endl;
	return 0;
}

/// <summary>
/// 初始化SDL相关
/// </summary>
/// <returns>正常返回0，错误返回-1</returns>
int QDecode::initSDL()
{
	win = nullptr;
	renderer = nullptr;
	texture = nullptr;
	pixformat = SDL_PIXELFORMAT_IYUV;
	//SDL初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	return 0;
}

/// <summary>
/// 释放内存
/// </summary>
void QDecode::free()
{
	//
	if (packet != nullptr) {
		av_packet_unref(packet);
		packet = nullptr;
	}

	if (pFrameNV12 != nullptr) {
		av_frame_free(&pFrameNV12);
		pFrameNV12 = nullptr;
	}

	if (pFrameYUV != nullptr) {
		av_frame_free(&pFrameYUV);
		pFrameYUV = nullptr;
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

	if (win) {
		SDL_DestroyWindow(win);
	}

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (texture) {
		SDL_DestroyTexture(texture);
	}
	SDL_Quit();
	cout << "decode end." << endl;
}


/// <summary>
/// NV12格式转为YUV420P
/// </summary>
/// <param name="nv12_frame">NV12格式的frame</param>
/// <returns>YUV420P格式的frame</returns>
AVFrame* QDecode::nv12_to_yuv420P(AVFrame* nv12_frame)
{
	///NV12 yyyy uvuv
	///YUV420P yyyy uu vv
	int x, y;
	//0
	AVFrame* frame = av_frame_alloc();

	//1. must be set
	frame->format = AV_PIX_FMT_YUV420P;
	frame->width = nv12_frame->width;
	frame->height = nv12_frame->height;
	//2. 32为了dui
	int ret = av_frame_get_buffer(frame, 64);
	if (ret < 0) {
		exit(1);
	}
	//3.make sure the frame data is writable;
	ret = av_frame_make_writable(frame);
	if (ret < 0) {
		exit(1);
	}

	//copy data
	//y
	if (nv12_frame->linesize[0] == nv12_frame->width) {
		memcpy(frame->data[0], frame->data[0], nv12_frame->height * nv12_frame->linesize[0]);
	}
	else {
		for (y = 0; y < frame->height; y++) {
			for (x = 0; x < frame->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = nv12_frame->data[0][y * nv12_frame->linesize[0] + x];
			}
		}
	}
	//cb and cr
	for (y = 0; y < frame->height / 2; y++) {
		for (x = 0; x < frame->width / 2; x++) {
			frame->data[1][y * frame->linesize[1] + x] = nv12_frame->data[1][y * nv12_frame->linesize[1] + 2 * x];
			frame->data[2][y * frame->linesize[2] + x] = nv12_frame->data[1][y * nv12_frame->linesize[1] + 2 * x + 1];
		}
	}
	return frame;
}