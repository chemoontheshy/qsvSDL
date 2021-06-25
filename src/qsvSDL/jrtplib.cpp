#include "jrtplib.h"

std::mutex m_mutex;
/// <summary>
/// ���캯��
/// </summary>
JRTPLIB::JRTPLIB()
{
	packetBuf = NULL;
	packetBuf = (unsigned char*)malloc(PACKET_LEN * sizeof(unsigned char));
	packetLen = 0;
	portbase = 4002;
	status = -1;
	
}

/// <summary>
/// ��������
/// </summary>
JRTPLIB::~JRTPLIB()
{
	free();
}

void JRTPLIB::setPort(const uint16_t _portbase)
{
// windows��أ�����Ҫ����ɾ��
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	portbase = _portbase;
	//�������
	RTPUDPv4TransmissionParams transparams;
	//�Ự����
	RTPSessionParams sessparams;
	//����ʱ���
	sessparams.SetOwnTimestampUnit(1 / 9000);
	//�Ƿ�����Լ��ķ��͵İ�
	sessparams.SetAcceptOwnPackets(false);
	//���ý��ն˿�
	transparams.SetPortbase(_portbase);
	//�����Ự
	status = sess.Create(sessparams, &transparams);
	checkerror(status);
	
}

void JRTPLIB::getVersion()
{
	// ��ӡJRTPLIB�汾
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;
}

void JRTPLIB::getRTPPacket()
{
	while (true)
	{
		sess.BeginDataAccess();

		// ����հ�
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;

				while ((pack = sess.GetNextPacket()) != NULL)
				{
					// ������������ݴ���
					//printf("Got packet !\n");
					// ������Ҫ������ˣ�ɾ��֮
					unPackRTPToh264(pack->GetPayloadData(), pack->GetPayloadLength());
					sess.DeletePacket(pack);
				}
			} while (sess.GotoNextSourceWithData());
		}

		sess.EndDataAccess();

		// �ⲿ����JThread�����
#ifndef RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif // RTP_SUPPORT_THREAD

		// �ȴ�һ�룬�������
		//RTPTime::Wait(RTPTime(1, 0));
	}
	free();
}


HPacket JRTPLIB::getPackets()
{
	return packet;
}

void JRTPLIB::delPacket()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	packets.pop_front();
}

void JRTPLIB::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr);
		exit(-1);
	}

}

void JRTPLIB::unPackRTPToh264(void* pack_data, int pack_len)
{
	if (pack_len < RTP_HEADLEN) {
		return;
	}

	unsigned  char* src = (unsigned  char*)pack_data + RTP_HEADLEN;
	unsigned  char  head1 = *src; // ��ȡ��һ���ֽ�
	unsigned  char  head2 = *(src + 1); // ��ȡ�ڶ����ֽ�
	unsigned  char  nal = head1 & 0x1f; // ��ȡFU indicator��������
	unsigned  char  flag = head2 & 0xe0; // ��ȡFU header��ǰ��λ���жϵ�ǰ�Ƿְ��Ŀ�ʼ���м�����
	unsigned  char  nal_fua = (head1 & 0xe0) | (head2 & 0x1f); // FU_A nal
	bool  bFinishFrame = false;
	//std::cout << "nal:" << +nal << std::endl;
	if (nal == 0x1c) // �ж�NAL������Ϊ0x1c=28��˵����FU-A��Ƭ
	{ // fu-a
		if (flag == 0x80) // ��ʼ
		{
			packetBuf = src - 3;
			*((int*)packetBuf) = 0x01000000; // zyf:��ģʽ��������
			*((char*)packetBuf + 4) = nal_fua;
			packetLen = pack_len - RTP_HEADLEN + 3;
			
		}
		else   if (flag == 0x40) // ����
		{
			packetBuf = src + 2;
			packetLen = pack_len - RTP_HEADLEN - 2;
		}
		else // �м�
		{
			packetBuf = src + 2;
			packetLen = pack_len - RTP_HEADLEN - 2;
		}
	}
	else // ��������
	{
		packetBuf = src - 4;
		*((int*)packetBuf) = 0x01000000; // zyf:��ģʽ��������
		packetLen = pack_len - RTP_HEADLEN + 4;
	}

	unsigned  char* bufTmp = (unsigned  char*)pack_data;
	if (bufTmp[1] & 0x80)
	{
		bFinishFrame = true; // rtp mark
		//std::cout << "test" << std::endl;
		std::lock_guard<std::mutex> lock(m_mutex);
		packet.data = packetBuf;
		packet.lenght = packetLen;
		//packets.push_back(packet);
	}
	else
	{
		bFinishFrame = false;
	}
	Sleep(1);

}



void JRTPLIB::free()
{
	
	// ���ٶԻ�
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Windows���
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
}