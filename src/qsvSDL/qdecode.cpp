#include "QDecode.h"

//ˢ���¼�
# define REFRESH_EVENT  (SDL_USEREVENT + 1) 

//�߳�ÿ40msˢ��һ��
int thread_exit = 0;
int thread_pause = 0;

int sfp_refresh_thread(void* opaque) {
	thread_exit = 0;

	while (!thread_exit) {
		SDL_Event event;
		event.type = REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(1000 / 30);
	}
	thread_exit = 0;
	return 0;
}

/// <summary>
/// ���캯��
/// </summary>
QDecode::QDecode()
{
	initFFmepg();
	initSDL();
}

/// <summary>
/// ��������
/// </summary>
QDecode::~QDecode()
{
	free();
}

/// <summary>
/// ���õ�ַ
/// </summary>
void QDecode::setUrl(const char* _url)
{
	this->url = _url;
}
void QDecode::setPortbase(int portbase)
{
	// windows��أ�����Ҫ����ɾ��
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	//�������
	RTPUDPv4TransmissionParams transparams;
	//�Ự����
	RTPSessionParams sessparams;
	//����ʱ���
	sessparams.SetOwnTimestampUnit(1 / 9000);
	//�Ƿ�����Լ��ķ��͵İ�
	sessparams.SetAcceptOwnPackets(false);
	//���ý��ն˿�
	transparams.SetPortbase(portbase);
	//�����Ự
	status = sess.Create(sessparams, &transparams);
	checkerror(status);

}

/// <summary>
/// ��ʼ����
/// </summary>
void QDecode::play()
{
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	//��ȡ��Ƶ������������ָ��������
	//����PPS
	
	AVCodecParserContext* parser =nullptr;
	//avcodec_parameters_to_context(videoCodec, avCod);

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
	videoCodec = avcodec_alloc_context3(videoDecoder);
	
	if (!videoCodec) {
		fprintf(stderr, "Could not allocate video codec context\n");
		return;
	}

	/*unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
					  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
	videoCodec->extradata_size = 23;
	videoCodec->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
	memcpy(videoCodec->extradata, sps_pps, 23);*/

	//���ü��ٽ���
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;
	//��ʼ��
	
	
	//�򿪽�����
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
	int c_num = 0;
	bool on = false;
	//����  
	while (true) {
		if (jrtp.getPackets().lenght <= 0) {
			//cout << "not packet" << endl;
			SDL_Delay(1);
			continue;
		}
		ret=av_parser_parse2(parser, videoCodec, &m_packet->data, &m_packet->size,
			reinterpret_cast<uint8_t*>((jrtp.getPackets().data)), jrtp.getPackets().lenght,
			AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			cout << "Error while parsing" << endl;
			break;
		}
		if (m_packet->size>0) {
			cout << "test" << endl;
			frameFinish = avcodec_send_packet(videoCodec, m_packet);
			if (frameFinish < 0) {
				continue;
			}
			frameFinish = avcodec_receive_frame(videoCodec,m_frame);
			if (frameFinish < 0) {
				continue;
			}
			if (frameFinish >= 0) {
				num++;
				printf("finish decode %d frame\n", num);
			}
			
		}
		av_packet_unref(m_packet);
		av_freep(m_packet);
		//cout << "packet.size��" << m_packet->size<< endl;
		
	}
	return;




	//SDL
	//��������
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

	//������Ⱦ��
	renderer = SDL_CreateRenderer(win, -1, 0);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
		return;
	}

	//��������
	texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		videoWidth, videoHeight);

	//�����߳�
	SDL_Thread* video_tid;
	SDL_Event event;
	video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
	//����
	num = 0;

	//�����ڴ�
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
					//clock()�������ش�ʱCPUʱ�Ӽ�ʱ��Ԫ��
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
				//չʾ
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
/// ��ֹ����
/// </summary>
void QDecode::stop()
{
	stopped = true;
	free();
}



/// <summary>
/// ��ʼ��FFmpeg���
/// </summary>
/// <returns>��������0�����󷵻�-1</returns>
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
/// ��ʼ��SDL���
/// </summary>
/// <returns>��������0�����󷵻�-1</returns>
int QDecode::initSDL()
{
	win = nullptr;
	renderer = nullptr;
	texture = nullptr;
	pixformat = SDL_PIXELFORMAT_IYUV;
	//SDL��ʼ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	return 0;
}

/// <summary>
/// �ͷ��ڴ�
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

	// ���ٶԻ�
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Windows���
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK

	cout << "decode end." << endl;
}


/// <summary>
/// NV12��ʽתΪYUV420P
/// </summary>
/// <param name="nv12_frame">NV12��ʽ��frame</param>
/// <returns>YUV420P��ʽ��frame</returns>
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
	//2. 32Ϊ��dui
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
