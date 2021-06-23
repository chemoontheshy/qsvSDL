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

private:
	//��ӡ������Ϣ
	void checkerror(int rtperr);
	//��ȡPacket
	void getPacket();
	//����RTP�Ự
	void free();

private:
	//RTP�Ự
	RTPSession sess;
	//����RTP�Ķ˿ں�
	uint16_t portbase;
	//
	int status;
	
};

#endif