#include "jrtplib.h"

std::mutex m_mutex;
/// <summary>
/// ���캯��
/// </summary>
JRTPLIB::JRTPLIB()
{
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
					bool flag = unPackRTPToh264(pack->GetPayloadData(), pack->GetPayloadLength());
					if (flag) {
						//std::cout << "packets.size" << packets.size() << std::endl;
					}
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


std::list <HPacket> JRTPLIB::getPackets()
{
	return packets;
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

bool JRTPLIB::unPackRTPToh264(void* pack_data, int pack_len)
{
	if (pack_len < RTP_HEADLEN) {
		return false;
	}
	//��Ƶ��
	unsigned char* src = (unsigned char*)pack_data + RTP_HEADLEN;
	//��ȡ��һ���ֽ�
	unsigned char head1 = *src;
	//��ȡ�ڶ����ֽ�
	unsigned char head2 = *(src+1);
	//��ȡFU indicator��������
	unsigned char nal = head1 & 0x1f;
	//��ȡFU header��ǰ��λ���жϵ�ǰ�Ƿְ��Ŀ�ʼ���м䣬�����
	unsigned char flag = head2 & 0xe0;
	//FU_A nal
	unsigned char nal_fua = (head1 & 0xe0) | (head2 & 0x1f);
	//���һ֡��־
	bool bFinishFrame = false;
	//std::cout << "begin" << std::endl;

	//int b = (int)nal;
	//std::cout << "nal  " << b << std::endl;
	//�ж�NAL������Ϊ0x1c=28,˵����FU-A��Ƭ
	if (nal == 0x1c)
	{
		//��ʼ
		if (flag == 0x80) {
			outBuf = src - 3;
			*((uint8_t*)(outBuf)) = 0x01000000;
			*((uint8_t*)outBuf + 4) = nal_fua;
			outLen = pack_len - RTP_HEADLEN + 3;
		}
		else if (flag == 0x40) {
			outBuf = src + 2;
			outLen = pack_len - RTP_HEADLEN -2;
		}
		else {
			outBuf = src + 2;
			outLen = pack_len - RTP_HEADLEN - 2;
		}
	}
	//��������
	else {
		outBuf = src - 4;
		*((uint8_t*)(outBuf)) = 0x01000000;
		outLen = pack_len - RTP_HEADLEN + 4;
	}
	uint8_t* bufTemp = (uint8_t*)outBuf;
	//std::cout << "size: " << sizeof(&bufTemp) << std::endl;
	if (bufTemp[1] & 0x80) {
		bFinishFrame = true;
		HPacket packet;
		std::lock_guard<std::mutex> lock(m_mutex);
		packet.data = outBuf;
		packet.lenght = outLen;
		outBuf = NULL;
		outLen = 0;
		packets.push_back(packet);
		//num++;
		//std::cout << "num: "<<num << std::endl;
		//std::cout << "packet.lenght:" << packet.data << std::endl;
	}
	else {
		bFinishFrame = false;
	}

	return bFinishFrame;
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