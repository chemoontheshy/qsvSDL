#ifndef QDECODE_H
#define QDECODE_H
#pragma once
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
//hwcontext:硬解码相关
#include <libavutil/hwcontext.h>
//SDL2
#include <SDL.h>
}
//标准库
#include<iostream>
//线程锁
#include<mutex>
//时间
#include<time.h>



using namespace std;


/// <summary>
/// QSV解码+SDL显示
/// </summary>
class QDecode
{

public:
	/// <summary>
	/// 构造函数
	/// </summary>
	explicit QDecode();

    /// <summary>
    /// 析构函数
    /// </summary>
	~QDecode();

public:
    /// <summary>
    /// 设置地址
    /// </summary>
    void setUrl(const char* url);

    /// <summary>
    /// 开始播放
    /// </summary>
    void play();

    /// <summary>
    /// 中止播放
    /// </summary>
    void stop();
    

private:
    /// <summary>
    /// 初始化FFmpeg相关
    /// </summary>
    /// <returns>正常返回0，错误返回-1</returns>
    int initFFmepg();
    /// <summary>
    /// 初始化SDL相关
    /// </summary>
    /// <returns>正常返回0，错误返回-1</returns>

    int initSDL();

    /// <summary>
    /// 释放内存
    /// </summary>
    void free();

    /// <summary>
    /// NV12格式转为YUV420P（已弃用，耗时太长）
    /// </summary>
    /// <param name="nv12_frame">NV12格式的frame</param>
    /// <returns>YUV420P格式的frame</returns>
    static AVFrame* nv12_to_yuv420P(AVFrame* nv12_frame);



private:
    /// <summary>
    /// 停止标志
    /// </summary>
    volatile bool stopped;         

    /// <summary>表示解码一帧完成</summary>
    int frameFinish; 

    /// <summary>视频宽度</summary>
    int videoWidth;                

    /// <summary>视频高度</summary>
    int videoHeight;              

    /// <summary>视频流索引</summary>
    int videoStreamIndex;     

    /// <summary>视频流地址</summary>
    const char* url;         

    /// <summary>Packet</summary>
    AVPacket* packet;        

    /// <summary>NV12格式的frame:由h264_qsv解码出来的</summary>
    AVFrame* pFrameNV12;   
   
    /// <summary>YUV420格式的frame:放到SDL里显示</summary>
    AVFrame* pFrameYUV;   

    /// <summary>总上下文</summary>
    AVFormatContext* pFormatCtx;

    /// <summary>视频解码上下文</summary>
    AVCodecContext* videoCodec;  

    /// <summary>参数对象：主要针对网络视频流</summary>
    AVDictionary* avdic;     

    /// <summary>视频解码器</summary>
    AVCodec* videoDecoder;   

    //SDL
    /// <summary>SDL_显示的位置</summary>
    SDL_Rect rect;

    /// <summary>SDL显示的格式</summary>
    Uint32 pixformat;

    /// <summary>SDL的窗口</summary>
    SDL_Window* win;

    /// <summary>渲染器的状态</summary>
    SDL_Renderer* renderer;

    /// <summary>纹理</summary>
    SDL_Texture* texture;

    SDL_Event event;

};

#endif