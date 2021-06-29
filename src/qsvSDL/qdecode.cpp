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
		SDL_Delay(1000 / 60);
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
	//���ü��ٽ���
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;
	//��ʼ��

	unsigned char sps_pps[23] = { 0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x0f, 0x00, 0x44, 0xbe, 0x8,
				  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };
	videoCodec->extradata_size = 23;
	videoCodec->extradata = (uint8_t*)av_malloc(23 + AV_INPUT_BUFFER_PADDING_SIZE);
	if (videoCodec->extradata == NULL) {
		printf("could not av_malloc the video params extradata!\n");
		return;
	}
	memcpy(videoCodec->extradata, sps_pps, 23);
	//�򿪽�����
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

		// ����հ�
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;

				while ((pack = sess.GetNextPacket()) != NULL)
				{
					// ������������ݴ���
					//printf("Got packet !\n");
					// ������Ҫ������ˣ�ɾ��֮
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

		// �ⲿ����JThread�����
#ifndef RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif // RTP_SUPPORT_THREAD

		// �ȴ�һ�룬�������
		//RTPTime::Wait(RTPTime(1, 0))
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
/// ��ʼ��SDL���
/// </summary>
/// <returns>��������0�����󷵻�-1</returns>
int QDecode::initSDL()
{
	win = nullptr;
	renderer = nullptr;
	texture = nullptr;
	//rect = nullptr;
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

	if (0x1C == fu_ind->nal_unit_type) { // �ж�NAL������Ϊ0x1C��˵����FU-A��Ƭ
		if (1 == fu_hdr->s) { // �����һ֡�Ŀ�ʼ
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
		// �����һ֡�м�����
		mem.data += 2;
		mem.lenght -= 2;
		return mem;
	}
	// ��������
	int offset = -4;
	(*reinterpret_cast<uint32_t*>(mem.data + offset)) = START_CODE;
	if ((6 != fu_ind->nal_unit_type) && (7 != fu_ind->nal_unit_type) && (8 != fu_ind->nal_unit_type)) {
		offset = -3;
	}
	mem.data += offset;
	mem.lenght -= offset;
	return mem;
}