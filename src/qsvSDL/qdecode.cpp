#include "QDecode.h"


JRTPLIB jrtp;
//刷新事件
# define REFRESH_EVENT  (SDL_USEREVENT + 1) 

//线程每40ms刷新一次
int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void* opaque) {
	thread_exit = 0;

	while (!thread_exit) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(1000/30);
	}
	thread_exit = 0;
	return 0;
}

int runRtp(void* opaque)
{
	jrtp.getRTPPacket();
	return 0;
}


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
void QDecode::setPortbase(int portbase)
{
	jrtp.setPort(portbase);
}

/// <summary>
/// 开始播放
/// </summary>
void QDecode::play()
{
	pFormatCtx = avformat_alloc_context();
	//获取视频流解码器或者指定解码器
	
	AVCodecParserContext* parser =nullptr;
	//avcodec_parameters_to_context(videoCodec, avCod);

	videoDecoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	//videoDecoder = avcodec_find_decoder_by_name("h264_qsv");
	if (videoDecoder == nullptr) {
		cout << "video decoder not foud." << endl;
		return;
	}

	

	videoCodec = avcodec_alloc_context3(videoDecoder);

	//设置加速解码
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;
	//初始化
	parser = av_parser_init(videoDecoder->id);
	
	//打开解码器
	int ret = avcodec_open2(videoCodec, videoDecoder, nullptr);
	if (ret < 0) {
		cout << "open video codec error" << endl;
		return;
	}

	SDL_Thread* rtp;
	rtp = SDL_CreateThread(runRtp, NULL, NULL);
	uint8_t* poutbuf;
	int poutbuf_size;
	AVPacket *m_packet=av_packet_alloc();
	while (true) {
		if (jrtp.getPackets().size() <= 0) {
			//cout << "not packet" << endl;
			SDL_Delay(1);
			continue;
		}
		cout << "jrtp.getPackets().front().data" << jrtp.getPackets().front().lenght << endl;
		ret=av_parser_parse2(parser, videoCodec, &m_packet->data, &m_packet->size,
			jrtp.getPackets().front().data, jrtp.getPackets().front().lenght,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			cout << "Error while parsing" << endl;
			break;
		}
		jrtp.delPacket();
		if (m_packet->size>0) {
			cout << "packet.size：" << m_packet->size << endl;
		}
		//cout << "packet.size：" << m_packet->size<< endl;
		SDL_Delay(20);
	}
	return;
	

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

	//创建线程
	SDL_Thread* video_tid;
	SDL_Event event;
	video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
	//计数
	int num = 0;

	//分配内存
	packet = av_packet_alloc();
	pFrameNV12 = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	cout << "test" << endl;
	clock_t start, finish;
	start = clock();
	for (;;) {
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT) {
			while (1) {
				if (av_read_frame(pFormatCtx, packet) < 0) {
					finish = clock();
					//clock()函数返回此时CPU时钟计时单元数
					cout << endl << "the time cost is:" << float(finish - start) / CLOCKS_PER_SEC << endl;

					free();
					thread_exit = 1;
				}
				if (packet->stream_index == videoStreamIndex)
					break;
			}
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
			}
			av_packet_unref(packet);
			av_freep(packet);
		}
		else if (event.type == SDL_QUIT) {
			
			thread_exit = 1;
		}
	}
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
