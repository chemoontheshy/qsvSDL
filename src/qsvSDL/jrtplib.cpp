#include "jrtplib.h"

/// <summary>
/// 构造函数
/// </summary>
JRTPLIB::JRTPLIB()
{
	
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
	getPacket();
}

void JRTPLIB::checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr);
		exit(-1);
	}

}

void JRTPLIB::getVersion()
{
	// 打印JRTPLIB版本
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;

}

void JRTPLIB::getPacket()
{
	int num = 30;

	for (int i = 1; i <= num; i++)
	{

		sess.BeginDataAccess();

		// 检查收包
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;
				int a = 0;
				while ((pack = sess.GetNextPacket()) != NULL)
				{
					a++;
					// 在这里进行数据处理
					printf("Got packet !%d\n", a);

					// 不再需要这个包了，删除之
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
		RTPTime::Wait(RTPTime(1, 0));
	}
	free();
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