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

using namespace jrtplib;

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

private:
	//打印错误信息
	void checkerror(int rtperr);
	//获取Packet
	void getPacket();
	//结束RTP会话
	void free();

private:
	//RTP会话
	RTPSession sess;
	//接受RTP的端口号
	uint16_t portbase;
	//
	int status;
	
};

#endif