// SPServer.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <thread>
#include <WS2tcpip.h>
#include <map>

#include "protocol.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#define MAX_CLIENTS 10

struct SOCKETINFO
{
	WSAOVERLAPPED	overlapped;
	WSABUF			senddataBuf;
	WSABUF			recvdataBuf;
	SOCKET			socket;
	char			recvmessageBuf[MAX_BUFFER];
	char			sendmessageBuf[MAX_BUFFER];

	int				x, y;
	bool			connected;
};

map<SOCKET, SOCKETINFO> clients;

// 오류 출력 함수
void err_quit(char* msg);
void err_display(char* msg);

int GetID();
void process_packet(int id, char* szMessagebuf);
void login_packet(int id, SOCKET soc);
void logout_packet(int id, SOCKET soc);

void CALLBACK recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED oerlapped, DWORD lnFlags);
void CALLBACK send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED oerlapped, DWORD lnFlags);

// 10명 넘을시 짤라내거나 대기한다

int g_iCurID = -1;

void recv_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0;

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}

	int iIdx = -1;

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (client_s == clients[i].socket) //
		{
			clients[i].recvmessageBuf[dataBytes] = 0;
			cout << "from clients : " /*<< clients[i].recvmessageBuf*/ << " (" << dataBytes << ") bytes)" << endl;

			clients[i].recvdataBuf.len = dataBytes;
			memset(&(clients[i].overlapped), 0x00, sizeof(WSAOVERLAPPED)); // 재사용하므로 초기화
			clients[i].overlapped.hEvent = (HANDLE)client_s; // 초기화 되었으므로 이벤트에 소켓을 넣어줌
			
			if (dataBytes == sizeof(sc_packet_logout))
			{
				logout_packet(i, client_s);
			}
			else if (dataBytes == sizeof(sc_packet_pos))
			{
				process_packet(i, clients[i].recvmessageBuf);
			}
			iIdx = i;
		}
	}

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i].connected)
		{
			WSAOVERLAPPED* overlappedSend = new WSAOVERLAPPED();
			overlappedSend->hEvent = (HANDLE)(clients[i].socket);

			if(WSASend(clients[i].socket, &(clients[iIdx].senddataBuf), 1, 0, 0, overlappedSend, send_callback))
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					cout << "error - Fail WSASend(error_code : %d) : " << WSAGetLastError() << endl;
				}
			}

			//delete overlappedSend;

			//if (WSASend(clients[i].socket, &(clients[iIdx].senddataBuf), 1, &dataBytes, 0,
			//	&(clients[i].overlapped), send_callback) == SOCKET_ERROR)
			//{
			//	if (WSAGetLastError() != WSA_IO_PENDING)
			//	{
			//		cout << "error - Fail WSASend(error_code : %d) : " << WSAGetLastError() << endl;
			//	}
			//}
		}
	}
}

void send_callback(DWORD Error, DWORD dataBytes, LPWSAOVERLAPPED overlapped, DWORD lnFlags)
{
	DWORD sendBytes = 0;
	DWORD receiveBytes = 0;
	DWORD flags = 0; // lpnumberofdata 가 0이 아니면 동기식으로 작동함

	SOCKET client_s = reinterpret_cast<int>(overlapped->hEvent);

	if (dataBytes == 0)
	{
		closesocket(clients[client_s].socket);
		clients.erase(client_s);
		return;
	}  // 클라이언트가 closesocket을 했을 경우


	// WSASend(응답에 대한)의 콜백일 경우
	//for (int i = 0; i < MAX_CLIENTS; ++i)
	//{
	//	if (client_s == clients[i].socket) //
	//	{
	//		clients[i].senddataBuf[i].len = MAX_BUFFER;
	//		clients[i].senddataBuf[i].buf = clients[i].sendmessageBuf;

	//		cout << "trace - Send message : " << clients[i].sendmessageBuf << " (" << dataBytes << " bytes)" << endl;
	//		memset(&(clients[i].overlapped), 0x00, sizeof(WSAOVERLAPPED));
	//		clients[i].overlapped.hEvent = (HANDLE)client_s;
	//	}
	//}

	// 왜 이게 들어가야만 돌아가는지
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (client_s == clients[i].socket/*clients[i].connected*/)
		{
			memset(&(clients[i].overlapped), 0x00, sizeof(WSAOVERLAPPED));
			clients[i].overlapped.hEvent = (HANDLE)client_s;

			if (WSARecv(clients[i].socket, &clients[i].recvdataBuf, 1, &receiveBytes,
				&flags, &(clients[i].overlapped), recv_callback) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					cout << "error - Fail WSARecv(error_code : %d) : " << WSAGetLastError() << endl;
				}
			}
		}
	}

	delete overlapped;

	//delete overlapped;
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << "error - " << (char*)lpMsgBuf << endl;
	LocalFree(lpMsgBuf);
}

int GetID()
{
	while (true)
	{
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (false == clients[i].connected)
				return i;
		}
	}
}

void process_packet(int id, char* szMessagebuf)
{
	sc_packet_pos* packet = reinterpret_cast<sc_packet_pos*>(szMessagebuf);

	int x = clients[id].x;
	int y = clients[id].y;

	switch (packet->type)
	{
	case CS_UP:
		if (y > 0)
		{
			y -= BOARD_GAP;
		}
		break;
	case CS_DOWN:
		if (y < BOARD_GAP * 7)
		{
			y += BOARD_GAP;
		}
		break;
	case CS_LEFT:
		if (0 < x)
		{
			x -= BOARD_GAP;
		}
		break;
	case CS_RIGHT:
		if ((BOARD_GAP * 7) > x)
		{
			x += BOARD_GAP;
		}
		break;
	default:
		cout << "Unknown Packet Type Error\n";
		break;
	}

	clients[id].x = x;
	clients[id].y = y;

	packet->x = x;
	packet->y = y;
	packet->id = id;
	packet->size = packet->size;
	packet->type = CS_NONE;

	memcpy(clients[id].sendmessageBuf, packet, sizeof(struct sc_packet_pos));
}

void login_packet(int id, SOCKET soc)
{
	int iIdx = -1;

	// 새로운 클라 들어간다
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i].socket == soc && !clients[i].connected)
		{
			sc_packet_login packet;
			packet.size = sizeof(sc_packet_login);
			packet.type = SC_LOGIN;
			packet.id = i;
			clients[i].connected = true;

			DWORD dataBytes = packet.size;

			memcpy(clients[i].sendmessageBuf, &packet, sizeof(struct sc_packet_login));

			for (int j = 0; j <= i; ++j) // 이런식으로 wsasend함수 호출해도 되는지
			{
				if (clients[j].connected)
				{
					WSAOVERLAPPED* overlappedSend = new WSAOVERLAPPED();
					overlappedSend->hEvent = (HANDLE)(clients[j].socket);

					WSASend(clients[j].socket, &(clients[i].senddataBuf), 1, 0, 0,
						overlappedSend, send_callback);
				}
			}

			iIdx = i;
			break;
		}
	}

	// 이전 애들 넣어주는것으로 빼기
	for (int i = 0; i <= MAX_CLIENTS; ++i)
	{
		if (clients[i].connected)
		{
			sc_packet_pos pospacket;
			pospacket.size = sizeof(sc_packet_pos);
			pospacket.id = i;
			pospacket.x = clients[i].x;
			pospacket.y = clients[i].y;

			DWORD dataBytes = pospacket.size;

			memcpy(clients[i].sendmessageBuf, &pospacket, sizeof(struct sc_packet_pos));

			WSAOVERLAPPED* overlappedSend = new WSAOVERLAPPED();
			overlappedSend->hEvent = (HANDLE)(clients[i].socket);

			// 이런식으로 wsasend함수 호출해도 되는지
			WSASend(clients[iIdx].socket, &(clients[i].senddataBuf), 1, 0, 0,
				overlappedSend, send_callback);
		}
	}
}

void logout_packet(int id, SOCKET soc)
{
	int iIdx = -1;

	sc_packet_logout packet;
	packet.size = sizeof(packet);
	packet.type = SC_LOGOUT;
	packet.id = id;

	closesocket(clients[id].socket);
	clients[id].connected = false;
	clients[id].x = 0.f;
	clients[id].y = 0.f;

	DWORD dataBytes = packet.size;

	memcpy(clients[id].sendmessageBuf, &packet, sizeof(struct sc_packet_pos));

}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	clock_t Curtime = 0;

	clock_t Oldtime = clock(); // clock() 함수는 프로그램이 시작된 이후의 시간을  밀리세컨드 단위로 알려줌
	auto last_time = chrono::high_resolution_clock::now();
	while (1)
	{
		auto duration_time = chrono::high_resolution_clock::now() - last_time;
		int durationMs = chrono::duration_cast<chrono::milliseconds>(duration_time).count();
		//초당 20번 전송
		if (durationMs > 50)
		{
			//process_packet(pos);
			last_time = chrono::high_resolution_clock::now();
		}
	}

	return 0;
}

int main()
{
	int retval = 0;

	HANDLE hThread = NULL;
	//hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "error - Can not load 'winsock.dll' file\n";
		return 0;
	}

	SOCKET listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "error - Invalid socket\n";
		return 0;
	}

	// 서버정보 객체설정 
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = bind(listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit("error - bind()");

	// 수신 대기열
	if (listen(listenSocket, 10) == SOCKET_ERROR)
	{
		cout << "error - listen() wait" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}

	
	SOCKADDR_IN clientaddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientaddr, 0, addrLen);
	SOCKET clientSocket;
	DWORD flags;


	while (true)
	{
		// 현재 클라수가 10이 넘어가면 accept거부?

		clientSocket = accept(listenSocket, (SOCKADDR*)&clientaddr, &addrLen);
		if (clientSocket == INVALID_SOCKET) {
			err_display("error - accept()");
		}
		
		// 여기에 들어가면 안될듯 

		int iCount = 0;

		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (clients[i].connected)
				++iCount;
		}

		if (iCount == MAX_CLIENTS)
		{
			cout << "최대 클라이언트 수를 넘었습니다." << endl;
			continue;
		}

		int id = GetID();

		clients[id] = SOCKETINFO{};
		clients[id].socket = clientSocket;
		clients[id].senddataBuf.len = MAX_BUFFER;
		clients[id].senddataBuf.buf = clients[id].sendmessageBuf;
		clients[id].recvdataBuf.len = MAX_BUFFER;
		clients[id].recvdataBuf.buf = clients[id].recvmessageBuf;
		memset(&clients[id].overlapped, 0, sizeof(WSAOVERLAPPED));
		clients[id].overlapped.hEvent = (HANDLE)clients[id].socket;

		clients[id].x = 0;
		clients[id].y = 0;

		login_packet(id, clientSocket);

		DWORD flags = 0;

		cout << id << "번 클라이언트 입장" << endl;

		if (WSARecv(clients[id].socket, &clients[id].recvdataBuf, 1
			, NULL, &flags, &(clients[id].overlapped), recv_callback)) // 일단 한개 클라
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "error - IO pending failure" << endl;
				return 0;
			}
		}
		else
		{
			cout << "non overlapped recv return" << endl;
			return 0;
		}
	}

	WaitForMultipleObjects(2, &hThread, TRUE, INFINITE);

	closesocket(listenSocket);
	CloseHandle(hThread);

	// 윈속 종료
	WSACleanup();
	return 0;
}
