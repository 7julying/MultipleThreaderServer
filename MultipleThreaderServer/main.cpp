#include <iostream>
#include <winsock2.h>
#include <winbase.h>
#include <vector>
#include <map>
#include <string>
#include <process.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <time.h>
// Need to link with Ws2_32.lib
using namespace std;
#pragma comment(lib, "ws2_32.lib")			//动态库函数

/**
* 全局变量
*/
/**
*函数申明
*/
BOOL initSever(void);                       //初始化
void initMember(void);
bool initSocket(void);						//初始化非阻塞套接字
void exitServer(void);						//释放资源
bool startService(void);					//启动服务器
void showServerStartMsg(BOOL bSuc);         //显示错误信息
void showServerExitMsg(void);               //显示退出消息
BOOL createAcceptThread(void);      //开启监控函数
DWORD __stdcall acceptThread(void* pParam); //开启客户端请求线程
/*** 全局变量
*/
char	dataBuf[2];				//写缓冲区
BOOL	bConning;							//与客户端的连接状态
BOOL    bSend;                              //发送标记位
BOOL    clientConn;                         //连接客户端标记
SOCKET	listenSocket;							//服务器监听套接字
HANDLE	hAcceptThread;						//数据处理线程句柄
HANDLE	hCleanThread;						//数据接收线程

											/**
											* 初始化
											*/
BOOL initSever(void)
{
	//初始化全局变量
	initMember();

	//初始化SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

/**
* 初始化全局变量
*/
void initMember(void)
{
	memset(dataBuf, 0, 2);
	bSend = FALSE;
	clientConn = FALSE;
	bConning = FALSE;									    //服务器为没有运行状态
	hAcceptThread = NULL;									//设置为NULL
	hCleanThread = NULL;
	listenSocket = INVALID_SOCKET;								//设置为无效的套接字
}

/**
*  初始化SOCKET
*/
bool initSocket(void)
{
	//返回值
	int reVal;

	//初始化Windows Sockets DLL
	WSADATA  wsData;
	reVal = WSAStartup(MAKEWORD(2, 2), &wsData);

	//创建套接字
	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
		//WSACleanup();
		return 1;
	}

	//设置套接字非阻塞模式
	
	/*unsigned long ul = 1;
	reVal = ioctlsocket(listenSocket, FIONBIO, (unsigned long*)&ul);
	if (SOCKET_ERROR == reVal)
		return FALSE;*/

	//绑定套接字
	sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	serAddr.sin_port = htons(8081);
	serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	reVal = bind(listenSocket, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (SOCKET_ERROR == reVal)
		return FALSE;

	//监听
	reVal = listen(listenSocket, 5);
	if (SOCKET_ERROR == reVal)
		return FALSE;

	return TRUE;
}

/**
*  启动服务
*/
bool startService(void)
{
	BOOL reVal = TRUE;	//返回值
			if (createAcceptThread())	//接受客户端请求的线程
			{
				showServerStartMsg(TRUE);		//创建线程成功信息
			}
			else {
				reVal = FALSE;
			}
	return reVal;
}

/**
* 接受客户端连接线程
*/
BOOL createAcceptThread(void)
{
	bConning = TRUE;//设置服务器为运行状态

					//创建释放资源线程
	unsigned long ulThreadId;
	//创建接收客户端请求线程
	hAcceptThread = CreateThread(NULL, 0, acceptThread, NULL, 0, &ulThreadId);
	if (NULL == hAcceptThread)
	{
		bConning = FALSE;
		return FALSE;
	}
	else
	{
		CloseHandle(hAcceptThread);
	}
	return TRUE;
}

DWORD WINAPI carMasterThreadProc(
	__in  LPVOID lpParameter
)
{
	unsigned long ul = 1;
	int nRet;
	SOCKET AcceptSocket = (SOCKET)lpParameter;
	//设置套接字非阻塞模式
	nRet = ioctlsocket(AcceptSocket, FIONBIO, (unsigned long*)&ul);
	printf("master car\n");
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理
	}
	//结束连接
	closesocket(AcceptSocket);
	return 0;
}

DWORD WINAPI carSlaveThreadProc(
	__in  LPVOID lpParameter
)
{
	unsigned long ul = 1;
	int nRet;
	SOCKET AcceptSocket = (SOCKET)lpParameter;
	//设置套接字非阻塞模式
	nRet = ioctlsocket(AcceptSocket, FIONBIO, (unsigned long*)&ul);
	printf("slave car\n");
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理
	}
	//结束连接
	closesocket(AcceptSocket);
	return 0;
}

/**
* 接受客户端连接
*/
DWORD __stdcall acceptThread(void* pParam)
{
								                        //接受客户端连接的套接字
	sockaddr_in addrClient;						                        //客户端SOCKET地址
	int	lenClient = sizeof(sockaddr_in);				        	//地址长度
	int connectNum = 0;//连接个数
	while (bConning)						                                //服务器的状态
	{			
		SOCKET  acceptSocket = accept(listenSocket, (sockaddr*)&addrClient, &lenClient);	//接受客户请求
		if (INVALID_SOCKET == acceptSocket)
		{
			int x = GetLastError();
			Sleep(1);
			continue;
		}
		else//接受客户端的请求
		{
			connectNum++;
			if (connectNum == 1)
			{
				//启动线程
				DWORD dwThread;
				HANDLE hThread = CreateThread(NULL, 0, carMasterThreadProc, (LPVOID)acceptSocket, 0, &dwThread);
				if (hThread == NULL)
				{
					closesocket(acceptSocket);
					wprintf(L"Thread Creat Failed!\n");
					break;
				}
				CloseHandle(hThread);
			}
			else if (connectNum > 1)
			{
				//启动线程
				DWORD dwThread;
				HANDLE hThread = CreateThread(NULL, 0, carSlaveThreadProc, (LPVOID)acceptSocket, 0, &dwThread);
				if (hThread == NULL)
				{
					closesocket(acceptSocket);
					wprintf(L"Thread Creat Failed!\n");
					break;
				}
				CloseHandle(hThread);
			}
			connectNum++;
		}
	}
	return 0;//线程退出
}

/**
*  释放资源
*/
void  exitServer(void)
{
	closesocket(listenSocket);					//关闭SOCKET
	listenSocket = INVALID_SOCKET;
	WSACleanup();							//卸载Windows Sockets DLL
}

/**
* 显示启动服务器成功与失败消息
*/
void  showServerStartMsg(BOOL bSuc)
{
	if (bSuc)
	{
		cout << "**********************" << endl;
		cout << "* Server succeeded!  *" << endl;
		cout << "**********************" << endl;
	}
	else {
		cout << "**********************" << endl;
		cout << "* Server failed   !  *" << endl;
		cout << "**********************" << endl;
	}

}

/**
* 显示服务器退出消息
*/
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}

int main(int argc, char* argv[])
{
	//初始化服务器
	if (!initSever())
	{
		exitServer();
		return 1;
	}

	//启动服务
	if (!startService())
	{
		showServerStartMsg(FALSE);
		exitServer();
		return 1;
	}

	//处理数据

	//退出主线程，清理资源
	exitServer();

	return 0;
}