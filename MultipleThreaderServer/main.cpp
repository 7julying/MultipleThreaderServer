#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <conio.h>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
int signal = 0;//signal为客户端编号
int count = 0;//count为读到的数据长度
char recvBuf[51] = { 0 };//接收的数据
char sendflag[5];//发送标志位

DWORD WINAPI ThreadProc(
	__in  LPVOID lpParameter
	)
{
	int number;
	unsigned long ul = 1;
	int nRet;
	SOCKET AcceptSocket = (SOCKET)lpParameter;
	//设置套接字非阻塞模式
	nRet = ioctlsocket(AcceptSocket, FIONBIO, (unsigned long*)&ul);
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理
	}
	//接收缓冲区的大小是50个字符
	signal = signal + 1;
	number = signal;//将客户端编号循环加一，并依次赋给number
//设1号客户端为头车，接收1号客户端的反馈信息
	if (number == 1)
	{
		while (1)
		{
			int c;
			if (_kbhit())//检测键盘有无按键，有按键_kbhit()返回一个非零值
			{
				//按键1代表前进，按键2代表停止
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
					c == 0;//清空按键状态信息，避免误判断
				}
			}
			char buf[51] = { 0 };
			memset(buf, 0, 51);//每循环一次就清空接收数据
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
			memset(sendflag, 1, 6);//初始化sendflag数组，每一位分别代表一个客户端的发送状态，没有发送为1
			memcpy(recvBuf, buf,sizeof buf);
			//将小车状态输出到屏幕
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
			std::cout << "接收来自客户端" << AcceptSocket << "的信息:" << output << std::endl;
		}
	}
//将1号客户端的反馈信息发送给其他客户端
	else
	{
		while (1)
		{
			if (sendflag[number])
			{
				send(AcceptSocket, recvBuf, count, 0);
				sendflag[number] = 0;//发送数据后将数组中的第number位置0，代表已发送
			}
		}
	}

	//结束连接
	closesocket(AcceptSocket);
	return 0;
}

int main(int argc, char* argv[])
{
	//----------------------
	// 套接字初始化
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		wprintf(L"WSAStartup failed with error: %ld\n", iResult);
		return 1;
	}
	//----------------------
	// 创造监听套接字接收客户端访问
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	//----------------------
	// 服务器套接字地址，协议及端口的绑定
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY); //实际上是0
	addrServer.sin_port = htons(8081);


	//绑定套接字到一个IP地址和一个端口上
	if (bind(ListenSocket, (SOCKADDR *)& addrServer, sizeof (addrServer)) == SOCKET_ERROR) {
		wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();

		return 1;
	}

	//将套接字设置为监听模式等待连接请求
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//接收客户端请求
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	//以一个无限循环的方式，不停地接收客户端socket连接
	while (1)
	{
		//请求到来后，接受连接请求，返回一个新的对应于此次连接的套接字
		SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&addrClient, &len);
		if (AcceptSocket == INVALID_SOCKET)break; //出错

		//启动线程
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