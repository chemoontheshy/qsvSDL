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
//SDL2
#include <SDL.h>
}

#include<iostream>
#include<mutex>
#include<time.h>

using namespace std;

//Ӳ�������
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

	// Ĭ�ϴ����С
	int w_width = 640;
	int w_height = 480;

	//SDL��ʼ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
		return 0;
	}
	//�����ʼ��
	initlib();

	//һ֡���
	int frameFinish;
	// ��Ƶ���
	int videoWidght;
	// ��Ƶ�߶�
	int videoHeight;
	// ��һ����Ƶ���
	int oldWidght;
	// ��һ����Ƶ�߶�
	int oldHeight;
	// ��Ƶ������
	int videoStreamIndex;
	//�洢�����ͼƬbuffer
	uint8_t *out_bufffer = nullptr;
	//������
	AVPacket *packet = nullptr;
	//����֡����
	AVFrame *pFrame = nullptr;
	//���֡����
	AVFrame *pFrameRPG = nullptr;
	//��ʽ����
	AVFormatContext *pFormatCtx = nullptr;
	//��Ƶ������
	AVCodecContext *videoCodec = nullptr;
	//����ͼƬ���ݶ���
	SwsContext *img_convert_ctx = nullptr;

	//��������
	AVDictionary *avdic = nullptr;
	//��Ƶ����
	AVCodec *videoDecoder = nullptr;

	

	//������ǰָ�����ֲ������磺̽��ʱ��/��ʱʱ��/�����ʱ��
	//���û����С��1080p���Խ�ֵ����
	 //���û����С,1080p�ɽ�ֵ����
	av_dict_set(&avdic, "buffer_size", "8192000", 0);
	//��tcp��ʽ��,�����udp��ʽ�򿪽�tcp�滻Ϊudp
	av_dict_set(&avdic, "rtsp_transport", "tcp", 0);
	//���ó�ʱ�Ͽ�����ʱ��,��λ΢��,3000000��ʾ3��
	av_dict_set(&avdic, "stimeout", "3000000", 0);
	//�������ʱ��,��λ΢��,1000000��ʾ1��
	av_dict_set(&avdic, "max_delay", "1000000", 0);
	//�Զ������߳���
	av_dict_set(&avdic, "threads", "auto", 0);
	
	//��ȡ������ļ���
	const char* input = "../../3rd/video/test.mp4";

	//��װ��ʽ������
	pFormatCtx = avformat_alloc_context();

    //����Ƶ��
	int ret = avformat_open_input(&pFormatCtx, input, nullptr, nullptr);
	if (ret < 0) {
		cout << "open input error." << endl;
		return 0;
	}
	//cout << "open input successful." << endl;

	//�ͷ����ò���
	if (avdic != nullptr) {
		av_dict_free(&avdic);
	}
	
	//��ȡ����Ϣ
	ret = avformat_find_stream_info(pFormatCtx, nullptr);
	if (ret < 0) {
		cout << "find stream info error." << endl;
		return 0;
	}

	//��Ƶ����ʼ
	videoStreamIndex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
	if (videoDecoder == nullptr) {
		cout << "find video stream index error." << endl;
		return 0;
	}


	//��ȡ��Ƶ��
	AVStream* videoStream = pFormatCtx->streams[videoStreamIndex];

	//��ȡ��Ƶ������������ָ��������
	videoCodec = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(videoCodec, videoStream->codecpar);

	videoDecoder = avcodec_find_decoder(videoCodec->codec_id);
	//videoDecoder = avcodec_find_decoder_by_name("h264_qsv");
	if (videoDecoder == nullptr) {
		cout << "video decoder not fond." << endl;
		return 0;
	}
	

	//���ü��ٽ���
	videoCodec->lowres = videoDecoder->max_lowres;
	videoCodec->flags2 |= AV_CODEC_FLAG2_FAST;

	//videoCodec->get_format = videoDecoder; b

	//�򿪽�����
	ret = avcodec_open2(videoCodec, videoDecoder, nullptr);
	if (ret < 0) {
		cout << "open video codec error" << endl;
		return 0;
	}

	//��ȡ�ֱ��ʴ�С
	videoWidght = videoCodec->width;
	videoHeight = videoCodec->height;

	//���û�л�ȡ������򷵻�
	if (videoHeight == 0 || videoWidght == 0) {
		cout << "find width height error" << endl;
		return 0;
	}
	//
	//�����Ƶ��Ϣ
	printf("video stream information->index: %d ,decodec: %s ,format: %s time: %lld s resolution:%d*%d",
		videoStreamIndex,videoDecoder->name,pFormatCtx->iformat->name,(pFormatCtx->duration)/1000000,videoWidght,videoHeight);
	
	//Ԥ������ڴ�
	packet = av_packet_alloc();
	pFrame = av_frame_alloc();
	pFrameRPG = av_frame_alloc();

	//SDL
	w_width = videoWidght;
	w_height = videoHeight;

	//��������
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

	//����
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
				//չʾ
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
	//clock()�������ش�ʱCPUʱ�Ӽ�ʱ��Ԫ��
	cout << endl << "the time cost is:" << double(finish - start) / CLOCKS_PER_SEC << endl;
	return 0;
}