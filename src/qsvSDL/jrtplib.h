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
constexpr auto PACKET_LEN = 1024 * 300;

struct HPacket {
	unsigned char* data = NULL;
	unsigned int lenght = 0;
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
	HPacket getPackets();

	//��ȡRTPPacket
	void delPacket();

private:
	//��ӡ������Ϣ
	void checkerror(int rtperr);
	//����RTP H.264��Ƶ
	void unPackRTPToh264(void* pack_data, int len);
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
	unsigned char* packetBuf; //�����洢�����Ƭ��
	unsigned int packetLen; //��¼�Ѿ��洢�ĳ���
	std::list<HPacket> packets;
	HPacket packet;
};

#endif