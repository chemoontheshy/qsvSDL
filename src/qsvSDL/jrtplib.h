#ifndef JRPTLIB_H
#define JRPTLIB_H
#pragma once

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtperrors.h>
#include <jrtplib3/rtplibraryversion.h>
#include <jrtplib3/rtppacket.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

using namespace jrtplib;


//定义RTP头？
constexpr auto RTP_HEADLEN = 12;
constexpr auto PACKET_LEN = 1024 * 300;

struct HPacket {
	unsigned char* data = NULL;
	unsigned int lenght = 0;
};



class JRTPLIB
{
public:
	/// <summary>
	/// 构造函数
	/// </summary>
	explicit JRTPLIB();

	/// <summary>
	/// 析构函数
	/// </summary>
	~JRTPLIB();

public:
	void setPort(const uint16_t portbase);
	
	//打印版本号
	void getVersion();

	//获取RTPPacket
	void getRTPPacket();

	//获取HPacketslist
	HPacket getPackets();

	//获取RTPPacket
	void delPacket();

private:
	//打印错误信息
	void checkerror(int rtperr);
	//解码RTP H.264视频
	void unPackRTPToh264(void* pack_data, int len);
	//结束RTP会话
	void free();
	

private:
	//RTP会话
	RTPSession sess;
	//接受RTP的端口号
	uint16_t portbase;
	//标志
	int status;
	//计算
	int num = 0;
	unsigned char* packetBuf; //用来存储解码分片包
	unsigned int packetLen; //记录已经存储的长度
	std::list<HPacket> packets;
	HPacket packet;
};

#endif