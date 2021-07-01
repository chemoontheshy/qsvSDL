#ifndef QDECODE_H
#define QDECODE_H
#pragma once
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

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtperrors.h>
#include <jrtplib3/rtplibraryversion.h>
#include <jrtplib3/rtppacket.h>
#include <stdlib.h>
#include <stdio.h>
//��׼��
#include<iostream>
//�߳���
#include<mutex>
//�߳�
#include<thread>
//ʱ��
#include<time.h>

using namespace jrtplib;


using namespace std;


struct HPacket {
    unsigned char* data = NULL;
    unsigned int lenght = 0;
};

/// <summary>
        /// FU Indicator�ṹ��
        /// </summary>
typedef struct fu_indicator
{
    unsigned char nal_unit_type : 5;
    unsigned char nal_ref_idc : 2;
    unsigned char f_unuse : 1;
} fu_indicator;

/// <summary>
/// FUͷ�ṹ��
/// </summary>
typedef struct fu_header
{
    unsigned char type : 5;
    unsigned char r : 1;
    unsigned char e : 1;
    unsigned char s : 1;
} fu_header;

typedef struct Mem
{
    uint8_t* data;
    size_t lenght;
};

/// <summary>
/// QSV����+SDL��ʾ
/// </summary>
class QDecode
{

public:
	/// <summary>
	/// ���캯��
	/// </summary>
	explicit QDecode();

    /// <summary>
    /// ��������
    /// </summary>
	~QDecode();

public:
    /// <summary>
    /// ���õ�ַ
    /// </summary>
    void setUrl(const char* url);

    /// <summary>
    /// ���õ�ַ
    /// </summary>
    void setPortbase(int url);

    /// <summary>
    /// ��ʼ����
    /// </summary>
    void play();

    /// <summary>
    /// ��ֹ����
    /// </summary>
    void stop();
    

private:
    /// <summary>
    /// ��ʼ��FFmpeg���
    /// </summary>
    /// <returns>��������0�����󷵻�-1</returns>
    int initFFmepg();
    /// <summary>
    /// ��ʼ��SDL���
    /// </summary>
    /// <returns>��������0�����󷵻�-1</returns>

    int initSDL();

    /// <summary>
    /// �ͷ��ڴ�
    /// </summary>
    void free();

    /// <summary>
    /// NV12��ʽתΪYUV420P
    /// </summary>
    /// <param name="nv12_frame">NV12��ʽ��frame</param>
    /// <returns>YUV420P��ʽ��frame</returns>
    static AVFrame* nv12_to_yuv420P(AVFrame* nv12_frame);

private:
    /// <summary>
    /// ֹͣ��־
    /// </summary>
    volatile bool stopped;         

    /// <summary>��ʾ����һ֡���</summary>
    int frameFinish; 

    /// <summary>��Ƶ���</summary>
    int videoWidth;                

    /// <summary>��Ƶ�߶�</summary>
    int videoHeight;              

    /// <summary>��Ƶ������</summary>
    int videoStreamIndex;     

    /// <summary>��Ƶ����ַ</summary>
    const char* url;         

    /// <summary>Packet</summary>
    AVPacket* packet;        

    /// <summary>NV12��ʽ��frame:��h264_qsv���������</summary>
    AVFrame* pFrameNV12;   
   
    /// <summary>YUV420��ʽ��frame:�ŵ�SDL����ʾ</summary>
    AVFrame* pFrameYUV;   

    /// <summary>��������</summary>
    AVFormatContext* pFormatCtx;

    /// <summary>��Ƶ����������</summary>
    AVCodecContext* videoCodec;  

    /// <summary>����������Ҫ���������Ƶ��</summary>
    AVDictionary* avdic;     

    /// <summary>��Ƶ������</summary>
    AVCodec* videoDecoder;   

    //SDL
    /// <summary>SDL_��ʾ��λ��</summary>
    SDL_Rect rect;

    /// <summary>SDL��ʾ�ĸ�ʽ</summary>
    Uint32 pixformat;

    /// <summary>SDL�Ĵ���</summary>
    SDL_Window* win;

    /// <summary>��Ⱦ����״̬</summary>
    SDL_Renderer* renderer;

    /// <summary>����</summary>
    SDL_Texture* texture;

    /// <summary>�߳�ʵ��</summary>
    SDL_Event event;

private:
    //��ӡ������Ϣ
    void checkerror(int rtperr);
    //����RTP H.264��Ƶ
    Mem unPackRTPToh264(Mem&rtpPacket);

private:
    //RTP�Ự
    RTPSession sess;
    //����RTP�Ķ˿ں�
    uint16_t portbase;
    //��־
    int status;
    //����
    int64_t m_timeBase =0;
    Mem mem;

};

#endif