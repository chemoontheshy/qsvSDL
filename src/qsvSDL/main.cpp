//��ǰC++����C����
extern "C"
{
	//avcodec:�����(����Ҫ�Ŀ�)
#include <libavcodec/avcodec.h>
//avformat:��װ��ʽ����
#include <libavformat/avformat.h>
//swscale:��Ƶ�������ݸ�ʽת��
#include <libswscale/swscale.h>
//avdevice:�����豸���������
#include <libavdevice/avdevice.h>
//avutil:���߿⣨�󲿷ֿⶼ��Ҫ������֧�֣�
#include <libavutil/avutil.h>
}

#include<iostream>

using namespace std;

int main()
{
	cout << "test git" << endl;
	cout << avcodec_configuration() << endl;
}