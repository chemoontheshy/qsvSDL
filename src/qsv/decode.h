#ifndef DECODE_H
#define DECODE_H
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
//hwcontext:Ӳ�������
#include <libavutil/hwcontext.h>
//SDL2
#include <SDL.h>
}
//��׼��
#include<iostream>
//�߳���
#include<mutex>
//ʱ��
#include<time.h>

using namespace std;

class Decode
{
public:
	explicit Decode();
	~Decode();
};

#endif
