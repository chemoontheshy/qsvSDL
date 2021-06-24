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


//����RTPͷ��
constexpr auto RTP_HEADLEN = 12;

struct HPacket {
	uint8_t* data = NULL;
	int lenght = 0;
};

class JRTPLIB
{
public:
	/// <summary>
	/// ���캯��
	/// </summary>
	explicit JRTPLIB();

	/// <summary>
	/// ��������
	/// </summary>
	~JRTPLIB();

public:
	void setPort(const uint16_t portbase);
	
	//��ӡ�汾��
	void getVersion();

	//��ȡRTPPacket
	void getRTPPacket();

	//��ȡHPacketslist
	std::list<HPacket> getPackets();

	//��ȡRTPPacket
	void delPacket();

private:
	//��ӡ������Ϣ
	void checkerror(int rtperr);
	//����RTP H.264��Ƶ
	bool unPackRTPToh264(void* pack_data, int len);
	//����RTP�Ự
	void free();
	

private:
	//RTP�Ự
	RTPSession sess;
	//����RTP�Ķ˿ں�
	uint16_t portbase;
	//��־
	int status;
	//����
	int num = 0;
	uint8_t* outBuf;
	int outLen;
	std::list<HPacket> packets;
};

#endif