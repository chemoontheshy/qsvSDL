#include "QDecode.h"

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
		SDL_Delay(1000 / 60);
	}
	thread_exit = 0;
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
	// windows相关，不需要可以删了
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	//传输参数
	RTPUDPv4TransmissionParams transparams;
	//会话参数
	RTPSessionParams sessparams;
	//设置时间戳
	sessparams.SetOwnTimestampUnit(1 / 9000);
	//是否接受自己的发送的包
	sessparams.SetAcceptOwnPackets(false);
	//设置接收端口
	transparams.SetPortbase(portbase);
	//创建会话
	status = sess.Create(sessparams, &transparams);
	checkerror(status);

}

/// <summary>
/// 开始播放
/// </summary>
void QDecode::play()
{
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	//获取视频流解码器或者指定解码器
	//设置PPS
	AVCodecParserContext* parser = nullptr;
	AVCodecParameters* para = avcodec_parameters_alloc();
	

	videoDecoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	//videoDecoder = avcodec_find_decoder_by_name("h264_qsv");
	if (videoDecoder == nullptr) {
		std::cout << "video decoder not foud." << std::endl;
		return;
	}
	parser = av_parser_init(videoDecoder->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		return;
	}
	videoCodec = avcodec_alloc_context3(nullptr);
	if (!videoCodec) {
		fprintf(stderr, "Could not allocate video codec context\n");
		return;
	}

	avcodec_parameters_to_context(videoCodec, para);
	//设置加速解码
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;
	//初始化

	unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
				  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
	videoCodec->extradata_size = 23;
	videoCodec->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
	if (videoCodec->extradata == NULL) {
		printf("could not av_malloc the video params extradata!\n");
		return;
	}
	memcpy(videoCodec->extradata, sps_pps, 23);
	//打开解码器
	videoCodec->width = 1920;
	videoCodec->height = 1088;

	int ret = avcodec_open2(videoCodec, videoDecoder, nullptr);
	if (ret < 0) {
		std::cout << "open video codec error" << std::endl;
		return;
	}

	uint8_t* poutbuf;
	int poutbuf_size;
	AVPacket* m_packet = av_packet_alloc();
	AVFrame* m_frame = av_frame_alloc();
	int num = 0;
	while (1) {
		sess.BeginDataAccess();

		// 检查收包
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;

				while ((pack = sess.GetNextPacket()) != NULL)
				{
					// 在这里进行数据处理
					//printf("Got packet !\n");
					// 不再需要这个包了，删除之
					m_timeBase = pack->GetTimestamp();
					Mem mem;
					mem.data = pack->GetPayloadData();
					mem.lenght = pack->GetPayloadLength();
					auto m_mem = unPackRTPToh264(mem);
					//cout << "verison:" << pack-> << endl;
					ret = av_parser_parse2(parser, videoCodec, &m_packet->data, &m_packet->size,
						m_mem.data, m_mem.lenght,
						AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
					if (ret < 0) {
						cout << "Error while parsing" << endl;
						break;
					}
					sess.DeletePacket(pack);
					m_packet->pts = m_timeBase;
					if (m_packet->size > 0) {

						frameFinish = avcodec_send_packet(videoCodec, m_packet);
						SDL_Delay(2);
						if (frameFinish < 0) {
							continue;
						}
						//cout << "frameFinish" << frameFinish << endl;
						frameFinish = avcodec_receive_frame(videoCodec, m_frame);
						SDL_Delay(7);
						if (0 > frameFinish) {
							continue;
						}
						if (frameFinish >= 0) {
							num++;
							cout << "finish decode " << num << " frame" << endl;
						}
					}
					SDL_Delay(2);
					av_packet_unref(m_packet);
					av_freep(m_packet);
				}
			} while (sess.GotoNextSourceWithData());
		}

		sess.EndDataAccess();

		// 这部分与JThread库相关
#ifndef RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif // RTP_SUPPORT_THREAD

		// 等待一秒，发包间隔
		//RTPTime::Wait(RTPTime(1, 0))
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
	videoWidth = 1920;
	videoHeight = 1280;
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
	//rect = nullptr;
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

	// 销毁对话
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Windows相关
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK

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


void QDecode::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr);
		exit(-1);
	}

}
Mem  QDecode::unPackRTPToh264(Mem &mem)
{
	static constexpr uint32_t START_CODE = 0x01000000;

	if (mem.lenght < 2) {
		printf("stream is too short to unpack");
	}
	fu_indicator* fu_ind = reinterpret_cast<fu_indicator*>(mem.data);
	fu_header* fu_hdr = reinterpret_cast<fu_header*>(mem.data + 1);

	if (0x1C == fu_ind->nal_unit_type) { // 判断NAL的类型为0x1C，说明是FU-A分片
		if (1 == fu_hdr->s) { // 如果是一帧的开始
			unsigned char nal_hdr = (((*mem.data) & 0xE0) | ((*(mem.data + 1)) & 0x1F));
			int offset = -3;
			(*reinterpret_cast<uint32_t*>(mem.data + offset)) = START_CODE;
			if ((6 != nal_hdr) && (7 != nal_hdr) && (8 != nal_hdr)) {
				offset = -2;
			}
			(*(mem.data + 1)) = nal_hdr;
			mem.data += offset;
			mem.lenght -= offset;
			return mem;
		}
		// 如果是一帧中间或结束
		mem.data += 2;
		mem.lenght -= 2;
		return mem;
	}
	// 单包数据
	int offset = -4;
	(*reinterpret_cast<uint32_t*>(mem.data + offset)) = START_CODE;
	if ((6 != fu_ind->nal_unit_type) && (7 != fu_ind->nal_unit_type) && (8 != fu_ind->nal_unit_type)) {
		offset = -3;
	}
	mem.data += offset;
	mem.lenght -= offset;
	return mem;
}