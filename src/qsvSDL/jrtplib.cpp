#include "jrtplib.h"

std::mutex m_mutex;
/// <summary>
/// 构造函数
/// </summary>
JRTPLIB::JRTPLIB()
{
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
					bool flag = unPackRTPToh264(pack->GetPayloadData(), pack->GetPayloadLength());
					if (flag) {
						//std::cout << "packets.size" << packets.size() << std::endl;
					}
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
	//视频流
	unsigned char* src = (unsigned char*)pack_data + RTP_HEADLEN;
	//获取第一个字节
	unsigned char head1 = *src;
	//获取第二个字节
	unsigned char head2 = *(src+1);
	//获取FU indicator的类型域
	unsigned char nal = head1 & 0x1f;
	//获取FU header的前三位，判断当前是分包的开始，中间，或结束
	unsigned char flag = head2 & 0xe0;
	//FU_A nal
	unsigned char nal_fua = (head1 & 0xe0) | (head2 & 0x1f);
	//完成一帧标志
	bool bFinishFrame = false;
	//std::cout << "begin" << std::endl;

	//int b = (int)nal;
	//std::cout << "nal  " << b << std::endl;
	//判断NAL的类型为0x1c=28,说明是FU-A分片
	if (nal == 0x1c)
	{
		//开始
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
	//单包数据
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
	
	// 销毁对话
	sess.BYEDestroy(RTPTime(10, 0), 0, 0);

	// Windows相关
#ifdef RTP_SOCKETTYPE_WINSOCK
	WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
}