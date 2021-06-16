#ifndef QDECODE_H
#define QDECODE_H
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

};

#endif