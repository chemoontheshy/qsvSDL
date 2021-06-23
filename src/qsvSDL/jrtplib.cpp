#include "jrtplib.h"

/// <summary>
/// ���캯��
/// </summary>
JRTPLIB::JRTPLIB()
{
	
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
	// ��ӡJRTPLIB�汾
	std::cout << "Using version " << RTPLibraryVersion::GetVersion().GetVersionString() << std::endl;

}

void JRTPLIB::getPacket()
{
	int num = 30;

	for (int i = 1; i <= num; i++)
	{

		sess.BeginDataAccess();

		// ����հ�
		if (sess.GotoFirstSourceWithData())
		{
			do
			{
				RTPPacket* pack;
				int a = 0;
				while ((pack = sess.GetNextPacket()) != NULL)
				{
					a++;
					// ������������ݴ���
					printf("Got packet !%d\n", a);

					// ������Ҫ������ˣ�ɾ��֮
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
		RTPTime::Wait(RTPTime(1, 0));
	}
	free();
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