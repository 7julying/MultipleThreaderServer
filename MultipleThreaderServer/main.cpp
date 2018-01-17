#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <conio.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
int signal = 0;//signalΪ�ͻ��˱��
int count = 0;//countΪ���������ݳ���
char recvBuf[51] = { 0 };//���յ�����
char sendflag[5];//���ͱ�־λ

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	int number;
	unsigned long ul = 1;
	int nRet;
	SOCKET AcceptSocket = (SOCKET)lpParameter;
	//�����׽��ַ�����ģʽ
	nRet = ioctlsocket(AcceptSocket, FIONBIO, (unsigned long*)&ul);
	if (nRet == SOCKET_ERROR)
	{
		//�����׽��ַ�����ģʽ��ʧ�ܴ���
	}
	//���ջ������Ĵ�С��50���ַ�
	signal = signal + 1;
	number = signal;//���ͻ��˱��ѭ����һ�������θ���number
//��1�ſͻ���Ϊͷ��������1�ſͻ��˵ķ�����Ϣ
	if (number == 1)
	{
		while (1)
		{
			int c;
			if (_kbhit())//���������ް������а���_kbhit()����һ������ֵ
			{
				//����1����ǰ��������2����ֹͣ
				std::cin >> c;
				if (c != 0)
				{
					if (c == 1)
					{
						recvBuf[0] = 0xff;
						recvBuf[1] = 0x00;
						recvBuf[2] = 0x01;
						recvBuf[3] = 0x00;
						recvBuf[4] = 0xff;
						send(AcceptSocket, recvBuf, 5, 0);
					}
					else if (c == 2)
					{
						recvBuf[0] = 0xff;
						recvBuf[1] = 0x00;
						recvBuf[2] = 0x00;
						recvBuf[3] = 0x00;
						recvBuf[4] = 0xff;
						send(AcceptSocket, recvBuf, 5, 0);
					}
					c == 0;//��հ���״̬��Ϣ���������ж�
				}
			}
			char buf[51] = { 0 };
			memset(buf, 0, 51);//ÿѭ��һ�ξ���ս�������
			count = recv(AcceptSocket, buf, 50, 0);
			if (SOCKET_ERROR == count)
			{
				int err = WSAGetLastError();
				if (WSAEWOULDBLOCK == err)
				{
					continue;
				}
				else if (WSAETIMEDOUT == err || WSAENETDOWN == err)
				{
					break;
				}
			}
			else if (0 == count)
			{
				break;
			}
			memset(sendflag, 1, 6);//��ʼ��sendflag���飬ÿһλ�ֱ����һ���ͻ��˵ķ���״̬��û�з���Ϊ1
			memcpy(recvBuf, buf,sizeof buf);
			//��С��״̬�������Ļ
			std::string output;
			if (buf[2] == 0x00)
			{
				output = "stop";
			}
			else if (buf[2] == 0x01)
			{
				output = "forward";
			}
			else if (buf[2] == 0x02)
			{
				output = "back";
			}
			else if (buf[2] == 0x03)
			{
				output = "left";
			}
			else if (buf[2] == 0x04)
			{
				output = "right";
			}
			std::cout << "�������Կͻ���" << AcceptSocket << "����Ϣ:" << output << std::endl;
		}
	}
//��1�ſͻ��˵ķ�����Ϣ���͸������ͻ���
	else
	{
		while (1)
		{
			if (sendflag[number])
			{
				send(AcceptSocket, recvBuf, count, 0);
				sendflag[number] = 0;//�������ݺ������еĵ�numberλ��0�������ѷ���
			}
		}
	}

	//��������
	closesocket(AcceptSocket);
	return 0;
}

int main(int argc, char* argv[])
{
	//----------------------
	// �׽��ֳ�ʼ��
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// ��������׽��ֽ��տͻ��˷���
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// �������׽��ֵ�ַ��Э�鼰�˿ڵİ�
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //ʵ������0
	addrServer.sin_port = htons(8081);


	//���׽��ֵ�һ��IP��ַ��һ���˿���
	if (bind(ListenSocket, (SOCKADDR *)& addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();

		return 1;
	}

	//���׽�������Ϊ����ģʽ�ȴ���������
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//���տͻ�������
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	//��һ������ѭ���ķ�ʽ����ͣ�ؽ��տͻ���socket����
	while (1)
	{
		//�������󣬽����������󣬷���һ���µĶ�Ӧ�ڴ˴����ӵ��׽���
		SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&addrClient, &len);
		if (AcceptSocket == INVALID_SOCKET)break; //����

		//�����߳�
		DWORD dwThread;
		HANDLE hThread = CreateThread(NULL, 0, ThreadProc, (LPVOID)AcceptSocket, 0, &dwThread);
		if (hThread == NULL)
		{
			closesocket(AcceptSocket);
			wprintf(L"Thread Creat Failed!\n");
			break;
		}

		CloseHandle(hThread);
	}

	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}