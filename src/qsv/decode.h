#ifndef DECODE_H
#define DECODE_H
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

class Decode
{
public:
	explicit Decode();
	~Decode();
};

#endif
