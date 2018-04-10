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
#include <math.h>

// Need to link with Ws2_32.lib
using namespace std;
#pragma comment(lib, "ws2_32.lib")			//动态库函数


/**
* 全局变量
*/
/**
*函数申明
*/
BOOL initServer(void);                       //初始化
void initMember(void);
bool initSocket(void);						//初始化非阻塞套接字
void exitServer(void);						//释放资源
bool startService(void);					//启动服务器
void showServerStartMsg(BOOL bSuc);         //显示错误信息
void showServerExitMsg(void);               //显示退出消息
BOOL createAcceptThread(void);      //开启监控函数
DWORD __stdcall acceptThread(void* pParam); //开启客户端请求线程
DWORD WINAPI timerThreadProc(__in LPVOID lpParameter);//开启计时器线程
void readyState(void);						//小车处于准备状态
void planState(void);						//小车处于计算状态
void moveState(void);						//小车处于运动状态
/*** 全局变量
*/
char	dataBuf[2];				//写缓冲区
BOOL	bConning;							//与客户端的连接状态
BOOL    bSend;                              //发送标记位
BOOL    clientConn;                         //连接客户端标记
SOCKET	listenSocket;							//服务器监听套接字
HANDLE	hAcceptThread;						//接受客户端连接线程句柄
HANDLE  hHelpThread;
HANDLE	hCleanThread;						//数据接收线程
int timerFlag;									//计时器标志位-计时器下一状态
int timeCount;									//运动时间
int carState;									//1代表READY,2代表PLAN,3代表MOVE
int carDirection;								//1代表UP，2代表DOWN,3代表LEFT，4代表RIGHT
int carMoveState;								//1代表STRAIGHT，2代表WAIT,3代表LEFT，4代表RIGHT
int masterSocketFlag;						//主车发送标志位
int bufCount;									//接受主车返回数据个数
char masterSocketStr[5];						//主车
int connectNum;									//连接个数
char slaveSocketFlag[5];						//从车发送标志位
char slaveSocketStr[5];						//从车
int obstacleFlag;								//障碍标志位
int xDistance;
int yDistance;
int xcopy;
int ycopy;
int stopFlag;
int turnningFlag;								//避障转弯标志位,为1时表示转弯完成执行直走命令

#define READY 1	//等待输入坐标
#define PLAN 2	//计算路径
#define MOVE 3	//小车运动
#define UP 1	//车头朝前
#define DOWN 2	//车头朝后
#define LEFT 3	//车头朝左或小车左转
#define RIGHT 4	//车头朝右或小车右转
#define STRAIGHT 1	//小车前进
#define WAIT 2	//维持当前状态等待计时器计时结束
#define STRAIGHT_1S 5	//小车避障转弯后直走
#define ORDER_STRAIGHT 0x01
#define ORDER_STOP 0x02
#define ORDER_LEFT 0x03
#define ORDER_RIGHT 0x04
#define ORDER_OBSTACLE 0x05
#define TURN_TIME 1000//转90度角所用时间
#define SERVER_SETUP_FALL 1//启动服务器失败
 
//初始化
BOOL initServer(void)
{
	//初始化全局变量
	initMember();

	//初始化SOCKET
	if (!initSocket())
		return FALSE;

	return TRUE;
}

//初始化全局变量
void initMember(void)
{
	bSend = FALSE;
	clientConn = FALSE;
	bConning = FALSE;									    //服务器为没有运行状态
	hAcceptThread = NULL;									//设置为NULL
	hCleanThread = NULL;
	listenSocket = INVALID_SOCKET;								//设置为无效的套接字
	timerFlag = 0;
	timeCount = 0;
	carState = 1;
	carDirection = 1;
	carMoveState = 2;
	masterSocketFlag = 0;
	memset(masterSocketStr, 0, 5);
	connectNum = 0;
	memset(slaveSocketFlag, 0, 5);
	memset(slaveSocketStr, 0, 5);
	bufCount = 0;
	obstacleFlag = 0;
	turnningFlag = 0;
	xDistance = -1;
	yDistance = -1;
	xcopy = -1;
	ycopy = -1;
	stopFlag = 0;
}

//初始化SOCKET
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

//启动服务
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


//接受客户端连接线程
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
	//printf("master car\n");
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理
	}

	printf("Master Car Connect\n");

	while (1)
	{
		int c;
		if (carState != READY)
		{
			if (_kbhit())//检测键盘有无按键，有按键_kbhit()返回一个非零值
			{
				//按键1代表停止
				std::cin >> c;
				if (c != 0)
				{
					if (c == 1)
					{
						timerFlag = 0;
						stopFlag = 1;
					}
					c = 0;//清空按键状态信息，避免误判断
				}
			}
		}
		char buf[51] = { 0 };
		memset(buf, 0, 51);//每循环一次就清空接收数据
		bufCount = recv(AcceptSocket, buf, 50, 0);
		if (SOCKET_ERROR == bufCount)
		{
			int err = WSAGetLastError();
			if (WSAETIMEDOUT == err || WSAENETDOWN == err)
			{
				break;
			}
		}		
		if(bufCount > 0)
		{
			if (buf[1] == ORDER_OBSTACLE)
			{
				timerFlag = 0;
				obstacleFlag = 1;
			}
			else
			{ 
				memset(slaveSocketFlag, 1, 6);//初始化sendflag数组，每一位分别代表一个客户端的发送状态，没有发送为1
				memcpy(slaveSocketStr, buf, sizeof slaveSocketStr);
			}
		}
		if (masterSocketFlag == 1)
		{
			send(AcceptSocket, masterSocketStr, 5, 0);
			masterSocketFlag = 0;
		}
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
	//printf("slave car\n");
	if (nRet == SOCKET_ERROR)
	{
		//设置套接字非阻塞模式，失败处理
	}
	int num = connectNum;
	printf("Slave Car %d Connect\n", num);
	
	while (1)
	{
		if (bufCount > 0)
		{
			if (slaveSocketFlag[num])
			{
				send(AcceptSocket, slaveSocketStr, 5, 0);
				slaveSocketFlag[num] = 0;//发送数据后将数组中的第number位置0，代表已发送
			}
		}
	}

	//结束连接
	closesocket(AcceptSocket);
	return 0;
}

//接受客户端连接
DWORD __stdcall acceptThread(void* pParam)
{
								                        //接受客户端连接的套接字
	sockaddr_in addrClient;						                        //客户端SOCKET地址
	int	lenClient = sizeof(sockaddr_in);				        	//地址长度
	while (bConning)						                                //服务器的状态
	{			
		SOCKET  acceptSocket = accept(listenSocket, (sockaddr*)&addrClient, &lenClient);	//接受客户请求
		if (INVALID_SOCKET == acceptSocket)
		{
			int x = GetLastError();
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
		}
	}
	return 0;//线程退出
}

//释放资源
void  exitServer(void)
{
	closesocket(listenSocket);					//关闭SOCKET
	listenSocket = INVALID_SOCKET;
	WSACleanup();							//卸载Windows Sockets DLL
}

//显示启动服务器成功与失败消息
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

//显示服务器退出消息
void  showServerExitMsg(void)
{

	cout << "**********************" << endl;
	cout << "* Server exit...     *" << endl;
	cout << "**********************" << endl;
}

DWORD WINAPI timerThreadProc(
__in LPVOID lpParameter
)
{
	clock_t startTime;
	clock_t endTime;
	int timerFlaglast = 0;
	while (1)
	{
		if (timerFlag == 1)
		{
			if (timerFlag==timerFlaglast)
			{
				endTime = clock();
				if ((endTime-startTime)>timeCount)
				{
					cout << "结束计时";
					timerFlag = 0;
					timeCount = 0;
				}
			}
			else
			{
				cout << "开始计时";
				startTime = clock();
			}
		}
		else
		{
			if (timerFlag != timerFlaglast)
			{
				cout << "结束计时";
				endTime = clock();
				timeCount = timeCount - (endTime - startTime);
			}
		}
			timerFlaglast = timerFlag;
		Sleep(1);
	}
}

int main(int argc, char* argv[])
{
	//初始化服务器
	if (!initServer())
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

	//启动定时器线程
	DWORD thread;
	HANDLE hThread = CreateThread(NULL, 0, timerThreadProc, NULL, 0, &thread);
	if (hThread == NULL)
	{
		printf("Thread Creat Failed!\n");
	}
	else
	{
		printf("Thread Creat Successful!\n");
	}
	//处理数据
	char c;
	cout << "键盘键入s开始运动:" << endl;
	std::cin >> c;
	if (c == 's' || c == 'S')
	{
		while (1)
		{
				switch (carState)
				{
				case READY:		
					readyState();
		
					
					break;
				case PLAN:
					planState();
					break;
				case MOVE:
					moveState();
					break;
				default:
					break;
				}	
		}
	}

	//退出主线程，清理资源
	exitServer();

	return 0;
}

void readyState()
{
			cout << "输入目的点坐标:" << endl;
			cout << "输入x坐标" << endl;
			cin >> xDistance;
			cout << "输入y坐标" << endl;
			cin >> yDistance;
			xDistance = xDistance * 1000;
			yDistance = yDistance * 1000;
			carState = PLAN;
}

void planState()
{
	xcopy = xDistance;
	ycopy = yDistance;
	if (stopFlag == 1)
	{
		stopFlag = 0;
		carState = READY;	
	}
	else
	{
		if (xDistance == 0 && yDistance == 0)
		{
			printf("运动到指定位置\n");
			carState = READY;
		}
		else
		{
			if (obstacleFlag == 1)
			{
				if (turnningFlag == 0)
				{
					turnningFlag = 1;
					switch (carDirection)
					{
					case UP:
						if (xDistance < 0)
						{
							carMoveState = LEFT;
							carDirection = LEFT;
							xDistance = xDistance + 1000;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = RIGHT;
							xDistance = xDistance - 1000;
						}
						break;
					case LEFT:
						if (yDistance < 0)
						{
							carMoveState = LEFT;
							carDirection = DOWN;
							yDistance = yDistance + 1000;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = UP;
							yDistance = yDistance - 1000;
						}
						break;
					case RIGHT:
						if (yDistance > 0)
						{
							carMoveState = LEFT;
							carDirection = UP;
							yDistance = yDistance - 1000;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = DOWN;
							yDistance = yDistance + 1000;
						}
						break;
					case DOWN:
						if (xDistance > 0)
						{
							carMoveState = LEFT;
							carDirection = RIGHT;
							xDistance = xDistance - 1000;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = LEFT;
							xDistance = xDistance + 1000;
						}
						break;
					}
				}
				else
				{
					carMoveState = STRAIGHT_1S;
					obstacleFlag = 0;
				}
			}
			else
			{
				switch (carDirection)
				{
				case UP:
					if (yDistance <= 0)
					{
						if (xDistance > 0)
						{
							carMoveState = RIGHT;
							carDirection = RIGHT;
						}
						else
						{
							carMoveState = LEFT;
							carDirection = LEFT;
						}
					}
					else
					{
						carMoveState = STRAIGHT;
						yDistance = 0;
					}
					break;
				case DOWN:
					if (yDistance >= 0)
					{
						if (xDistance > 0)
						{
							carMoveState = LEFT;
							carDirection = RIGHT;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = LEFT;
						}
					}
					else
					{
						carMoveState = STRAIGHT;
						yDistance = 0;
					}
					break;
				case LEFT:
					if (xDistance < 0)
					{
						carMoveState = STRAIGHT;
						xDistance = 0;
					}
					else
					{
						if (yDistance < 0)
						{
							carMoveState = LEFT;
							carDirection = DOWN;
						}
						else
						{
							carMoveState = RIGHT;
							carDirection = UP;
						}
					}
					break;
				case RIGHT:
					if (xDistance > 0)
					{
						carMoveState = STRAIGHT;
						xDistance = 0;
					}
					else
					{
						if (yDistance < 0)
						{
							carMoveState = RIGHT;
							carDirection = DOWN;
						}
						else
						{
							carMoveState = LEFT;
							carDirection = UP;
						}
					}
					break;
				default:
					break;
				}
			}		
			carState = MOVE;
		}
	}	
}

void moveState()
{
	switch (carMoveState)
	{
	case STRAIGHT:
		masterSocketStr[1] = ORDER_STRAIGHT;
		masterSocketFlag = 1;
		if (carDirection == UP || carDirection == DOWN)
		{
			timeCount = abs(ycopy);
		}
		else
		{
			timeCount = abs(xcopy);
		}
		timerFlag = 1;
		carMoveState = WAIT;
		break;
	case LEFT:
		masterSocketStr[1] = ORDER_LEFT;
		masterSocketFlag = 1;
		timeCount = TURN_TIME;
		timerFlag = 1;
		carMoveState = WAIT;
		break;
	case RIGHT:
		masterSocketStr[1] = ORDER_RIGHT;
		masterSocketFlag = 1;
		timeCount = TURN_TIME;
		timerFlag = 1;
		carMoveState = WAIT;
		break;
	case STRAIGHT_1S:
		masterSocketStr[1] = ORDER_STRAIGHT;
		masterSocketFlag = 1;
		timeCount = 1000;
		turnningFlag = 0;
		timerFlag = 1;
		carMoveState = WAIT;
	case WAIT:
		if (timerFlag == 0)
		{
			masterSocketStr[1] = ORDER_STOP;
			masterSocketFlag = 1;
			carState = PLAN;
			Sleep(1000);
			if (timeCount != 0)
			{
				switch (carDirection)
				{
				case UP:
					yDistance = yDistance + timeCount;
					break;
				case DOWN:
					yDistance = yDistance - timeCount;
					break;
				case LEFT:
					xDistance = xDistance - timeCount;
					break;
				case RIGHT:
					xDistance = xDistance + timeCount;
					break;
				default:
					break;
				}
			}
		}
		break;
	default:
		break;
	}
}