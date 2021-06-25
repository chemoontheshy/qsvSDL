#include "jrtplib.h"

std::mutex m_mutex;
/// <summary>
/// 构造函数
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
/// 析构函数
/// </summary>
JRTPLIB::~JRTPLIB()
{
	free();
}

void JRTPLIB::setPort(const uint16_t _portbase)
{
// windows相关，不需要可以删了
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSADATA dat;
	WSAStartup(MAKEWORD(2, 2), &dat);
#endif // RTP_SOCKETTYPE_WINSOCK
	portbase = _portbase;
	//传输参数
	RTPUDPv4TransmissionParams transparams;
	//会话参数
	RTPSessionParams sessparams;
	//设置时间戳
	sessparams.SetOwnTimestampUnit(1 / 9000);
	//是否接受自己的发送的包
	sessparams.SetAcceptOwnPackets(false);
	//设置接收端口
	transparams.SetPortbase(_portbase);
	//创建会话
	status = sess.Create(sessparams, &transparams);
	checkerror(status);
	
}

void JRTPLIB::getVersion()
{
	// 打印JRTPLIB版本
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;
}

void JRTPLIB::getRTPPacket()
{
	while (true)
	{
		sess.BeginDataAccess();

		// 检查收包
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;

				while ((pack = sess.GetNextPacket()) != NULL)
				{
					// 在这里进行数据处理
					//printf("Got packet !\n");
					// 不再需要这个包了，删除之
					unPackRTPToh264(pack->GetPayloadData(), pack->GetPayloadLength());
					sess.DeletePacket(pack);
				}
			} while (sess.GotoNextSourceWithData());
		}

		sess.EndDataAccess();

		// 这部分与JThread库相关
#ifndef RTP_SUPPORT_THREAD
		status = sess.Poll();
		checkerror(status);
#endif // RTP_SUPPORT_THREAD

		// 等待一秒，发包间隔
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
	unsigned  char  head1 = *src; // 获取第一个字节
	unsigned  char  head2 = *(src + 1); // 获取第二个字节
	unsigned  char  nal = head1 & 0x1f; // 获取FU indicator的类型域，
	unsigned  char  flag = head2 & 0xe0; // 获取FU header的前三位，判断当前是分包的开始、中间或结束
	unsigned  char  nal_fua = (head1 & 0xe0) | (head2 & 0x1f); // FU_A nal
	bool  bFinishFrame = false;
	//std::cout << "nal:" << +nal << std::endl;
	if (nal == 0x1c) // 判断NAL的类型为0x1c=28，说明是FU-A分片
	{ // fu-a
		if (flag == 0x80) // 开始
		{
			packetBuf = src - 3;
			*((int*)packetBuf) = 0x01000000; // zyf:大模式会有问题
			*((char*)packetBuf + 4) = nal_fua;
			packetLen = pack_len - RTP_HEADLEN + 3;
			
		}
		else   if (flag == 0x40) // 结束
		{
			packetBuf = src + 2;
			packetLen = pack_len - RTP_HEADLEN - 2;
		}
		else // 中间
		{
			packetBuf = src + 2;
			packetLen = pack_len - RTP_HEADLEN - 2;
		}
	}
	else // 单包数据
	{
		packetBuf = src - 4;
		*((int*)packetBuf) = 0x01000000; // zyf:大模式会有问题
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
	
	// 销毁对话
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Windows相关
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
}