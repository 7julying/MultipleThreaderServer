#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
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
	SOCKET AcceptSocket = (SOCKET)lpParameter;
	//���ջ������Ĵ�С��50���ַ�
	signal = signal + 1;
	number = signal;//���ͻ��˱��ѭ����һ�������θ���number
//��1�ſͻ���Ϊͷ��������1�ſͻ��˵ķ�����Ϣ
	if (number == 1)
	{
		while (1)
		{
			char buf[51] = { 0 };
			memset(buf, 0, 51);//ÿѭ��һ�ξ���ս�������
			count = recv(AcceptSocket, buf, 50, 0);
			memset(sendflag, 1, 6);//��ʼ��sendflag���飬ÿһλ�ֱ����һ���ͻ��˵ķ���״̬��û�з���Ϊ1
			strcpy(recvBuf, buf);
			if (count == 0)break;//���Է��ر�
			if (count == SOCKET_ERROR)break;//����count<0
			printf("�������Կͻ���%d����Ϣ��%s\n", AcceptSocket, recvBuf);
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
			Sleep(10);
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
	addrServer.sin_port = htons(20131);


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

